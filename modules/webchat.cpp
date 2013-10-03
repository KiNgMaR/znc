/**
 * (c) Copyright 2013 - See the AUTHORS file for details.
 * (c) Ingmar Runge 2013
 * (c) flakes @ EFNet 2010
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 **/

#include <znc/Modules.h>
#include <znc/User.h>
#include <znc/Chan.h>
#include <znc/IRCNetwork.h>
#include <znc/znc.h>
#include <map>
#include <vector>

class CJsonValue;
typedef CSmartPtr<CJsonValue> PJsonValue;

class CJsonValue {
public:
	typedef enum {
		JST_INT = 1,
		JST_FLOAT,
		JST_STRING,
		JST_BOOL,
		JST_ARRAY,
		JST_OBJECT,
		JST_NULL
	} JsonType;

	CJsonValue(JsonType a_type) :
		m_type(a_type), m_dataDouble(0), m_dataInt(0), m_flags(0)
	{
	}

	CJsonValue(long long i) { m_type = JST_INT; m_dataInt = i; }
	CJsonValue(int i) { m_type = JST_INT; m_dataInt = i; }
	CJsonValue(unsigned long long i) { m_type = JST_INT; m_dataInt = i; }
	CJsonValue(unsigned int i) { m_type = JST_INT; m_dataInt = i; }
	CJsonValue(double d) { m_type = JST_FLOAT; m_dataInt = d; }
	CJsonValue(const CString& str) { m_type = JST_STRING; m_dataString = str; }
	CJsonValue(const char* str) { m_type = JST_STRING; m_dataString = str; }
	CJsonValue(bool b) { m_type = JST_BOOL; m_dataInt = (b ? 1 : 0); }
	CJsonValue() { m_type = JST_NULL; }

	void SetProperty(const CString& a_name, PJsonValue a_value) {
		if(m_type == JST_OBJECT) {
			m_properties[a_name] = a_value;
		}
	}

	void SetProperty(const CString& a_name, long long i) { SetProperty(a_name, PJsonValue(new CJsonValue(i))); }
	void SetProperty(const CString& a_name, int i) { SetProperty(a_name, PJsonValue(new CJsonValue(i))); }
	void SetProperty(const CString& a_name, unsigned long long i) { SetProperty(a_name, PJsonValue(new CJsonValue(i))); }
	void SetProperty(const CString& a_name, unsigned int i) { SetProperty(a_name, PJsonValue(new CJsonValue(i))); }
	void SetProperty(const CString& a_name, double d) { SetProperty(a_name, PJsonValue(new CJsonValue(d))); }
	void SetProperty(const CString& a_name, const CString& str) { SetProperty(a_name, PJsonValue(new CJsonValue(str))); }
	void SetProperty(const CString& a_name, const char* str) { SetProperty(a_name, PJsonValue(new CJsonValue(str))); }
	void SetProperty(const CString& a_name, bool b) { SetProperty(a_name, PJsonValue(new CJsonValue(b))); }
	void SetPropertyNull(const CString& a_name) { SetProperty(a_name, PJsonValue(new CJsonValue())); }

	bool RemoveProperty(const CString& a_name) {
		return (m_type == JST_OBJECT) && (m_properties.erase(a_name) != 0);
	}

	void Add(PJsonValue a_value) {
		if(m_type == JST_ARRAY) {
			m_entries.push_back(a_value);
		}
	}

	void Add(long long i) { Add(PJsonValue(new CJsonValue(i))); }
	void Add(int i) { Add(PJsonValue(new CJsonValue(i))); }
	void Add(unsigned long long i) { Add(PJsonValue(new CJsonValue(i))); }
	void Add(unsigned int i) { Add(PJsonValue(new CJsonValue(i))); }
	void Add(double d) { Add(PJsonValue(new CJsonValue(d))); }
	void Add(const CString& str) { Add(PJsonValue(new CJsonValue(str))); }
	void Add(const char* str) { Add(PJsonValue(new CJsonValue(str))); }
	void Add(bool b) { Add(PJsonValue(new CJsonValue(b))); }

	CString Serialize() const {
		switch(m_type) {
		case JST_INT: return CString(m_dataInt);
		case JST_FLOAT: return CString(m_dataDouble, 10);
		case JST_STRING: return SerializeString(m_dataString);
		case JST_BOOL: return (m_dataInt ? "true" : "false");
		case JST_ARRAY: return SerializeArray();
		case JST_OBJECT: return SerializeObject();
		default:
			return "null";
		}
	}

