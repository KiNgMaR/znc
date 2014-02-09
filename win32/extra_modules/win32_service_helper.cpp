/*
 * Copyright (C) 2012-2013 ZNC-MSVC
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

#include "znc/znc.h"
#include "znc/User.h"
#include "znc/Modules.h"
#include "znc/IRCNetwork.h"

class CServiceHelperListener;

/************************************************************************/
/* ZNC MODULE CLASS                                                     */
/************************************************************************/

class CServiceHelperMod : public CModule
{
public:
	MODCONSTRUCTOR(CServiceHelperMod)
	{
		m_pServer = NULL;
	}
	virtual ~CServiceHelperMod();

	/* ZNC events */
	bool OnLoad(const CString& sArgsi, CString& sMessage) override;
	EModRet OnStatusCommand(CString& sLine) override;
	void OnModCommand(const CString& sLine) override;

private:
	CServiceHelperListener* m_pServer;

	/* local methods */

};

/************************************************************************/
/* ACCEPTED CONNECTION SOCKET CLASS                                     */
/************************************************************************/

class CServiceHelperConnection : public CSocket
{
public:
	CServiceHelperConnection(CServiceHelperMod *pMod) : CSocket(pMod)
	{
		EnableReadLine();
	}

	void ReadLine(const CS_STRING & sLine) override
	{
		Write("lel\r\n");

		Close(CLT_AFTERWRITE);
	}
};

/************************************************************************/
/* SERVER SOCKET CLASS                                                  */
/************************************************************************/

class CServiceHelperListener : public CSocket
{
private:
	CServiceHelperMod *m_pModule;
	unsigned short m_uPort;

public:
	CServiceHelperListener(CServiceHelperMod *pMod, unsigned short uPort) : CSocket(pMod),
		m_pModule(pMod), m_uPort(uPort)
	{
	}

	bool StartListening()
	{
		return GetModule()->GetManager()->ListenHost(m_uPort, "win32_service_helper",
			"127.0.0.1", false, SOMAXCONN, this);
	}

	bool ConnectionFrom(const CS_STRING & sHostname, u_short uPort) override
	{
		// don't apply max anon connection count logic...
		return true;
	}

	Csock *GetSockObj(const CS_STRING & sHostname, u_short uPort) override;
};

Csock *CServiceHelperListener::GetSockObj(const CS_STRING & sHostname, u_short uPort)
{
	return new CServiceHelperConnection(m_pModule);
}

/************************************************************************/
/* MAIN IMPLEMENTATION LOGIC                                            */
/************************************************************************/

bool CServiceHelperMod::OnLoad(const CString& sArgsi, CString& sMessage)
{
	m_pServer = new CServiceHelperListener(this, CZNCWin32Helpers::GetServiceControlPort());

	return m_pServer->StartListening();
}

CModule::EModRet CServiceHelperMod::OnStatusCommand(CString& sLine)
{
	if(CZNCWin32Helpers::IsWindowsService() && m_pUser && m_pUser->IsAdmin())
	{
		const CString& sCommand = sLine.Token(0);

		if(sCommand.Equals("RESTART"))
		{
			PutStatus("Error: for technical reasons, restarting ZNC when running as a service is not possible. Please restart ZNC via outside means.");
			return HALTCORE;
		}
	}

	return CONTINUE;
}

void CServiceHelperMod::OnModCommand(const CString& sLine)
{
	const CString& sCommand = sLine.Token(0);

	if(sCommand.Equals("HELP"))
	{
		PutModule("No commands available.");
	}
}

CServiceHelperMod::~CServiceHelperMod()
{
	if (m_pServer)
	{
		m_pServer->Close(); // memory is freed by CSocketManager
	}
}

GLOBALMODULEDEFS(CServiceHelperMod, "Communication helper for the ZNC Windows service and GUI.")
