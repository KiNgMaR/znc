/*
* Copyright (C) 2009-2013 Ingmar Runge <ingmar@irsoft.de>
* See the AUTHORS file for details.
*
* An ident server module for Windows ZNC.
* http://code.google.com/p/znc-msvc/
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License version 2 as published
* by the Free Software Foundation.
*/

#include "znc/znc.h"
#include "znc/ZNCString.h"
#include "znc/User.h"
#include "znc/IRCNetwork.h"
#include "znc/IRCSock.h"
#include "znc/Modules.h"
#include <map>
#include <set>

/************************************************************************/
/*   CLASS DECLARATIONS                                                 */
/************************************************************************/
class CIdentServer;

class CIdentServerMod : public CModule
{
protected:
	unsigned short m_serverPort;
	CIdentServer *m_identServer;
	bool m_listenFailed;
	CString m_sLastReply;
	CString m_sLastRequest;

public:
	MODCONSTRUCTOR(CIdentServerMod)
	{
		m_serverPort = 113;
		m_identServer = NULL;
		m_listenFailed = false;
	}
	virtual ~CIdentServerMod();

	EModRet OnIRCConnecting(CIRCSock *pIRCSock) override;
	void OnIRCConnected() override;
	void OnIRCDisconnected() override;
	void OnClientLogin() override;
	EModRet OnDeleteUser(CUser& User) override;
	EModRet OnDeleteNetwork(CIRCNetwork& Network) override;
	void OnModCommand(const CString& sLine) override;

	void NoLongerNeedsIdentServer();

	CIdentServer *GetIdentServer() { return m_identServer; }

	void SetLastReply(const CString& s) { m_sLastReply = s; };
	void SetLastRequest(const CString& s) { m_sLastRequest = s; };
};


class CIdentAcceptedSocket : public CSocket
{
public:
	CIdentAcceptedSocket(CModule *pMod);
	virtual ~CIdentAcceptedSocket();

	void ReadLine(const CS_STRING & sLine) override;
};


/**
* Ident server implementation.
* RFC 1413: http://www.faqs.org/rfcs/rfc1413.html
* Not thread safe!
**/
class CIdentServer : public CSocket
{
protected:
	std::set<CIRCNetwork*> m_activeUsers;
	CModule *m_pModule;
	unsigned short m_uPort;

	static bool AreIPStringsEqual(const CString& sIP1, const CString& sIP2);
public:
	CIdentServer(CModule *pMod, unsigned short uPort);
	virtual ~CIdentServer();

	bool IncreaseUseCount(CIRCNetwork* pUser);
	bool DecreaseUseCount(CIRCNetwork* pUser);
	bool InUse() { return !m_activeUsers.empty(); }
	bool StartListening();

	Csock *GetSockObj(const CS_STRING & sHostname, u_short uPort) override;
	bool ConnectionFrom(const CS_STRING & sHostname, u_short uPort) override;

	CString GetResponse(const CString& sLine, const CString& sSocketIP, const CString& sRemoteIP);
	const std::set<CIRCNetwork*>& GetActiveUsers() { return m_activeUsers; };
};


/************************************************************************/
/* CIdentServer method implementation section                           */
/************************************************************************/

CIdentServer::CIdentServer(CModule *pMod, unsigned short uPort) : CSocket(pMod)
{
	m_pModule = pMod;
	m_uPort = uPort;
}

bool CIdentServer::IncreaseUseCount(CIRCNetwork *pUser)
{
	if(m_activeUsers.find(pUser) != m_activeUsers.end())
	{
		return false;
	}

	m_activeUsers.insert(pUser);

	return true;
}

bool CIdentServer::DecreaseUseCount(CIRCNetwork *pUser)
{
	return (m_activeUsers.erase(pUser) != 0);
}