	// flag(s) with application-defined value
	void SetFlags(int i) { m_flags = i; }
	int GetFlags() { return m_flags; }

protected:
	JsonType m_type;
	double m_dataDouble;
	long long m_dataInt; // and bool
	CString m_dataString;
	std::map<const CString, PJsonValue> m_properties; // for obj
	std::vector<PJsonValue> m_entries; // for array
	int m_flags;

	// Note: assumes that a_str is a valid UTF-8 string.
	CString SerializeString(const CString& a_str) const {
		CString l_s;
		l_s.reserve(a_str.size());

		for(CString::size_type i = 0; i < a_str.size(); i++) {
			char c = a_str[i];
			switch(c) {
				case '"': l_s += "\\\""; break;
				case '\\': l_s += "\\\\"; break;
				case '/': l_s += "\\/"; break;
				case '\n': l_s += "\\n"; break;
				case '\t': l_s += "\\t"; break;
				default:
					if(c >= 0x20) {
						l_s += a_str[i];
					}
			}
		}

		return "\"" + l_s + "\"";
	}

	CString SerializeArray() const {
		CString l_s = "[ ";

		for(std::vector<PJsonValue>::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
			l_s += (*it)->Serialize() + ", ";
		}

		l_s.TrimRight(", ");
		l_s += " ]";

		return l_s;
	}

	CString SerializeObject() const {
		CString l_s = "{ ";

		for(std::map<const CString, PJsonValue>::const_iterator it = m_properties.begin(); it != m_properties.end(); ++it) {
			l_s += SerializeString(it->first) + ": " + it->second->Serialize() + ", ";
		}

		l_s.TrimRight(", ");
		l_s += " }";

		return l_s;
	}
};

#define NEW_JVAL(VAR, ARGS) PJsonValue VAR(new CJsonValue(ARGS));


class CWebChatMod : public CModule {
private:
	typedef std::map<unsigned int, PJsonValue> MUIntJson;

	unsigned int m_uSequenceMax;
	unsigned int m_uSocketNameIndex;
	VCString m_cometSocketNames;
	MUIntJson m_cometQueue;
	bool m_bIsPreviousJoinNewChannel;

	static const int FLAG_VOLATILE = 1;

	CWebSock* GetCometSock(const CString& sSocketName) {
		if(sSocketName.empty())
			return NULL;
		else
			return dynamic_cast<CWebSock*>(GetManager()->FindSockByName(sSocketName));
	}

	void AddToQueue(PJsonValue jEvent, bool bVolatile = false) {
		NEW_JVAL(jSequenceContainer, CJsonValue::JST_OBJECT);

		++m_uSequenceMax;
		jSequenceContainer->SetProperty("seq", m_uSequenceMax);
		jSequenceContainer->SetProperty("event", jEvent);

		if(bVolatile) jSequenceContainer->SetFlags(FLAG_VOLATILE);
		m_cometQueue[m_uSequenceMax] = jSequenceContainer;

		while(m_cometQueue.size() > 100) {
			m_cometQueue.erase(m_cometQueue.begin());
		}

		// *TODO* this needs some updates to support channel buffers and such
		// (keep a permanent buffer of the last N events, maybe per channel)

		FlushQueueToBrowser();
	}

	void FlushQueueToBrowser() {
		// make a copy of the list and clear the member var...
		const VCString vsSockets = m_cometSocketNames;
		m_cometSocketNames.clear();
		// ... we do that so that Write() calls in the loop
		// do not trigger a select() that may call OnWebPreRequest and
		// mess with m_cometSocketNames.
		VCString vsUnchangedSockets;

		for(VCString::const_iterator sit = vsSockets.begin(); sit != vsSockets.end(); ++sit) {
			CWebSock* pSock = GetCometSock(*sit);

			if(!pSock) continue;

			unsigned int uSequenceStart = pSock->GetParam("seq", false).ToUInt();
			unsigned int uEntriesSent = 0;

			DEBUG("WebChat: Sequence request [" << uSequenceStart << "] [" << *sit << "]");

			if(uSequenceStart > m_uSequenceMax + 1) {
				// notify client about bad sequence number:
				NEW_JVAL(jFalse, false);
				SimpleHTTPJsonResponse(*pSock, jFalse);
			} else {
				NEW_JVAL(jData, CJsonValue::JST_ARRAY);

				for(MUIntJson::iterator it = m_cometQueue.begin(); it != m_cometQueue.end(); ++it) {
					if(it->first >= uSequenceStart) {
						// volatile items should not be sent together with the initial response (which always has a seq. number < 2)
						if((it->second->GetFlags() & FLAG_VOLATILE) == 0 || uSequenceStart > 1) {
							jData->Add(it->second);
							++uEntriesSent;
						}
					}
				}

				if(uEntriesSent > 0) {
					SimpleHTTPJsonResponse(*pSock, jData);

					DEBUG("WebChat: Sent [" << uEntriesSent << "] entries to [" << *sit << "]");
				} else {
					vsUnchangedSockets.push_back(*sit);
				}
			}
		}

		// sockets that a) exist AND b) have not been used have to be kept:
		for(VCString::const_iterator sit = vsUnchangedSockets.begin(); sit != vsUnchangedSockets.end(); ++sit) {
			m_cometSocketNames.push_back(*sit);
		}
	}

