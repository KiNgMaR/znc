/*
 * Copyright (C) 2004-2015 ZNC, see the NOTICE file for details.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <znc/FileUtils.h>
#include <znc/IRCSock.h>
#include <znc/Server.h>
#include <znc/User.h>
#include <znc/IRCNetwork.h>
#include <znc/Config.h>
#include <tuple>

using std::endl;
using std::cout;
using std::map;
using std::set;
using std::vector;
using std::list;
using std::tuple;
using std::make_tuple;

static inline CString FormatBindError() {
	CString sError;

#ifdef _WIN32
	if(!CUtils::Win32StringError(::WSAGetLastError(), sError))
		sError = "unknown error, check the host name";
#else
	sError = (errno == 0 ? CString("unknown error, check the host name") : CString(strerror(errno)));
#endif

	return "Unable to bind [" + sError + "]";
}

CZNC::CZNC() {
	if (!InitCsocket()) {
		CUtils::PrintError("Could not initialize Csocket!");
		exit(-1);
	}

	m_pModules = new CModules();
	m_uiConnectDelay = 5;
	m_uiAnonIPLimit = 10;
	m_uBytesRead = 0;
	m_uBytesWritten = 0;
	m_uiMaxBufferSize = 500;
	m_pConnectQueueTimer = NULL;
	m_uiConnectPaused = 0;
	m_eConfigState = ECONFIG_NOTHING;
	m_TimeStarted = time(NULL);
	m_sConnectThrottle.SetTTL(30000);
	m_pLockFile = NULL;
	m_bProtectWebSessions = true;
	m_bHideVersion = false;
	m_uDisabledSSLProtocols = Csock::EDP_SSL;
	m_sSSLProtocols = "";
}

CZNC::~CZNC() {
	m_pModules->UnloadAll();

	for (map<CString,CUser*>::iterator a = m_msUsers.begin(); a != m_msUsers.end(); ++a) {
		a->second->GetModules().UnloadAll();

		const vector<CIRCNetwork*>& networks = a->second->GetNetworks();
		for (vector<CIRCNetwork*>::const_iterator b = networks.begin(); b != networks.end(); ++b) {
			(*b)->GetModules().UnloadAll();
		}
	}

	for (size_t b = 0; b < m_vpListeners.size(); b++) {
		delete m_vpListeners[b];
	}

	for (map<CString,CUser*>::iterator a = m_msUsers.begin(); a != m_msUsers.end(); ++a) {
		a->second->SetBeingDeleted(true);
	}

	m_pConnectQueueTimer = NULL;
	// This deletes m_pConnectQueueTimer
	m_Manager.Cleanup();
	DeleteUsers();

	delete m_pModules;
	delete m_pLockFile;

	ShutdownCsocket();
	DeletePidFile();
}

CString CZNC::GetVersion() {
	return CString(VERSION_STR) + CString(ZNC_VERSION_EXTRA);
}

CString CZNC::GetTag(bool bIncludeVersion, bool bHTML) {
	if (!Get().m_bHideVersion) {
		bIncludeVersion = true;
	}
	CString sAddress = bHTML ? "<a href=\"http://znc.in\">http://znc.in</a>" : "http://znc.in";

	if (!bIncludeVersion) {
		return "ZNC - " + sAddress;
	}

	CString sVersion = GetVersion();

	return "ZNC " + sVersion + " - " + sAddress;
}

CString CZNC::GetCompileOptionsString() {
	return
		"IPv6: "
#ifdef HAVE_IPV6
		"yes"
#else
		"no"
#endif
		", SSL: "
#ifdef HAVE_LIBSSL
		"yes"
#else
		"no"
#endif
		", DNS: "
#ifdef HAVE_THREADED_DNS
		"threads"
#else
		"blocking"
#endif
		", charset: "
#ifdef HAVE_ICU
		"yes"
#else
		"no"
#endif
	;
}

CString CZNC::GetUptime() const {
	time_t now = time(NULL);
	return CString::ToTimeStr(now - TimeStarted());
}

bool CZNC::OnBoot() {
	bool bFail = false;
	ALLMODULECALL(OnBoot(), &bFail);
	if (bFail) return false;

	return true;
}

bool CZNC::HandleUserDeletion()
{
	map<CString, CUser*>::iterator it;
	map<CString, CUser*>::iterator end;

	if (m_msDelUsers.empty())
		return false;

	end = m_msDelUsers.end();
	for (it = m_msDelUsers.begin(); it != end; ++it) {
		CUser* pUser = it->second;
		pUser->SetBeingDeleted(true);

		if (GetModules().OnDeleteUser(*pUser)) {
			pUser->SetBeingDeleted(false);
			continue;
		}
		m_msUsers.erase(pUser->GetUserName());
		CWebSock::FinishUserSessions(*pUser);
		delete pUser;
	}

	m_msDelUsers.clear();

	return true;
}

void CZNC::Loop(bool& bKeepRunning) {
	while (bKeepRunning) {
		LoopDoMaintenance();

		// always react within 1 second:
		m_Manager.DynamicSelectLoop(100 * 1000, 1000 * 1000);
	}

	Broadcast("ZNC has been requested to shut down!");

	if (!WriteConfig())	{
		CUtils::PrintError("Writing config file to disk failed! We are forced to continue shutting down anyway.");
	}
}

void CZNC::Loop() {
	while (true) {
		LoopDoMaintenance();

		// Csocket wants micro seconds
		// 100 msec to 600 sec
		m_Manager.DynamicSelectLoop(100 * 1000, 600 * 1000 * 1000);
	}
}

void CZNC::LoopDoMaintenance() {
	CString sError;

	ConfigState eState = GetConfigState();
	switch (eState) {
	case ECONFIG_NEED_REHASH:
		SetConfigState(ECONFIG_NOTHING);

		if (RehashConfig(sError)) {
			Broadcast("Rehashing succeeded", true);
		}
		else {
			Broadcast("Rehashing failed: " + sError, true);
			Broadcast("ZNC is in some possibly inconsistent state!", true);
		}
		break;
	case ECONFIG_NEED_WRITE:
	case ECONFIG_NEED_VERBOSE_WRITE:
		SetConfigState(ECONFIG_NOTHING);

		if (!WriteConfig()) {
			Broadcast("Writing the config file failed", true);
		}
		else if (eState == ECONFIG_NEED_VERBOSE_WRITE) {
			Broadcast("Writing the config succeeded", true);
		}
		break;
	case ECONFIG_NOTHING:
		break;
	}

	// Check for users that need to be deleted
	if (HandleUserDeletion()) {
		// Also remove those user(s) from the config file
		WriteConfig();
	}
}

CFile* CZNC::InitPidFile() {
	if (!m_sPidFile.empty()) {
		CString sFile;

		// absolute path or relative to the data dir?
		if (m_sPidFile[0] != '/')
			sFile = GetZNCPath() + "/" + m_sPidFile;
		else
			sFile = m_sPidFile;

		return new CFile(sFile);
	}

	return NULL;
}

bool CZNC::WritePidFile(int iPid) {
	CFile* File = InitPidFile();
	if (File == NULL)
		return false;

	CUtils::PrintAction("Writing pid file [" + File->GetLongName() + "]");

	bool bRet = false;
	if (File->Open(O_WRONLY | O_TRUNC | O_CREAT)) {
		File->Write(CString(iPid) + "\n");
		File->Close();
		bRet = true;
	}

	delete File;
	CUtils::PrintStatus(bRet);
	return bRet;
}

bool CZNC::DeletePidFile() {
	CFile* File = InitPidFile();
	if (File == NULL)
		return false;

	CUtils::PrintAction("Deleting pid file [" + File->GetLongName() + "]");

	bool bRet = File->Delete();

	delete File;
	CUtils::PrintStatus(bRet);
	return bRet;
}

bool CZNC::WritePemFile() {
#ifndef HAVE_LIBSSL
	CUtils::PrintError("ZNC was not compiled with ssl support.");
	return false;
#else
	CString sPemFile = GetPemLocation();

	CUtils::PrintAction("Writing Pem file [" + sPemFile + "]");
#ifndef _WIN32
    int fd = creat(sPemFile.c_str(), 0600);
	if (fd == -1) {
		CUtils::PrintStatus(false, "Unable to open");
		return false;
	}
    FILE *f = fdopen(fd, "w");
#else
	FILE *f = fopen(sPemFile.c_str(), "w");
#endif

	if (!f) {
		CUtils::PrintStatus(false, "Unable to open");
		return false;
	}

	CUtils::GenerateCert(f, "");
	fclose(f);

	CUtils::PrintStatus(true);
	return true;
#endif
}

void CZNC::DeleteUsers() {
	for (map<CString,CUser*>::iterator a = m_msUsers.begin(); a != m_msUsers.end(); ++a) {
		a->second->SetBeingDeleted(true);
		delete a->second;
	}

	m_msUsers.clear();
	DisableConnectQueue();
}

bool CZNC::IsHostAllowed(const CString& sHostMask) const {
	for (map<CString,CUser*>::const_iterator a = m_msUsers.begin(); a != m_msUsers.end(); ++a) {
		if (a->second->IsHostAllowed(sHostMask)) {
			return true;
		}
	}

	return false;
}

bool CZNC::AllowConnectionFrom(const CString& sIP) const {
	if (m_uiAnonIPLimit == 0)
		return true;
	return (GetManager().GetAnonConnectionCount(sIP) < m_uiAnonIPLimit);
}

void CZNC::InitDirs(const CString& sArgvPath, const CString& sDataDir) {
#ifndef _WIN32
	// If the bin was not ran from the current directory, we need to add that dir onto our cwd
	CString::size_type uPos = sArgvPath.rfind('/');
	if (uPos == CString::npos)
		m_sCurPath = "./";
	else
		m_sCurPath = CDir::ChangeDir("./", sArgvPath.Left(uPos), "");

	// Try to set the user's home dir, default to binpath on failure
	CFile::InitHomePath(m_sCurPath);

	if (sDataDir.empty()) {
		m_sZNCPath = CFile::GetHomePath() + "/.znc";
	} else {
		m_sZNCPath = sDataDir;
	}
#else
	char strPath[MAX_PATH + 1] = {0};

	m_sCurPath = CDir::ChangeDir("./", "", "");

	CFile::InitHomePath(m_sCurPath);

	if (!sDataDir.empty())
	{
		// a custom datadir overrides everything else.
		m_sZNCPath = CDir::ChangeDir("./", sDataDir, "");
	}
	else
	{
		// .znc dir in current folder overrides default
		// to maintain backwards compatibility.
		m_sZNCPath = CDir::ChangeDir(m_sCurPath, ".znc", "");

		if (!::PathIsDirectory(m_sZNCPath.c_str()))
		{
			*strPath = 0;

			// default config location = Application Data\ZNC.
			if (SUCCEEDED(::SHGetFolderPath(0, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath)) && ::PathIsDirectory(strPath))
			{
				m_sZNCPath = CDir::ChangeDir(strPath, "ZNC", "");
			}
		}
	}
#endif

	m_sSSLCertFile = m_sZNCPath + "/znc.pem";
}

CString CZNC::GetConfPath(bool bAllowMkDir) const {
	CString sConfPath = m_sZNCPath + "/configs";
	if (bAllowMkDir && !CFile::Exists(sConfPath)) {
		CDir::MakeDir(sConfPath);
	}

	return sConfPath;
}

CString CZNC::GetUserPath() const {
	CString sUserPath = m_sZNCPath + "/users";
	if (!CFile::Exists(sUserPath)) {
		CDir::MakeDir(sUserPath);
	}

	return sUserPath;
}

CString CZNC::GetModPath() const {
	CString sModPath = m_sZNCPath + "/modules";

	return sModPath;
}

const CString& CZNC::GetCurPath() const {
	if (!CFile::Exists(m_sCurPath)) {
		CDir::MakeDir(m_sCurPath);
	}
	return m_sCurPath;
}

const CString& CZNC::GetHomePath() const {
	return CFile::GetHomePath();
}

const CString& CZNC::GetZNCPath() const {
	if (!CFile::Exists(m_sZNCPath)) {
		CDir::MakeDir(m_sZNCPath);
	}
	return m_sZNCPath;
}

CString CZNC::GetPemLocation() const {
	return CDir::ChangeDir("", m_sSSLCertFile);
}

CString CZNC::ExpandConfigPath(const CString& sConfigFile, bool bAllowMkDir) {
	CString sRetPath;

	if (sConfigFile.empty()) {
		sRetPath = GetConfPath(bAllowMkDir) + "/znc.conf";
	} else {
		if (sConfigFile.Left(2) == "./" || sConfigFile.Left(3) == "../") {
			sRetPath = GetCurPath() + "/" + sConfigFile;
		} else if (sConfigFile.Left(1) != "/") {
			sRetPath = GetConfPath(bAllowMkDir) + "/" + sConfigFile;
		} else {
			sRetPath = sConfigFile;
		}
	}

	return sRetPath;
}

bool CZNC::WriteConfig() {
	if (GetConfigFile().empty()) {
		DEBUG("Config file name is empty?!");
		return false;
	}

	// We first write to a temporary file and then move it to the right place
	CFile *pFile = new CFile(GetConfigFile() + "~");

	if (!pFile->Open(O_WRONLY | O_CREAT | O_TRUNC, 0600)) {
		DEBUG("Could not write config to " + GetConfigFile() + "~: " + CString(strerror(errno)));
		delete pFile;
		return false;
	}

	// We have to "transfer" our lock on the config to the new file.
	// The old file (= inode) is going away and thus a lock on it would be
	// useless. These lock should always succeed (races, anyone?).
	if (!pFile->TryExLock()) {
		DEBUG("Error while locking the new config file, errno says: " + CString(strerror(errno)));
		pFile->Delete();
		delete pFile;
		return false;
	}

	pFile->Write(MakeConfigHeader() + "\n");

	CConfig config;
	config.AddKeyValuePair("AnonIPLimit", CString(m_uiAnonIPLimit));
	config.AddKeyValuePair("MaxBufferSize", CString(m_uiMaxBufferSize));
	config.AddKeyValuePair("SSLCertFile", CString(m_sSSLCertFile));
	config.AddKeyValuePair("ProtectWebSessions", CString(m_bProtectWebSessions));
	config.AddKeyValuePair("HideVersion", CString(m_bHideVersion));
	config.AddKeyValuePair("Version", CString(VERSION_STR));

	for (size_t l = 0; l < m_vpListeners.size(); l++) {
		CListener* pListener = m_vpListeners[l];
		CConfig listenerConfig;

		listenerConfig.AddKeyValuePair("Host", pListener->GetBindHost());
		listenerConfig.AddKeyValuePair("URIPrefix", pListener->GetURIPrefix() + "/");
		listenerConfig.AddKeyValuePair("Port", CString(pListener->GetPort()));

		listenerConfig.AddKeyValuePair("IPv4", CString(pListener->GetAddrType() != ADDR_IPV6ONLY));
		listenerConfig.AddKeyValuePair("IPv6", CString(pListener->GetAddrType() != ADDR_IPV4ONLY));

		listenerConfig.AddKeyValuePair("SSL", CString(pListener->IsSSL()));

		listenerConfig.AddKeyValuePair("AllowIRC", CString(pListener->GetAcceptType() != CListener::ACCEPT_HTTP));
		listenerConfig.AddKeyValuePair("AllowWeb", CString(pListener->GetAcceptType() != CListener::ACCEPT_IRC));

		config.AddSubConfig("Listener", "listener" + CString(l), listenerConfig);
	}

	config.AddKeyValuePair("ConnectDelay", CString(m_uiConnectDelay));
	config.AddKeyValuePair("ServerThrottle", CString(m_sConnectThrottle.GetTTL()/1000));

	if (!m_sPidFile.empty()) {
		config.AddKeyValuePair("PidFile", m_sPidFile.FirstLine());
	}

	if (!m_sSkinName.empty()) {
		config.AddKeyValuePair("Skin", m_sSkinName.FirstLine());
	}

	if (!m_sStatusPrefix.empty()) {
		config.AddKeyValuePair("StatusPrefix", m_sStatusPrefix.FirstLine());
	}

	if (!m_sSSLCiphers.empty()) {
		config.AddKeyValuePair("SSLCiphers", CString(m_sSSLCiphers));
	}

	if (!m_sSSLProtocols.empty()) {
		config.AddKeyValuePair("SSLProtocols", m_sSSLProtocols);
	}

	for (unsigned int m = 0; m < m_vsMotd.size(); m++) {
		config.AddKeyValuePair("Motd", m_vsMotd[m].FirstLine());
	}

	for (unsigned int v = 0; v < m_vsBindHosts.size(); v++) {
		config.AddKeyValuePair("BindHost", m_vsBindHosts[v].FirstLine());
	}

	for (unsigned int v = 0; v < m_vsTrustedProxies.size(); v++) {
		config.AddKeyValuePair("TrustedProxy", m_vsTrustedProxies[v].FirstLine());
	}

	CModules& Mods = GetModules();

	for (unsigned int a = 0; a < Mods.size(); a++) {
		CString sName = Mods[a]->GetModName();
		CString sArgs = Mods[a]->GetArgs();
		
#ifdef WIN_MSVC
		if (sName == "win32_service_helper")
		{
			continue;
		}
#endif

		if (!sArgs.empty()) {
			sArgs = " " + sArgs.FirstLine();
		}

		config.AddKeyValuePair("LoadModule", sName.FirstLine() + sArgs);
	}

	for (map<CString,CUser*>::iterator it = m_msUsers.begin(); it != m_msUsers.end(); ++it) {
		CString sErr;

		if (!it->second->IsValid(sErr)) {
			DEBUG("** Error writing config for user [" << it->first << "] [" << sErr << "]");
			continue;
		}

		config.AddSubConfig("User", it->second->GetUserName(), it->second->ToConfig());
	}

	config.Write(*pFile);

	// If Sync() fails... well, let's hope nothing important breaks..
	pFile->Sync();

	if (pFile->HadError()) {
		DEBUG("Error while writing the config, errno says: " + CString(strerror(errno)));
		pFile->Delete();
		delete pFile;
		return false;
	}

	// We wrote to a temporary name, move it to the right place
#ifdef _WIN32
	// on win32, we need to close the ~ temp file first.
	pFile->Close();
	// we also need to close the file we are going to overwrite:
	m_pLockFile->Close();
#endif

	if (!pFile->Move(GetConfigFile(), true)) {
		DEBUG("Error while replacing the config file with a new version, errno says " << strerror(errno));
		pFile->Delete();
		delete pFile;
		return false;
	}

#ifdef _WIN32
	// re-open config to keep it from being deleted.
	// aka restore state before entering this function.
	pFile->Open(GetConfigFile());
#endif

	// Everything went fine, just need to update the saved path.
	pFile->SetFileName(GetConfigFile());

	// Make sure the lock is kept alive as long as we need it.
	delete m_pLockFile;
	m_pLockFile = pFile;

	return true;
}

CString CZNC::MakeConfigHeader() {
	return
		"// WARNING\n"
		"//\n"
		"// Do NOT edit this file while ZNC is running!\n"
		"// Use webadmin or *controlpanel instead.\n"
		"//\n"
		"// Altering this file by hand will forfeit all support.\n"
		"//\n"
		"// But if you feel risky, you might want to read help on /znc saveconfig and /znc rehash.\n"
		"// Also check http://en.znc.in/wiki/Configuration\n";
}

bool CZNC::WriteNewConfig(const CString& sConfigFile) {
	CString sAnswer, sUser, sNetwork;
	VCString vsLines;

	vsLines.push_back(MakeConfigHeader());
	vsLines.push_back("Version = " + CString(VERSION_STR));

	m_sConfigFile = ExpandConfigPath(sConfigFile);

	if (CFile::Exists(m_sConfigFile)) {
		CUtils::PrintStatus(false, "WARNING: config [" + m_sConfigFile + "] already exists.");
	}

	CUtils::PrintMessage("");
	CUtils::PrintMessage("-- Global settings --");
	CUtils::PrintMessage("");

	// Listen
#ifdef HAVE_IPV6
	bool b6 = true;
#else
	bool b6 = false;
#endif
	CString sListenHost;
	CString sURIPrefix;
	bool bListenSSL = false;
	unsigned int uListenPort = 0;
	bool bSuccess;

	do {
		bSuccess = true;
		while (true) {
			if (!CUtils::GetNumInput("Listen on port", uListenPort, 1025, 65534)) {
				continue;
			}
			if (uListenPort == 6667) {
				CUtils::PrintStatus(false, "WARNING: Some web browsers reject port 6667. If you intend to");
				CUtils::PrintStatus(false, "use ZNC's web interface, you might want to use another port.");
				if (!CUtils::GetBoolInput("Proceed with port 6667 anyway?", true)) {
					continue;
				}
			}
			break;
		}


#ifdef HAVE_LIBSSL
		bListenSSL = CUtils::GetBoolInput("Listen using SSL", bListenSSL);
#endif

#ifdef HAVE_IPV6
		b6 = CUtils::GetBoolInput("Listen using both IPv4 and IPv6", b6);
#endif

		// Don't ask for listen host, it may be configured later if needed.

		CUtils::PrintAction("Verifying the listener");
		CListener* pListener = new CListener((unsigned short int)uListenPort, sListenHost, sURIPrefix, bListenSSL,
				b6 ? ADDR_ALL : ADDR_IPV4ONLY, CListener::ACCEPT_ALL);
		if (!pListener->Listen()) {
			CUtils::PrintStatus(false, FormatBindError());
			bSuccess = false;
		} else
			CUtils::PrintStatus(true);
		delete pListener;
	} while (!bSuccess);

#ifdef HAVE_LIBSSL
	CString sPemFile = GetPemLocation();
	if (!CFile::Exists(sPemFile)) {
		CUtils::PrintMessage("Unable to locate pem file: [" + sPemFile + "], creating it");
		WritePemFile();
	}
#endif

	vsLines.push_back("<Listener l>");
	vsLines.push_back("\tPort = " + CString(uListenPort));
	vsLines.push_back("\tIPv4 = true");
	vsLines.push_back("\tIPv6 = " + CString(b6));
	vsLines.push_back("\tSSL = " + CString(bListenSSL));
	if (!sListenHost.empty()) {
		vsLines.push_back("\tHost = " + sListenHost);
	}
	vsLines.push_back("</Listener>");
	// !Listen

	set<CModInfo> ssGlobalMods;
	GetModules().GetDefaultMods(ssGlobalMods, CModInfo::GlobalModule);
	vector<CString> vsGlobalModNames;
	for (set<CModInfo>::const_iterator it = ssGlobalMods.begin(); it != ssGlobalMods.end(); ++it) {
		vsGlobalModNames.push_back(it->GetName());
		vsLines.push_back("LoadModule = " + it->GetName());
	}
	CUtils::PrintMessage("Enabled global modules [" + CString(", ").Join(vsGlobalModNames.begin(), vsGlobalModNames.end()) + "]");

	// User
	CUtils::PrintMessage("");
	CUtils::PrintMessage("-- Admin user settings --");
	CUtils::PrintMessage("");

	vsLines.push_back("");
	CString sNick;
	do {
		CUtils::GetInput("Username", sUser, "", "alphanumeric");
	} while (!CUser::IsValidUserName(sUser));

	vsLines.push_back("<User " + sUser + ">");
	CString sSalt;
	sAnswer = CUtils::GetSaltedHashPass(sSalt);
	vsLines.push_back("\tPass       = " + CUtils::sDefaultHash + "#" + sAnswer + "#" + sSalt + "#");

	vsLines.push_back("\tAdmin      = true");

	CUtils::GetInput("Nick", sNick, CUser::MakeCleanUserName(sUser));
	vsLines.push_back("\tNick       = " + sNick);
	CUtils::GetInput("Alternate nick", sAnswer, sNick + "_");
	if (!sAnswer.empty()) {
		vsLines.push_back("\tAltNick    = " + sAnswer);
	}
	CUtils::GetInput("Ident", sAnswer, sUser);
	vsLines.push_back("\tIdent      = " + sAnswer);
	CUtils::GetInput("Real name", sAnswer, "Got ZNC?");
	vsLines.push_back("\tRealName   = " + sAnswer);
	CUtils::GetInput("Bind host", sAnswer, "", "optional");
	if (!sAnswer.empty()) {
		vsLines.push_back("\tBindHost   = " + sAnswer);
	}

	set<CModInfo> ssUserMods;
	GetModules().GetDefaultMods(ssUserMods, CModInfo::UserModule);
	vector<CString> vsUserModNames;
	for (set<CModInfo>::const_iterator it = ssUserMods.begin(); it != ssUserMods.end(); ++it) {
		vsUserModNames.push_back(it->GetName());
		vsLines.push_back("\tLoadModule = " + it->GetName());
	}
	CUtils::PrintMessage("Enabled user modules [" + CString(", ").Join(vsUserModNames.begin(), vsUserModNames.end()) + "]");

	CUtils::PrintMessage("");
	if (CUtils::GetBoolInput("Set up a network?", true)) {
		vsLines.push_back("");

		CUtils::PrintMessage("");
		CUtils::PrintMessage("-- Network settings --");
		CUtils::PrintMessage("");

		do {
			CUtils::GetInput("Name", sNetwork, "freenode");
		} while (!CIRCNetwork::IsValidNetwork(sNetwork));

		vsLines.push_back("\t<Network " + sNetwork + ">");

		set<CModInfo> ssNetworkMods;
		GetModules().GetDefaultMods(ssNetworkMods, CModInfo::NetworkModule);
		vector<CString> vsNetworkModNames;
		for (set<CModInfo>::const_iterator it = ssNetworkMods.begin(); it != ssNetworkMods.end(); ++it) {
			vsNetworkModNames.push_back(it->GetName());
			vsLines.push_back("\t\tLoadModule = " + it->GetName());
		}

		CString sHost, sPass, sHint;
		bool bSSL = false;
		unsigned int uServerPort = 0;

		if (sNetwork.Equals("freenode")) {
			sHost = "chat.freenode.net";
#ifdef HAVE_LIBSSL
			bSSL = true;
#endif
		} else {
			sHint = "host only";
		}

		while (!CUtils::GetInput("Server host", sHost, sHost, sHint) || !CServer::IsValidHostName(sHost));
#ifdef HAVE_LIBSSL
		bSSL = CUtils::GetBoolInput("Server uses SSL?", bSSL);
#endif
		while (!CUtils::GetNumInput("Server port", uServerPort, 1, 65535, bSSL ? 6697 : 6667));
		CUtils::GetInput("Server password (probably empty)", sPass);

		vsLines.push_back("\t\tServer     = " + sHost + ((bSSL) ? " +" : " ") + CString(uServerPort) + " " + sPass);

		CString sChans;
		if (CUtils::GetInput("Initial channels", sChans)) {
			vsLines.push_back("");
			VCString vsChans;
			sChans.Replace(",", " ");
			sChans.Replace(";", " ");
			sChans.Split(" ", vsChans, false, "", "", true, true);
			for (const CString& sChan : vsChans) {
				vsLines.push_back("\t\t<Chan " + sChan + ">");
				vsLines.push_back("\t\t</Chan>");
			}
		}

		CUtils::PrintMessage("Enabled network modules [" + CString(", ").Join(vsNetworkModNames.begin(), vsNetworkModNames.end()) + "]");

		vsLines.push_back("\t</Network>");
	}

	vsLines.push_back("</User>");

	CUtils::PrintMessage("");
	// !User

	CFile File;
	bool bFileOK, bFileOpen = false;
	do {
		CUtils::PrintAction("Writing config [" + m_sConfigFile + "]");

		bFileOK = true;
		if (CFile::Exists(m_sConfigFile)) {
			if (!File.TryExLock(m_sConfigFile)) {
				CUtils::PrintStatus(false, "ZNC is currently running on this config.");
				bFileOK = false;
			} else {
				File.Close();
				CUtils::PrintStatus(false, "This config already exists.");
				if (CUtils::GetBoolInput("Are you sure you want to overwrite it?", false))
					CUtils::PrintAction("Overwriting config [" + m_sConfigFile + "]");
				else
					bFileOK = false;
			}
		}

		if (bFileOK) {
			File.SetFileName(m_sConfigFile);
			if (File.Open(O_WRONLY | O_CREAT | O_TRUNC, 0600)) {
				bFileOpen = true;
			} else {
				CUtils::PrintStatus(false, "Unable to open file");
				bFileOK = false;
			}
		}
		if (!bFileOK) {
			while (!CUtils::GetInput("Please specify an alternate location", m_sConfigFile, "",  "or \"stdout\" for displaying the config"));
			if (m_sConfigFile.Equals("stdout"))
				bFileOK = true;
			else
				m_sConfigFile = ExpandConfigPath(m_sConfigFile);
		}
	} while (!bFileOK);

	if (!bFileOpen) {
		CUtils::PrintMessage("");
		CUtils::PrintMessage("Printing the new config to stdout:");
		CUtils::PrintMessage("");
		cout << endl << "----------------------------------------------------------------------------" << endl << endl;
	}

	for (unsigned int a = 0; a < vsLines.size(); a++) {
		if (bFileOpen) {
			File.Write(vsLines[a] + "\n");
		} else {
			cout << vsLines[a] << endl;
		}
	}

	if (bFileOpen) {
		File.Close();
		if (File.HadError())
			CUtils::PrintStatus(false, "There was an error while writing the config");
		else
			CUtils::PrintStatus(true);
	} else {
		cout << endl << "----------------------------------------------------------------------------" << endl << endl;
	}

	if (File.HadError()) {
		bFileOpen = false;
		CUtils::PrintMessage("Printing the new config to stdout instead:");
		cout << endl << "----------------------------------------------------------------------------" << endl << endl;
		for (unsigned int a = 0; a < vsLines.size(); a++) {
			cout << vsLines[a] << endl;
		}
		cout << endl << "----------------------------------------------------------------------------" << endl << endl;
	}

	const CString sProtocol(bListenSSL ? "https" : "http");
	const CString sSSL(bListenSSL ? "+" : "");
	CUtils::PrintMessage("");
	CUtils::PrintMessage("To connect to this ZNC you need to connect to it as your IRC server", true);
	CUtils::PrintMessage("using the port that you supplied.  You have to supply your login info", true);
	CUtils::PrintMessage("as the IRC server password like this: user/network:pass.", true);
	CUtils::PrintMessage("");
	CUtils::PrintMessage("Try something like this in your IRC client...", true);
	CUtils::PrintMessage("/server <znc_server_ip> " + sSSL + CString(uListenPort) + " " + sUser + ":<pass>", true);
	CUtils::PrintMessage("");
	CUtils::PrintMessage("To manage settings, users and networks, point your web browser to", true);
	CUtils::PrintMessage(sProtocol + "://<znc_server_ip>:" + CString(uListenPort) + "/", true);
	CUtils::PrintMessage("");

	File.UnLock();
	return bFileOpen && CUtils::GetBoolInput("Launch ZNC now?", true);
}

void CZNC::BackupConfigOnce(const CString& sSuffix) {
	static bool didBackup = false;
	if (didBackup)
		return;
	didBackup = true;

	CUtils::PrintAction("Creating a config backup");

	CString sBackup = CDir::ChangeDir(m_sConfigFile, "../znc.conf." + sSuffix);
	if (CFile::Copy(m_sConfigFile, sBackup))
		CUtils::PrintStatus(true, sBackup);
	else
		CUtils::PrintStatus(false, strerror(errno));
}

bool CZNC::ParseConfig(const CString& sConfig, CString& sError)
{
	m_sConfigFile = ExpandConfigPath(sConfig, false);

	return DoRehash(sError);
}

bool CZNC::RehashConfig(CString& sError)
{
	ALLMODULECALL(OnPreRehash(), NOTHING);

	// This clears m_msDelUsers
	HandleUserDeletion();

	// Mark all users as going-to-be deleted
	m_msDelUsers = m_msUsers;
	m_msUsers.clear();

	if (DoRehash(sError)) {
		ALLMODULECALL(OnPostRehash(), NOTHING);

		return true;
	}

	// Rehashing failed, try to recover
	CString s;
	while (!m_msDelUsers.empty()) {
		AddUser(m_msDelUsers.begin()->second, s);
		m_msDelUsers.erase(m_msDelUsers.begin());
	}

	return false;
}

bool CZNC::DoRehash(CString& sError)
{
	sError.clear();

	CUtils::PrintAction("Opening config [" + m_sConfigFile + "]");

	if (!CFile::Exists(m_sConfigFile)) {
		sError = "No such file";
		CUtils::PrintStatus(false, sError);
		CUtils::PrintMessage("Restart ZNC with the --makeconf option if you wish to create this config.");
		return false;
	}

	if (!CFile::IsReg(m_sConfigFile)) {
		sError = "Not a file";
		CUtils::PrintStatus(false, sError);
		return false;
	}

	CFile *pFile = new CFile(m_sConfigFile);

	// need to open the config file Read/Write for fcntl()
	// exclusive locking to work properly!
	if (!pFile->Open(m_sConfigFile, O_RDWR)) {
		sError = "Can not open config file";
		CUtils::PrintStatus(false, sError);
		delete pFile;
		return false;
	}

	if (!pFile->TryExLock()) {
		sError = "ZNC is already running on this config.";
		CUtils::PrintStatus(false, sError);
		delete pFile;
		return false;
	}

	// (re)open the config file
	delete m_pLockFile;
	m_pLockFile = pFile;
	CFile &File = *pFile;

	CConfig config;
	if (!config.Parse(File, sError)) {
		CUtils::PrintStatus(false, sError);
		return false;
	}
	CUtils::PrintStatus(true);

	CString sSavedVersion;
	config.FindStringEntry("version", sSavedVersion);
	tuple<unsigned int, unsigned int> tSavedVersion = make_tuple(sSavedVersion.Token(0, false, ".").ToUInt(),
																 sSavedVersion.Token(1, false, ".").ToUInt());
	tuple<unsigned int, unsigned int> tCurrentVersion = make_tuple(VERSION_MAJOR, VERSION_MINOR);
	if (tSavedVersion < tCurrentVersion) {
		if (sSavedVersion.empty()) {
			sSavedVersion = "< 0.203";
		}
		CUtils::PrintMessage("Found old config from ZNC " + sSavedVersion + ". Saving a backup of it.");
		BackupConfigOnce("pre-" + CString(VERSION_STR));
	} else if (tSavedVersion > tCurrentVersion) {
		CUtils::PrintError("Config was saved from ZNC " + sSavedVersion + ". It may or may not work with current ZNC " + GetVersion());
	}

	m_vsBindHosts.clear();
	m_vsTrustedProxies.clear();
	m_vsMotd.clear();

	// Delete all listeners
	while (!m_vpListeners.empty()) {
		delete m_vpListeners[0];
		m_vpListeners.erase(m_vpListeners.begin());
	}

	MCString msModules;          // Modules are queued for later loading

	VCString vsList;
	VCString::const_iterator vit;
	config.FindStringVector("loadmodule", vsList);
#ifdef _WIN32
#ifndef _DEBUG
	if (CZNCWin32Helpers::IsWindowsService() &&
#else
	if (
#endif
		std::find(vsList.begin(), vsList.end(), "win32_service_helper") == vsList.end())
	{
		CString sModPath, sDataPath;
		if (CModules::FindModPath("win32_service_helper", sModPath, sDataPath) != NULL)
		{
			vsList.push_back("win32_service_helper");
		}
	}
#endif
	for (vit = vsList.begin(); vit != vsList.end(); ++vit) {
		CString sModName = vit->Token(0);
		CString sArgs = vit->Token(1, true);

		if (sModName == "saslauth" && tSavedVersion < make_tuple(0ul, 207ul)) {
			// XXX compatibility crap, added in 0.207
			CUtils::PrintMessage("saslauth module was renamed to cyrusauth. Loading cyrusauth instead.");
			sModName = "cyrusauth";
		}

		if (msModules.find(sModName) != msModules.end()) {
			sError = "Module [" + sModName +
				"] already loaded";
			CUtils::PrintError(sError);
			return false;
		}
		CString sModRet;
		CModule *pOldMod;

		pOldMod = GetModules().FindModule(sModName);
		if (!pOldMod) {
			CUtils::PrintAction("Loading global module [" + sModName + "]");

			bool bModRet = GetModules().LoadModule(sModName, sArgs, CModInfo::GlobalModule, NULL, NULL, sModRet);

			CUtils::PrintStatus(bModRet, sModRet);
			if (!bModRet) {
				sError = sModRet;
				return false;
			}
		} else if (pOldMod->GetArgs() != sArgs) {
			CUtils::PrintAction("Reloading global module [" + sModName + "]");

			bool bModRet = GetModules().ReloadModule(sModName, sArgs, NULL, NULL, sModRet);

			CUtils::PrintStatus(bModRet, sModRet);
			if (!bModRet) {
				sError = sModRet;
				return false;
			}
		} else
			CUtils::PrintMessage("Module [" + sModName + "] already loaded.");

		msModules[sModName] = sArgs;
	}

	CString sISpoofFormat, sISpoofFile;
	config.FindStringEntry("ispoofformat", sISpoofFormat);
	config.FindStringEntry("ispooffile", sISpoofFile);
	if (!sISpoofFormat.empty() || !sISpoofFile.empty()) {
		CModule *pIdentFileMod = GetModules().FindModule("identfile");
		if (!pIdentFileMod) {
			CUtils::PrintAction("Loading global Module [identfile]");

			CString sModRet;
			bool bModRet = GetModules().LoadModule("identfile", "", CModInfo::GlobalModule, NULL, NULL, sModRet);

			CUtils::PrintStatus(bModRet, sModRet);
			if (!bModRet) {
				sError = sModRet;
				return false;
			}

			pIdentFileMod = GetModules().FindModule("identfile");
			msModules["identfile"] = "";
		}

		pIdentFileMod->SetNV("File", sISpoofFile);
		pIdentFileMod->SetNV("Format", sISpoofFormat);
	}

	config.FindStringVector("motd", vsList);
	for (vit = vsList.begin(); vit != vsList.end(); ++vit) {
		AddMotd(*vit);
	}

	config.FindStringVector("bindhost", vsList);
	for (vit = vsList.begin(); vit != vsList.end(); ++vit) {
		AddBindHost(*vit);
	}

	config.FindStringVector("trustedproxy", vsList);
	for (vit = vsList.begin(); vit != vsList.end(); ++vit) {
		AddTrustedProxy(*vit);
	}

	config.FindStringVector("vhost", vsList);
	for (vit = vsList.begin(); vit != vsList.end(); ++vit) {
		AddBindHost(*vit);
	}

	CString sVal;
	if (config.FindStringEntry("pidfile", sVal))
		m_sPidFile = sVal;
	if (config.FindStringEntry("statusprefix", sVal))
		m_sStatusPrefix = sVal;
	if (config.FindStringEntry("sslcertfile", sVal))
		m_sSSLCertFile = sVal;
	if (config.FindStringEntry("sslciphers", sVal))
		m_sSSLCiphers = sVal;
	if (config.FindStringEntry("skin", sVal))
		SetSkinName(sVal);
	if (config.FindStringEntry("connectdelay", sVal))
		SetConnectDelay(sVal.ToUInt());
	if (config.FindStringEntry("serverthrottle", sVal))
		m_sConnectThrottle.SetTTL(sVal.ToUInt() * 1000);
	if (config.FindStringEntry("anoniplimit", sVal))
		m_uiAnonIPLimit = sVal.ToUInt();
	if (config.FindStringEntry("maxbuffersize", sVal))
		m_uiMaxBufferSize = sVal.ToUInt();
	if (config.FindStringEntry("protectwebsessions", sVal))
  		m_bProtectWebSessions = sVal.ToBool();
	if (config.FindStringEntry("hideversion", sVal))
		m_bHideVersion = sVal.ToBool();

	if (config.FindStringEntry("sslprotocols", m_sSSLProtocols)) {
		VCString vsProtocols;
		m_sSSLProtocols.Split(" ", vsProtocols, false, "", "", true, true);

		for (CString& sProtocol : vsProtocols) {

			unsigned int uFlag = 0;
			bool bEnable = sProtocol.TrimPrefix("+");
			bool bDisable = sProtocol.TrimPrefix("-");

			if (sProtocol.Equals("All")) {
				uFlag = ~0;
			} else if (sProtocol.Equals("SSLv2")) {
				uFlag = Csock::EDP_SSLv2;
			} else if (sProtocol.Equals("SSLv3")) {
				uFlag = Csock::EDP_SSLv3;
			} else if (sProtocol.Equals("TLSv1")) {
				uFlag = Csock::EDP_TLSv1;
			} else if (sProtocol.Equals("TLSv1.1")) {
				uFlag = Csock::EDP_TLSv1_1;
			} else if (sProtocol.Equals("TLSv1.2")) {
				uFlag = Csock::EDP_TLSv1_2;
			} else {
				CUtils::PrintError("Invalid SSLProtocols value [" + sProtocol + "]");
				CUtils::PrintError("The syntax is [SSLProtocols = [+|-]<protocol> ...]");
				CUtils::PrintError("Available protocols are [SSLv2, SSLv3, TLSv1, TLSv1.1, TLSv1.2]");
				return false;
			}

			if (bEnable) {
				m_uDisabledSSLProtocols &= ~uFlag;
			} else if (bDisable) {
				m_uDisabledSSLProtocols |= uFlag;
			} else {
				m_uDisabledSSLProtocols = ~uFlag;
			}
		}
	}

	// This has to be after SSLCertFile is handled since it uses that value
	const char *szListenerEntries[] = {
		"listen", "listen6", "listen4",
		"listener", "listener6", "listener4"
	};
	const size_t numListenerEntries = sizeof(szListenerEntries) / sizeof(szListenerEntries[0]);

	for (size_t i = 0; i < numListenerEntries; i++) {
		config.FindStringVector(szListenerEntries[i], vsList);
		vit = vsList.begin();

		for (; vit != vsList.end(); ++vit) {
			if (!AddListener(szListenerEntries[i] + CString(" ") + *vit, sError))
				return false;
		}
	}

	CConfig::SubConfig subConf;
	CConfig::SubConfig::const_iterator subIt;

	config.FindSubConfig("listener", subConf);
	for (subIt = subConf.begin(); subIt != subConf.end(); ++subIt) {
		CConfig* pSubConf = subIt->second.m_pSubConfig;
		if (!AddListener(pSubConf, sError))
			return false;
		if (!pSubConf->empty()) {
			sError = "Unhandled lines in Listener config!";
			CUtils::PrintError(sError);

			CZNC::DumpConfig(pSubConf);
			return false;
		}
	}

	config.FindSubConfig("user", subConf);
	for (subIt = subConf.begin(); subIt != subConf.end(); ++subIt) {
		const CString& sUserName = subIt->first;
		CConfig* pSubConf = subIt->second.m_pSubConfig;
		CUser* pRealUser = NULL;

		CUtils::PrintMessage("Loading user [" + sUserName + "]");

		// Either create a CUser* or use an existing one
		map<CString, CUser*>::iterator it = m_msDelUsers.find(sUserName);

		if (it != m_msDelUsers.end()) {
			pRealUser = it->second;
			m_msDelUsers.erase(it);
		}

		CUser* pUser = new CUser(sUserName);

		if (!m_sStatusPrefix.empty()) {
			if (!pUser->SetStatusPrefix(m_sStatusPrefix)) {
				sError = "Invalid StatusPrefix [" + m_sStatusPrefix + "] Must be 1-5 chars, no spaces.";
				CUtils::PrintError(sError);
				return false;
			}
		}

		if (!pUser->ParseConfig(pSubConf, sError)) {
			CUtils::PrintError(sError);
			delete pUser;
			pUser = NULL;
			return false;
		}

		if (!pSubConf->empty()) {
			sError = "Unhandled lines in config for User [" + sUserName + "]!";
			CUtils::PrintError(sError);

			DumpConfig(pSubConf);
			return false;
		}

		CString sErr;
		if (pRealUser) {
			if (!pRealUser->Clone(*pUser, sErr)
					|| !AddUser(pRealUser, sErr)) {
				sError = "Invalid user [" + pUser->GetUserName() + "] " + sErr;
				DEBUG("CUser::Clone() failed in rehash");
			}
			pUser->SetBeingDeleted(true);
			delete pUser;
			pUser = NULL;
		} else if (!AddUser(pUser, sErr)) {
			sError = "Invalid user [" + pUser->GetUserName() + "] " + sErr;
		}

		if (!sError.empty()) {
			CUtils::PrintError(sError);
			if (pUser) {
				pUser->SetBeingDeleted(true);
				delete pUser;
				pUser = NULL;
			}
			return false;
		}

		pUser = NULL;
		pRealUser = NULL;
	}

	if (!config.empty()) {
		sError = "Unhandled lines in config!";
		CUtils::PrintError(sError);

		DumpConfig(&config);
		return false;
	}


	// Unload modules which are no longer in the config
	set<CString> ssUnload;
	for (size_t i = 0; i < GetModules().size(); i++) {
		CModule *pCurMod = GetModules()[i];

		if (msModules.find(pCurMod->GetModName()) == msModules.end())
			ssUnload.insert(pCurMod->GetModName());
	}

	for (set<CString>::iterator it = ssUnload.begin(); it != ssUnload.end(); ++it) {
		if (GetModules().UnloadModule(*it))
			CUtils::PrintMessage("Unloaded global module [" + *it + "]");
		else
			CUtils::PrintMessage("Could not unload [" + *it + "]");
	}

	if (m_msUsers.empty()) {
		sError = "You must define at least one user in your config.";
		CUtils::PrintError(sError);
		return false;
	}

	if (m_vpListeners.empty()) {
		sError = "You must supply at least one Listen port in your config.";
		CUtils::PrintError(sError);
		return false;
	}

	return true;
}

void CZNC::DumpConfig(const CConfig* pConfig) {
	CConfig::EntryMapIterator eit = pConfig->BeginEntries();
	for (; eit != pConfig->EndEntries(); ++eit) {
		const CString& sKey = eit->first;
		const VCString& vsList = eit->second;
		VCString::const_iterator it = vsList.begin();
		for (; it != vsList.end(); ++it) {
			CUtils::PrintError(sKey + " = " + *it);
		}
	}

	CConfig::SubConfigMapIterator sit = pConfig->BeginSubConfigs();
	for (; sit != pConfig->EndSubConfigs(); ++sit) {
		const CString& sKey = sit->first;
		const CConfig::SubConfig& sSub = sit->second;
		CConfig::SubConfig::const_iterator it = sSub.begin();

		for (; it != sSub.end(); ++it) {
			CUtils::PrintError("SubConfig [" + sKey + " " + it->first + "]:");
			DumpConfig(it->second.m_pSubConfig);
		}
	}
}

void CZNC::ClearBindHosts() {
	m_vsBindHosts.clear();
}

bool CZNC::AddBindHost(const CString& sHost) {
	if (sHost.empty()) {
		return false;
	}

	for (unsigned int a = 0; a < m_vsBindHosts.size(); a++) {
		if (m_vsBindHosts[a].Equals(sHost)) {
			return false;
		}
	}

	m_vsBindHosts.push_back(sHost);
	return true;
}

bool CZNC::RemBindHost(const CString& sHost) {
	VCString::iterator it;
	for (it = m_vsBindHosts.begin(); it != m_vsBindHosts.end(); ++it) {
		if (sHost.Equals(*it)) {
			m_vsBindHosts.erase(it);
			return true;
		}
	}

	return false;
}

void CZNC::ClearTrustedProxies() {
	m_vsTrustedProxies.clear();
}

bool CZNC::AddTrustedProxy(const CString& sHost) {
	if (sHost.empty()) {
		return false;
	}

	for (unsigned int a = 0; a < m_vsTrustedProxies.size(); a++) {
		if (m_vsTrustedProxies[a].Equals(sHost)) {
			return false;
		}
	}

	m_vsTrustedProxies.push_back(sHost);
	return true;
}

bool CZNC::RemTrustedProxy(const CString& sHost) {
	VCString::iterator it;
	for (it = m_vsTrustedProxies.begin(); it != m_vsTrustedProxies.end(); ++it) {
		if (sHost.Equals(*it)) {
			m_vsTrustedProxies.erase(it);
			return true;
		}
	}

	return false;
}

void CZNC::Broadcast(const CString& sMessage, bool bAdminOnly,
		CUser* pSkipUser, CClient *pSkipClient) {
	for (map<CString,CUser*>::iterator a = m_msUsers.begin(); a != m_msUsers.end(); ++a) {
		if (bAdminOnly && !a->second->IsAdmin())
			continue;

		if (a->second != pSkipUser) {
			CString sMsg = sMessage;

			bool bContinue = false;
			USERMODULECALL(OnBroadcast(sMsg), a->second, NULL, &bContinue);
			if (bContinue) continue;

			a->second->PutStatusNotice("*** " + sMsg, NULL, pSkipClient);
		}
	}
}

CModule* CZNC::FindModule(const CString& sModName, const CString& sUsername) {
	if (sUsername.empty()) {
		return CZNC::Get().GetModules().FindModule(sModName);
	}

	CUser* pUser = FindUser(sUsername);

	return (!pUser) ? NULL : pUser->GetModules().FindModule(sModName);
}

CModule* CZNC::FindModule(const CString& sModName, CUser* pUser) {
	if (pUser) {
		return pUser->GetModules().FindModule(sModName);
	}

	return CZNC::Get().GetModules().FindModule(sModName);
}

bool CZNC::UpdateModule(const CString &sModule) {
	CModule *pModule;

	map<CString,CUser*>::const_iterator it;
	map<CUser*, CString> musLoaded;
	map<CUser*, CString>::iterator musIt;
	map<CIRCNetwork*, CString> mnsLoaded;
	map<CIRCNetwork*, CString>::iterator mnsIt;

	// Unload the module for every user and network
	for (it = m_msUsers.begin(); it != m_msUsers.end(); ++it) {
		CUser *pUser = it->second;

		pModule = pUser->GetModules().FindModule(sModule);
		if (pModule) {
			musLoaded[pUser] = pModule->GetArgs();
			pUser->GetModules().UnloadModule(sModule);
		}

		// See if the user has this module loaded to a network
		vector<CIRCNetwork*> vNetworks = pUser->GetNetworks();
		vector<CIRCNetwork*>::iterator it2;
		for (it2 = vNetworks.begin(); it2 != vNetworks.end(); ++it2) {
			CIRCNetwork *pNetwork = *it2;

			pModule = pNetwork->GetModules().FindModule(sModule);
			if (pModule) {
				mnsLoaded[pNetwork] = pModule->GetArgs();
				pNetwork->GetModules().UnloadModule(sModule);
			}
		}
	}

	// Unload the global module
	bool bGlobal = false;
	CString sGlobalArgs;

	pModule = GetModules().FindModule(sModule);
	if (pModule) {
		bGlobal = true;
		sGlobalArgs = pModule->GetArgs();
		GetModules().UnloadModule(sModule);
	}

	// Lets reload everything
	bool bError = false;
	CString sErr;

	// Reload the global module
	if (bGlobal) {
		if (!GetModules().LoadModule(sModule, sGlobalArgs, CModInfo::GlobalModule, NULL, NULL, sErr)) {
			DEBUG("Failed to reload [" <<  sModule << "] globally [" << sErr << "]");
			bError = true;
		}
	}

	// Reload the module for all users
	for (musIt = musLoaded.begin(); musIt != musLoaded.end(); ++musIt) {
		CUser *pUser = musIt->first;
		CString& sArgs = musIt->second;

		if (!pUser->GetModules().LoadModule(sModule, sArgs, CModInfo::UserModule, pUser, NULL, sErr)) {
			DEBUG("Failed to reload [" <<  sModule << "] for ["
					<< pUser->GetUserName() << "] [" << sErr << "]");
			bError = true;
		}
	}

	// Reload the module for all networks
	for (mnsIt = mnsLoaded.begin(); mnsIt != mnsLoaded.end(); ++mnsIt) {
		CIRCNetwork *pNetwork = mnsIt->first;
		CString& sArgs = mnsIt->second;

		if (!pNetwork->GetModules().LoadModule(sModule, sArgs, CModInfo::NetworkModule, pNetwork->GetUser(), pNetwork, sErr)) {
			DEBUG("Failed to reload [" <<  sModule << "] for ["
					<< pNetwork->GetUser()->GetUserName() << "/"
					<< pNetwork->GetName() << "] [" << sErr << "]");
			bError = true;
		}
	}

	return !bError;
}

CUser* CZNC::FindUser(const CString& sUsername) {
	map<CString,CUser*>::iterator it = m_msUsers.find(sUsername);

	if (it != m_msUsers.end()) {
		return it->second;
	}

	return NULL;
}

bool CZNC::DeleteUser(const CString& sUsername) {
	CUser* pUser = FindUser(sUsername);

	if (!pUser) {
		return false;
	}

	m_msDelUsers[pUser->GetUserName()] = pUser;
	return true;
}

bool CZNC::AddUser(CUser* pUser, CString& sErrorRet) {
	if (FindUser(pUser->GetUserName()) != NULL) {
		sErrorRet = "User already exists";
		DEBUG("User [" << pUser->GetUserName() << "] - already exists");
		return false;
	}
	if (!pUser->IsValid(sErrorRet)) {
		DEBUG("Invalid user [" << pUser->GetUserName() << "] - ["
				<< sErrorRet << "]");
		return false;
	}
	bool bFailed = false;
	GLOBALMODULECALL(OnAddUser(*pUser, sErrorRet), &bFailed);
	if (bFailed) {
		DEBUG("AddUser [" << pUser->GetUserName() << "] aborted by a module ["
			<< sErrorRet << "]");
		return false;
	}
	m_msUsers[pUser->GetUserName()] = pUser;
	return true;
}

CListener* CZNC::FindListener(u_short uPort, const CString& sBindHost, EAddrType eAddr) {
	vector<CListener*>::iterator it;

	for (it = m_vpListeners.begin(); it < m_vpListeners.end(); ++it) {
		if ((*it)->GetPort() != uPort)
			continue;
		if ((*it)->GetBindHost() != sBindHost)
			continue;
		if ((*it)->GetAddrType() != eAddr)
			continue;
		return *it;
	}
	return NULL;
}

bool CZNC::AddListener(const CString& sLine, CString& sError) {
	CString sName = sLine.Token(0);
	CString sValue = sLine.Token(1, true);

	EAddrType eAddr = ADDR_ALL;
	if (sName.Equals("Listen4") || sName.Equals("Listen") || sName.Equals("Listener4")) {
		eAddr = ADDR_IPV4ONLY;
	}
	if (sName.Equals("Listener6")) {
		eAddr = ADDR_IPV6ONLY;
	}

	CListener::EAcceptType eAccept = CListener::ACCEPT_ALL;
	if (sValue.TrimPrefix("irc_only "))
		eAccept = CListener::ACCEPT_IRC;
	else if (sValue.TrimPrefix("web_only "))
		eAccept = CListener::ACCEPT_HTTP;

	bool bSSL = false;
	CString sPort;
	CString sBindHost;

	if (ADDR_IPV4ONLY == eAddr) {
		sValue.Replace(":", " ");
	}

	if (sValue.find(" ") != CString::npos) {
		sBindHost = sValue.Token(0, false, " ");
		sPort = sValue.Token(1, true, " ");
	} else {
		sPort = sValue;
	}

	if (sPort.Left(1) == "+") {
		sPort.LeftChomp();
		bSSL = true;
	}

	// No support for URIPrefix for old-style configs.
	CString sURIPrefix;
	unsigned short uPort = sPort.ToUShort();
	return AddListener(uPort, sBindHost, sURIPrefix, bSSL, eAddr, eAccept, sError);
}

bool CZNC::AddListener(unsigned short uPort, const CString& sBindHost,
		       const CString& sURIPrefixRaw, bool bSSL,
		       EAddrType eAddr, CListener::EAcceptType eAccept, CString& sError) {
	CString sHostComment;

	if (!sBindHost.empty()) {
		sHostComment = " on host [" + sBindHost + "]";
	}

	CString sIPV6Comment;

	switch (eAddr) {
		case ADDR_ALL:
			sIPV6Comment = "";
			break;
		case ADDR_IPV4ONLY:
			sIPV6Comment = " using ipv4";
			break;
		case ADDR_IPV6ONLY:
			sIPV6Comment = " using ipv6";
	}

	CUtils::PrintAction("Binding to port [" + CString((bSSL) ? "+" : "") + CString(uPort) + "]" + sHostComment + sIPV6Comment);

#ifndef HAVE_IPV6
	if (ADDR_IPV6ONLY == eAddr) {
		sError = "IPV6 is not enabled";
		CUtils::PrintStatus(false, sError);
		return false;
	}
#endif

#ifndef HAVE_LIBSSL
	if (bSSL) {
		sError = "SSL is not enabled";
		CUtils::PrintStatus(false, sError);
		return false;
	}
#else
	CString sPemFile = GetPemLocation();

	if (bSSL && !CFile::Exists(sPemFile)) {
		sError = "Unable to locate pem file: [" + sPemFile + "]";
		CUtils::PrintStatus(false, sError);

		// If stdin is e.g. /dev/null and we call GetBoolInput(),
		// we are stuck in an endless loop!
		if (isatty(0) && CUtils::GetBoolInput("Would you like to create a new pem file?", true)) {
			sError.clear();
			WritePemFile();
		} else {
			return false;
		}

		CUtils::PrintAction("Binding to port [+" + CString(uPort) + "]" + sHostComment + sIPV6Comment);
	}
#endif
	if (!uPort) {
		sError = "Invalid port";
		CUtils::PrintStatus(false, sError);
		return false;
	}

	// URIPrefix must start with a slash and end without one.
	CString sURIPrefix = CString(sURIPrefixRaw);
	if(!sURIPrefix.empty()) {
		if (!sURIPrefix.StartsWith("/")) {
			sURIPrefix = "/" + sURIPrefix;
		}
		if (sURIPrefix.EndsWith("/")) {
			sURIPrefix.TrimRight("/");
		}
	}

	CListener* pListener = new CListener(uPort, sBindHost, sURIPrefix, bSSL, eAddr, eAccept);

	if (!pListener->Listen()) {
		sError = FormatBindError();
		CUtils::PrintStatus(false, sError);
		delete pListener;
		return false;
	}

	m_vpListeners.push_back(pListener);
	CUtils::PrintStatus(true);

	return true;
}

bool CZNC::AddListener(CConfig* pConfig, CString& sError) {
	CString sBindHost;
	CString sURIPrefix;
	bool bSSL;
	bool b4;
#ifdef HAVE_IPV6
	bool b6 = true;
#else
	bool b6 = false;
#endif
	bool bIRC;
	bool bWeb;
	unsigned short uPort;
	if (!pConfig->FindUShortEntry("port", uPort)) {
		sError = "No port given";
		CUtils::PrintError(sError);
		return false;
	}
	pConfig->FindStringEntry("host", sBindHost);
	pConfig->FindBoolEntry("ssl", bSSL, false);
	pConfig->FindBoolEntry("ipv4", b4, true);
	pConfig->FindBoolEntry("ipv6", b6, b6);
	pConfig->FindBoolEntry("allowirc", bIRC, true);
	pConfig->FindBoolEntry("allowweb", bWeb, true);
	pConfig->FindStringEntry("uriprefix", sURIPrefix);

	EAddrType eAddr;
	if (b4 && b6) {
		eAddr = ADDR_ALL;
	} else if (b4 && !b6) {
		eAddr = ADDR_IPV4ONLY;
	} else if (!b4 && b6) {
		eAddr = ADDR_IPV6ONLY;
	} else {
		sError = "No address family given";
		CUtils::PrintError(sError);
		return false;
	}

	CListener::EAcceptType eAccept;
	if (bIRC && bWeb) {
		eAccept = CListener::ACCEPT_ALL;
	} else if (bIRC && !bWeb) {
		eAccept = CListener::ACCEPT_IRC;
	} else if (!bIRC && bWeb) {
		eAccept = CListener::ACCEPT_HTTP;
	} else {
		sError = "Either Web or IRC or both should be selected";
		CUtils::PrintError(sError);
		return false;
	}

	return AddListener(uPort, sBindHost, sURIPrefix, bSSL, eAddr, eAccept, sError);
}

bool CZNC::AddListener(CListener* pListener) {
	if (!pListener->GetRealListener()) {
		// Listener doesnt actually listen
		delete pListener;
		return false;
	}

	// We don't check if there is an identical listener already listening
	// since one can't listen on e.g. the same port multiple times

	m_vpListeners.push_back(pListener);
	return true;
}

bool CZNC::DelListener(CListener* pListener) {
	vector<CListener*>::iterator it;

	for (it = m_vpListeners.begin(); it < m_vpListeners.end(); ++it) {
		if (*it == pListener) {
			m_vpListeners.erase(it);
			delete pListener;
			return true;
		}
	}

	return false;
}

static CZNC* s_pZNC = NULL;

void CZNC::CreateInstance() {
	if (s_pZNC)
		abort();

	s_pZNC = new CZNC();
}

CZNC& CZNC::Get() {
	return *s_pZNC;
}

void CZNC::DestroyInstance() {
	delete s_pZNC;
	s_pZNC = NULL;
}

CZNC::TrafficStatsMap CZNC::GetTrafficStats(TrafficStatsPair &Users,
			TrafficStatsPair &ZNC, TrafficStatsPair &Total) {
	TrafficStatsMap ret;
	unsigned long long uiUsers_in, uiUsers_out, uiZNC_in, uiZNC_out;
	const map<CString, CUser*>& msUsers = CZNC::Get().GetUserMap();

	uiUsers_in = uiUsers_out = 0;
	uiZNC_in  = BytesRead();
	uiZNC_out = BytesWritten();

	for (map<CString, CUser*>::const_iterator it = msUsers.begin(); it != msUsers.end(); ++it) {
		ret[it->first] = TrafficStatsPair(it->second->BytesRead(), it->second->BytesWritten());
		uiUsers_in  += it->second->BytesRead();
		uiUsers_out += it->second->BytesWritten();
	}

	for (CSockManager::const_iterator it = m_Manager.begin(); it != m_Manager.end(); ++it) {
		CUser *pUser = NULL;
		if ((*it)->GetSockName().Left(5) == "IRC::") {
			pUser = ((CIRCSock *) *it)->GetNetwork()->GetUser();
		} else if ((*it)->GetSockName().Left(5) == "USR::") {
			pUser = ((CClient*) *it)->GetUser();
		}

		if (pUser) {
			ret[pUser->GetUserName()].first  += (*it)->GetBytesRead();
			ret[pUser->GetUserName()].second += (*it)->GetBytesWritten();
			uiUsers_in  += (*it)->GetBytesRead();
			uiUsers_out += (*it)->GetBytesWritten();
		} else {
			uiZNC_in  += (*it)->GetBytesRead();
			uiZNC_out += (*it)->GetBytesWritten();
		}
	}

	Users = TrafficStatsPair(uiUsers_in, uiUsers_out);
	ZNC   = TrafficStatsPair(uiZNC_in, uiZNC_out);
	Total = TrafficStatsPair(uiUsers_in + uiZNC_in, uiUsers_out + uiZNC_out);

	return ret;
}

void CZNC::AuthUser(std::shared_ptr<CAuthBase> AuthClass) {
	// TODO unless the auth module calls it, CUser::IsHostAllowed() is not honoured
	bool bReturn = false;
	GLOBALMODULECALL(OnLoginAttempt(AuthClass), &bReturn);
	if (bReturn) return;

	CUser* pUser = FindUser(AuthClass->GetUsername());

	if (!pUser || !pUser->CheckPass(AuthClass->GetPassword())) {
		AuthClass->RefuseLogin("Invalid Password");
		return;
	}

	CString sHost = AuthClass->GetRemoteIP();

	if (!pUser->IsHostAllowed(sHost)) {
		AuthClass->RefuseLogin("Your host [" + sHost + "] is not allowed");
		return;
	}

	AuthClass->AcceptLogin(*pUser);
}

class CConnectQueueTimer : public CCron {
public:
	CConnectQueueTimer(int iSecs) : CCron() {
		SetName("Connect users");
		Start(iSecs);
		// Don't wait iSecs seconds for first timer run
		m_bRunOnNextCall = true;
	}
	virtual ~CConnectQueueTimer() {
		// This is only needed when ZNC shuts down:
		// CZNC::~CZNC() sets its CConnectQueueTimer pointer to NULL and
		// calls the manager's Cleanup() which destroys all sockets and
		// timers. If something calls CZNC::EnableConnectQueue() here
		// (e.g. because a CIRCSock is destroyed), the socket manager
		// deletes that timer almost immediately, but CZNC now got a
		// dangling pointer to this timer which can crash later on.
		//
		// Unlikely but possible ;)
		CZNC::Get().LeakConnectQueueTimer(this);
	}

protected:
	virtual void RunJob() {
		list<CIRCNetwork*> ConnectionQueue;
		list<CIRCNetwork*>& RealConnectionQueue = CZNC::Get().GetConnectionQueue();

		// Problem: If a network can't connect right now because e.g. it
		// is throttled, it will re-insert itself into the connection
		// queue. However, we must only give each network a single
		// chance during this timer run.
		//
		// Solution: We move the connection queue to our local list at
		// the beginning and work from that.
		ConnectionQueue.swap(RealConnectionQueue);

		while (!ConnectionQueue.empty()) {
			CIRCNetwork *pNetwork = ConnectionQueue.front();
			ConnectionQueue.pop_front();

			if (pNetwork->Connect()) {
				break;
			}
		}

		/* Now re-insert anything that is left in our local list into
		 * the real connection queue.
		 */
		RealConnectionQueue.splice(RealConnectionQueue.begin(), ConnectionQueue);

		if (RealConnectionQueue.empty()) {
			DEBUG("ConnectQueueTimer done");
			CZNC::Get().DisableConnectQueue();
		}
	}
};