CString CIdentServer::GetResponse(const CString& sLine, const CString& sSocketIP, const CString& sRemoteIP)
{
	unsigned short uLocalPort = 0; // local port that ZNC connected to IRC FROM
	unsigned short uRemotePort = 0; // remote server port that ZNC connected TO, e.g. 6667

	CString sResponseType = "ERROR";
	CString sAddInfo = "INVALID-PORT";

	DEBUG("IDENT request: " << sLine << " from " << sRemoteIP << " on " << sSocketIP);

	if(sscanf(sLine.c_str(), "%hu , %hu", &uLocalPort, &uRemotePort) == 2)
	{
		sAddInfo = "NO-USER";

		for(auto itu = CZNC::Get().GetUserMap().begin();
			itu != CZNC::Get().GetUserMap().end(); ++itu)
		{
			CUser* pUser = itu->second;
			bool bFound = false;

			for(CIRCNetwork* pNetwork : pUser->GetNetworks())
			{
				CIRCSock *pSock = pNetwork->GetIRCSock();

				if(!pSock)
					continue;

				DEBUG("Checking user (" << pSock->GetLocalPort() << ", " << pSock->GetRemotePort() << ", " << pSock->GetLocalIP() << ")");

				if(pSock->GetLocalPort() == uLocalPort &&
					pSock->GetRemotePort() == uRemotePort &&
					AreIPStringsEqual(pSock->GetLocalIP(), sSocketIP))
				{
					sResponseType = "USERID";
					sAddInfo = "UNIX : " + pUser->GetIdent();
					// exact match found, leave the loop:
					bFound = true;
					break;
				}

				DEBUG("Checking user fallback (" << pSock->GetRemoteIP() << ", " << pSock->GetRemotePort() << ", " << pSock->GetLocalIP() << ")");

				if(pSock->GetRemoteIP() == sRemoteIP &&
					pSock->GetRemotePort() == uRemotePort &&
					AreIPStringsEqual(pSock->GetLocalIP(), sSocketIP))
				{
					sResponseType = "USERID";
					sAddInfo = "UNIX : " + pUser->GetIdent();
					// keep looping, we may find something better
				}
			}

			if(bFound)
				break;
		}
	}

	CString sReply = CString(uLocalPort) + ", " + CString(uRemotePort) + " : " + sResponseType + " : " + sAddInfo;

	DEBUG("IDENT response: " << sReply);

	CIdentServerMod *pMod = reinterpret_cast<CIdentServerMod*>(m_pModule);
	if(pMod)
	{
		pMod->SetLastRequest(sLine.Replace_n("\r", "").Replace_n("\n", " ") + "from " + sRemoteIP + " on " + sSocketIP);
		pMod->SetLastReply(sReply);
	}

	return sReply;
}

bool CIdentServer::StartListening()
{
	return GetModule()->GetManager()->ListenAll(m_uPort, "IDENT_SERVER", false, SOMAXCONN, this);
}

Csock *CIdentServer::GetSockObj(const CS_STRING & sHostname, u_short uPort)
{
	return new CIdentAcceptedSocket(m_pModule);
}

bool CIdentServer::ConnectionFrom(const CS_STRING & sHostname, u_short uPort)
{
	DEBUG("IDENT connection from " << sHostname << ":" << uPort << " (on " << GetLocalIP() << ":" << GetLocalPort() << ")");

	return (!m_activeUsers.empty());
}

bool CIdentServer::AreIPStringsEqual(const CString& sIP1, const CString& sIP2)
{
	return sIP1.TrimPrefix_n("::ffff:").Equals(sIP2.TrimPrefix_n("::ffff:"));
}

CIdentServer::~CIdentServer()
{
}


/************************************************************************/
/* CIdentAcceptedSocket method implementation section                   */
/************************************************************************/

CIdentAcceptedSocket::CIdentAcceptedSocket(CModule *pMod) : CSocket(pMod)
{
	EnableReadLine();
}

void CIdentAcceptedSocket::ReadLine(const CS_STRING & sLine)
{
	CIdentServerMod *pMod = reinterpret_cast<CIdentServerMod*>(m_pModule);
	const CString sReply = pMod->GetIdentServer()->GetResponse(sLine, GetLocalIP(), GetRemoteIP());

	Write(sReply + "\r\n");

	Close(CLT_AFTERWRITE);
}

CIdentAcceptedSocket::~CIdentAcceptedSocket()
{
}


/************************************************************************/
/* CIdentServerMod method implementation section                        */
/************************************************************************/