	PJsonValue PatchEventJson(const CString& sType, PJsonValue jEventPayload) {
		NEW_JVAL(jEvent, CJsonValue::JST_OBJECT);
		time_t now = time(NULL);
		// not thread-safe, but that's ok -- timestamp is UTC!
		CString sTimestamp(asctime(gmtime(&now)));
		sTimestamp.Trim(); // stupid POSIX

		jEvent->SetProperty("type", sType);
		jEvent->SetProperty("timestamp", sTimestamp);
		jEvent->SetProperty("payload", jEventPayload);

		return jEvent;
	}

	static PJsonValue JsonNick(const CNick& Nick, const CString sRole = "") {
		NEW_JVAL(jNick, CJsonValue::JST_OBJECT);

		if(!sRole.empty()) jNick->SetProperty("role", sRole);
		jNick->SetProperty("nick", Nick.GetNick());
		jNick->SetProperty("ident", Nick.GetIdent());
		jNick->SetProperty("host", Nick.GetHost());
		jNick->SetProperty("full_mask", Nick.GetNickMask());

		return jNick;
	}

	static PJsonValue JsonIRCNetwork(const CIRCNetwork& Network) {
		NEW_JVAL(jNet, CJsonValue::JST_OBJECT);
		NEW_JVAL(jChans, CJsonValue::JST_ARRAY);

		jNet->SetProperty("name", Network.GetName());
		jNet->SetProperty("configured_nick", Network.GetNick());
		jNet->SetProperty("connected", Network.IsIRCConnected());

		if(Network.IsIRCConnected()) {
			jNet->SetProperty("me", JsonNick(Network.GetIRCNick()));
		}

		for(std::vector<CChan*>::const_iterator it = Network.GetChans().begin();
			it != Network.GetChans().end(); ++it)
		{
			jChans->Add(JsonChan(**it, true));
		}

		jNet->SetProperty("channels", jChans);

		return jNet;
	}

	static PJsonValue JsonChan(const CChan& Chan, bool bCompleteInfo = false) {
		PJsonValue jChan = JsonChan(Chan.GetName());

		if(bCompleteInfo) {
			jChan->SetProperty("topic", Chan.GetTopic());

			NEW_JVAL(jNickList, CJsonValue::JST_ARRAY);
			for(std::map<CString, CNick>::const_iterator it = Chan.GetNicks().begin(); it != Chan.GetNicks().end(); ++it) {
				const CNick& Nick = it->second;
				PJsonValue jNick = JsonNick(Nick);
				CString sNickRole;

				if(Nick.HasPerm(CChan::Voice))
					sNickRole = "voice";
				else if(Nick.HasPerm(CChan::HalfOp))
					sNickRole = "halfop";
				else if(Nick.HasPerm(CChan::Op) || Nick.HasPerm(CChan::Admin)
					|| Nick.HasPerm(CChan::Owner) || Nick.HasPerm('&')
					|| Nick.HasPerm('~'))
					sNickRole = "op";
				else
					sNickRole = "regular";

				jNick->SetProperty("role", sNickRole);
				jNickList->Add(jNick);
			}

			jChan->SetProperty("nicklist", jNickList);
		}

		jChan->SetProperty("joined", Chan.IsOn());
		jChan->SetProperty("enabled", !Chan.IsDisabled());
		jChan->SetProperty("detached", Chan.IsDetached());

		return jChan;
	}

	static PJsonValue JsonChan(const CString& sName) {
		NEW_JVAL(jChan, CJsonValue::JST_OBJECT);

		jChan->SetProperty("name", sName);

		return jChan;
	}