void CZNC::SetConnectDelay(unsigned int i) {
	if (i < 1) {
		// Don't hammer server with our failed connects
		i = 1;
	}
	if (m_uiConnectDelay != i && m_pConnectQueueTimer != NULL) {
		m_pConnectQueueTimer->Start(i);
	}
	m_uiConnectDelay = i;
}

void CZNC::EnableConnectQueue() {
	if (!m_pConnectQueueTimer && !m_uiConnectPaused && !m_lpConnectQueue.empty()) {
		m_pConnectQueueTimer = new CConnectQueueTimer(m_uiConnectDelay);
		GetManager().AddCron(m_pConnectQueueTimer);
	}
}

void CZNC::DisableConnectQueue() {
	if (m_pConnectQueueTimer) {
		// This will kill the cron
		m_pConnectQueueTimer->Stop();
		m_pConnectQueueTimer = NULL;
	}
}

void CZNC::PauseConnectQueue() {
	DEBUG("Connection queue paused");
	m_uiConnectPaused++;

	if (m_pConnectQueueTimer) {
		m_pConnectQueueTimer->Pause();
	}
}

void CZNC::ResumeConnectQueue() {
	DEBUG("Connection queue resumed");
	m_uiConnectPaused--;

	EnableConnectQueue();
	if (m_pConnectQueueTimer) {
		m_pConnectQueueTimer->UnPause();
	}
}

void CZNC::AddNetworkToQueue(CIRCNetwork *pNetwork) {
	// Make sure we are not already in the queue
	for (list<CIRCNetwork*>::const_iterator it = m_lpConnectQueue.begin(); it != m_lpConnectQueue.end(); ++it) {
		if (*it == pNetwork) {
			return;
		}
	}

	m_lpConnectQueue.push_back(pNetwork);
	EnableConnectQueue();
}

void CZNC::LeakConnectQueueTimer(CConnectQueueTimer *pTimer) {
	if (m_pConnectQueueTimer == pTimer)
		m_pConnectQueueTimer = NULL;
}

bool CZNC::WaitForChildLock() {
	return m_pLockFile && m_pLockFile->ExLock();
}

// it is important that this method doesn't have its body in an .h file.
double CZNC::GetCoreDLLVersion()
{
	return VERSION;
}