CIdentServerMod::EModRet CIdentServerMod::OnIRCConnecting(CIRCSock *pIRCSock)
{
	assert(m_pNetwork != NULL);

	DEBUG("CIdentServerMod::OnIRCConnecting");

	if(!m_identServer)
	{
		DEBUG("Starting up IDENT listener.");
		m_identServer = new CIdentServer(this, m_serverPort);

		if(!m_identServer->StartListening())
		{
			DEBUG("WARNING: Opening the listening socket failed!");
			m_listenFailed = true;
			m_identServer = NULL; /* Csock deleted the instance. (gross) */
			return CONTINUE;
		}
		else
		{
			m_listenFailed = false;
		}
	}

	m_identServer->IncreaseUseCount(m_pNetwork);

	return CONTINUE;
}

void CIdentServerMod::NoLongerNeedsIdentServer()
{
	assert(m_pNetwork != NULL);

	if(m_identServer)
	{
		m_identServer->DecreaseUseCount(m_pNetwork);

		if(!m_identServer->InUse())
		{
			DEBUG("Closing down IDENT listener.");
			m_identServer->Close();
			m_identServer = NULL;
		}
	}
}

void CIdentServerMod::OnIRCConnected()
{
	if((!m_pClient) && (m_listenFailed))
	{
		PutModule("*** WARNING: Opening the listening socket failed!");
		PutModule("*** IDENT listener is NOT running.");
	}
	NoLongerNeedsIdentServer();
}

void CIdentServerMod::OnIRCDisconnected()
{
	NoLongerNeedsIdentServer();
}

CIdentServerMod::EModRet CIdentServerMod::OnDeleteUser(CUser& User)
{
	// NoLongerNeedsIdentServer needs m_pNetwork, so we have to provide it:

	CIRCNetwork* pBackup = m_pNetwork;

	for(CIRCNetwork* pNetwork : User.GetNetworks())
	{
		m_pNetwork = pNetwork;

		NoLongerNeedsIdentServer();
	}

	m_pNetwork = pBackup;

	return CONTINUE;
}

CIdentServerMod::EModRet CIdentServerMod::OnDeleteNetwork(CIRCNetwork& Network)
{
	CIRCNetwork* pBackup = m_pNetwork;

	m_pNetwork = &Network; // meh

	NoLongerNeedsIdentServer();

	m_pNetwork = pBackup;

	return CONTINUE;
}

void CIdentServerMod::OnClientLogin()
{
	if(m_listenFailed)
	{
		PutModule("*** WARNING: Opening the listening socket failed!");
		PutModule("*** IDENT listener is NOT running.");
	}
}

void CIdentServerMod::OnModCommand(const CString& sLine)
{
	CString sCommand = sLine.Token(0);

	if(sCommand.Equals("HELP"))
	{
		CTable Table;
		Table.AddColumn("Command");
		Table.AddColumn("Description");

		Table.AddRow();
		Table.SetCell("Command", "Status");
		Table.SetCell("Description", "Displays status information about IdentServer");

		PutModule(Table);
		return;
	}
	else if(sCommand.Equals("STATUS"))
	{
		if(m_identServer)
		{
			PutModule("IdentServer is listening on: " + m_identServer->GetLocalIP() + ":" + CString(m_serverPort));

			if(m_pUser->IsAdmin())
			{
				PutModule("List of active users/networks:");

				for(CIRCNetwork* pNetwork : m_identServer->GetActiveUsers())
				{
					PutModule("* " + pNetwork->GetUser()->GetCleanUserName() + "/" + pNetwork->GetName());
				}
			}
		}
		else
		{
			if(m_listenFailed)
			{
				PutModule("WARNING: Opening the listening socket failed!");
			}
			PutModule("IdentServer isn't listening.");
		}
		if(m_pUser->IsAdmin())
		{
			PutModule("Last IDENT request: " + m_sLastRequest);
			PutModule("Last IDENT reply: " + m_sLastReply);
		}
	}
	else
	{
		PutModule("Unknown command [" + sCommand + "] try 'Help'");
	}
}

CIdentServerMod::~CIdentServerMod()
{
	if(m_identServer)
	{
		m_identServer->Close();
	}
}


GLOBALMODULEDEFS(CIdentServerMod, "Provides a simple IDENT server implementation.")