	void SimpleHTTPJsonResponse(CWebSock& WebSock, const PJsonValue& jResponse) {
		const CString sResponse = jResponse->Serialize();

		WebSock.PrintHeader(sResponse.size(), "application/json;charset=UTF-8", 200, "OK");
		// *TODO* add an anti-CSRF prefix
		WebSock.Write(sResponse);
		WebSock.Close(Csock::CLT_AFTERWRITE);
	}

	void ApiUserData(CWebSock& WebSock) {
		NEW_JVAL(jData, CJsonValue::JST_OBJECT);
		NEW_JVAL(jUsers, CJsonValue::JST_ARRAY);
		NEW_JVAL(jUser, CJsonValue::JST_OBJECT);
		NEW_JVAL(jNetworks, CJsonValue::JST_ARRAY);

		jUser->SetProperty("user_name", m_pUser->GetUserName());

		for(std::vector<CIRCNetwork*>::const_iterator it = m_pUser->GetNetworks().begin();
			it != m_pUser->GetNetworks().end(); ++it)
		{
			jNetworks->Add(JsonIRCNetwork(**it));
		}

		jUser->SetProperty("networks", jNetworks);

		jUsers->Add(jUser);
		jData->SetProperty("users", jUsers);

		SimpleHTTPJsonResponse(WebSock, jData);
	}

	// *TODO* design this in a way that allows request data
	// from WebSockets to be passed in, too.
	void ApiUserAction(CWebSock& WebSock) {
		bool bResult = false;
		CString sAction = WebSock.GetParam("action");

		if(sAction == "send_chat_message") {
			bResult = SendChatMessage(
				WebSock.GetParam("target_network"),
				WebSock.GetParam("target_type"),
				WebSock.GetParam("target_name"),
				WebSock.GetParam("msg_is_action").ToBool(),
				WebSock.GetParam("text"));
		}

		NEW_JVAL(jResult, bResult);
		SimpleHTTPJsonResponse(WebSock, jResult);
	}

	bool SendChatMessage(const CString& sNetwork, const CString& sToType, const CString& sTarget, bool bIsAction, const CString& sText) {
		CIRCNetwork *pNetwork = m_pUser->FindNetwork(sNetwork);

		if(!pNetwork || !pNetwork->IsIRCConnected() || sText.empty()) {
			return false;
		}

		if(sToType == "chan" && !pNetwork->FindChan(sTarget)) {
			return false;
		}

		CString sMessage = sText.FirstLine().Trim_n(),
			sCmd = "PRIVMSG " + sTarget.FirstLine().Trim_n() + " :" +
			(bIsAction ? "\x01""ACTION " + sMessage + "\x01" : sMessage);

		if(!pNetwork->PutIRC(sCmd)) {
			return false;
		}

		// echo to other clients:
		pNetwork->PutUser(":" + pNetwork->GetIRCNick().GetNickMask() + " " + sCmd);

		// *TODO* find out if this can be improved:
		CIRCNetwork *pNetOld = m_pNetwork;
		m_pNetwork = pNetwork;

		CString sTargetCopy(sTarget);
		if(bIsAction)
			CWebChatMod::OnUserAction(sTargetCopy, sMessage);
		else
			CWebChatMod::OnUserMsg(sTargetCopy, sMessage);

		m_pNetwork = pNetOld;

		return true;
	}

	void QueueUserMessage(const CString& sTarget, const CString& sMessage, bool bIsAction) {
		NEW_JVAL(jEventData, CJsonValue::JST_OBJECT);
		CString sType = "msg_query";

		jEventData->SetProperty("network", m_pNetwork->GetName());
		jEventData->SetProperty("person", JsonNick(m_pNetwork->GetIRCNick()));

		if(m_pNetwork->IsChan(sTarget)) {
			sType = "msg_chan";
			if(CChan* pChan = m_pNetwork->FindChan(sTarget)) {
				jEventData->SetProperty("chan", JsonChan(*pChan));
			} else {
				jEventData->SetProperty("chan", JsonChan(sTarget));
			}
		}

		jEventData->SetProperty("msg_body", sMessage);
		jEventData->SetProperty("msg_is_own", true);
		if(bIsAction) jEventData->SetProperty("msg_is_action", true);

		AddToQueue(PatchEventJson(sType, jEventData));
	}

	void QueueChanMessage(const CNick& Nick, const CChan& Channel, const CString& sMessage, bool bIsAction) {
		NEW_JVAL(jEventData, CJsonValue::JST_OBJECT);

		// *TODO* fix up [convert] invalid UTF-8 if possible,
		// substitute invalid characters if not.

		jEventData->SetProperty("network", m_pNetwork->GetName());
		jEventData->SetProperty("person", JsonNick(Nick));
		jEventData->SetProperty("chan", JsonChan(Channel));
		jEventData->SetProperty("msg_body", sMessage);
		if(bIsAction) jEventData->SetProperty("msg_is_action", true);

		AddToQueue(PatchEventJson("msg_chan", jEventData));
	}

	void QueuePrivMessage(const CNick& Nick, const CString& sMessage, bool bIsAction) {
		NEW_JVAL(jEventData, CJsonValue::JST_OBJECT);

		jEventData->SetProperty("network", m_pNetwork->GetName());
		jEventData->SetProperty("person", JsonNick(Nick));
		jEventData->SetProperty("msg_body", sMessage);
		if(bIsAction) jEventData->SetProperty("msg_is_action", true);

		AddToQueue(PatchEventJson("msg_query", jEventData));
	}

	void QueueUserPart(const CString& sChannel, const CString& sMessage) {
		NEW_JVAL(jEventData, CJsonValue::JST_OBJECT);

		jEventData->SetProperty("network", m_pNetwork->GetName());
		jEventData->SetProperty("chan", JsonChan(sChannel));
		jEventData->SetProperty("part_msg", sMessage);

		AddToQueue(PatchEventJson("user_part", jEventData), true);
	}

public:
	MODCONSTRUCTOR(CWebChatMod) {
		// this allows the client to detect lost events and ZNC restarts:
		m_uSequenceMax = static_cast<unsigned int>(time(NULL) - CZNC::Get().TimeStarted());
		// (if this module is loaded at startup, it's probably 0 - that's fine too.)

		m_uSocketNameIndex = 0;
		m_bIsPreviousJoinNewChannel = false;
	}

	virtual ~CWebChatMod() {
	}

	virtual bool WebRequiresLogin() { return true; }
	virtual bool WebRequiresAdmin() { return false; }
	virtual CString GetWebMenuTitle() { return "Web Chat"; }

	virtual bool OnWebRequest(CWebSock& WebSock, const CString& sPageName, CTemplate& Tmpl) {
		if(sPageName == "index") {
			Tmpl["VersionShort"] = CString((float)VERSION, 1);
			return true;
		}

		return false;
	}

	virtual bool OnWebPreRequest(CWebSock& WebSock, const CString& sPageName) {
		if(!m_pUser) {
			return false;
		}

		if(sPageName == "comet") {
			const CString sSockName = "WebChat::" + m_pUser->GetUserName() + "::Comet::" + CString(m_uSocketNameIndex++);

			WebSock.SetSockName(sSockName);
			WebSock.SetTimeout(60);

			m_cometSocketNames.push_back(sSockName);

			FlushQueueToBrowser();

			return true;
		} else if(sPageName == "user_data") {
			ApiUserData(WebSock);
			WebSock.Close(Csock::CLT_AFTERWRITE);
			return true;
		} else if(sPageName == "user_action") {
			ApiUserAction(WebSock);
			WebSock.Close(Csock::CLT_AFTERWRITE);
			return true;
		}

		return false;
	}

	virtual EModRet OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage) {
		QueueChanMessage(Nick, Channel, sMessage, false);
		return CONTINUE;
	}
	virtual EModRet OnChanAction(CNick& Nick, CChan& Channel, CString& sMessage) {
		QueueChanMessage(Nick, Channel, sMessage, true);
		return CONTINUE;
	}

	virtual EModRet OnPrivMsg(CNick& Nick, CString& sMessage) {
		QueuePrivMessage(Nick, sMessage, false);
		return CONTINUE;
	}
	virtual EModRet OnPrivAction(CNick& Nick, CString& sMessage) {
		QueuePrivMessage(Nick, sMessage, true);
		return CONTINUE;
	}

	virtual EModRet OnUserMsg(CString& sTarget, CString& sMessage) {
		QueueUserMessage(sTarget, sMessage, false);
		return CONTINUE;
	}
	virtual EModRet OnUserAction(CString& sTarget, CString& sMessage) {
		QueueUserMessage(sTarget, sMessage, true);
		return CONTINUE;
	}

	virtual EModRet OnUserPart(CString& sChannel, CString& sMessage) {
		QueueUserPart(sChannel, sMessage);
		return CONTINUE;
	}

	virtual void OnJoin(const CNick& Nick, CChan& Channel) {
		NEW_JVAL(jEventData, CJsonValue::JST_OBJECT);

		jEventData->SetProperty("network", m_pNetwork->GetName());

		if(Nick.GetNick().Equals(m_pNetwork->GetIRCNick().GetNick())) {
			// there usually is no topic or nick list info available at this
			// point, so there's no use in sending it with the JSON:
			jEventData->SetProperty("chan", JsonChan(Channel, false));

			// this looks weird, but is required to get situations with
			// multiple user parts and user joins for the same chan in
			// one buffer correct. *work in progress*
			AddToQueue(PatchEventJson("user_join", jEventData), true);
			AddToQueue(PatchEventJson("user_join_notif", jEventData), false);
		} else {
			jEventData->SetProperty("chan", JsonChan(Channel, false));
			jEventData->SetProperty("person", JsonNick(Nick, "regular"));

			AddToQueue(PatchEventJson("chan_join", jEventData));
		}
	}

	virtual void OnPart(const CNick& Nick, CChan& Channel, const CString& sMessage) {
		if(Nick.GetNick().Equals(m_pNetwork->GetIRCNick().GetNick())) {
			// usually handled in OnUserPart, but we need to consider the case
			// where we are e.g. /sapart'ed (so there's not /part from the client):
			QueueUserPart(Channel.GetName(), sMessage);
			return;
		}

		NEW_JVAL(jEventData, CJsonValue::JST_OBJECT);

		jEventData->SetProperty("network", m_pNetwork->GetName());
		jEventData->SetProperty("chan", JsonChan(Channel, false));
		jEventData->SetProperty("person", JsonNick(Nick));
		jEventData->SetProperty("part_msg", sMessage);

		AddToQueue(PatchEventJson("chan_part", jEventData));
	}
	virtual void OnQuit(const CNick& Nick, const CString& sMessage, const std::vector<CChan*>& vChans) {
		NEW_JVAL(jEventData, CJsonValue::JST_OBJECT);
		NEW_JVAL(jChans, CJsonValue::JST_ARRAY);

		jEventData->SetProperty("network", m_pNetwork->GetName());
		jEventData->SetProperty("person", JsonNick(Nick));
		jEventData->SetProperty("quit_msg", sMessage);

		for(std::vector<CChan*>::const_iterator it = vChans.begin(); it != vChans.end(); ++it) {
			jChans->Add(JsonChan(**it, false));
		}

		jEventData->SetProperty("channels", jChans);

		AddToQueue(PatchEventJson("chan_quit", jEventData));
	}

	virtual EModRet OnRaw(CString& sLine) {
		const CString sCmd = sLine.Token(1).AsUpper();

		if(sCmd == "366") {
			// :irc.server.com 366 nick #chan :End of /NAMES list.
			CChan* pChan = m_pNetwork->FindChan(sLine.Token(3));

			if(pChan && pChan->IsOn()) {
				NEW_JVAL(jEventData, CJsonValue::JST_OBJECT);
				jEventData->SetProperty("network", m_pNetwork->GetName());
				jEventData->SetProperty("chan", JsonChan(*pChan, true));
				jEventData->SetProperty("updated", "nicklist");
				AddToQueue(PatchEventJson("chan_update", jEventData), true);
			}
		}

		return CONTINUE;
	}

	virtual void OnIRCConnected() {
		NEW_JVAL(jEventData, CJsonValue::JST_OBJECT);
		jEventData->SetProperty("network", m_pNetwork->GetName());
		jEventData->SetProperty("connected", true);
		AddToQueue(PatchEventJson("irc_conn", jEventData), true);
	}
	virtual void OnIRCDisconnected() {
		NEW_JVAL(jEventData, CJsonValue::JST_OBJECT);
		jEventData->SetProperty("network", m_pNetwork->GetName());
		jEventData->SetProperty("connected", false);
		AddToQueue(PatchEventJson("irc_conn", jEventData), true);
	}
	virtual EModRet OnBroadcast(CString& sMessage) {
		NEW_JVAL(jEventData, CJsonValue::JST_OBJECT);
		jEventData->SetProperty("message", sMessage);
		AddToQueue(PatchEventJson("znc_broadcast", jEventData));
		return CONTINUE;
	}

};

template<> void TModInfo<CWebChatMod>(CModInfo& Info) {
	Info.SetWikiPage("webchat");
}

USERMODULEDEFS(CWebChatMod, "Browser-based chat for all of your networks.")
