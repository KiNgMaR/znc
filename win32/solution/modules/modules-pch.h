#pragma once

// To speed up module compilation, collect some common
// headers in this file. It's referenced from Modules.make.

#include "target_winver.h"
#include "znc_msvc.h"

// all these headers include all the big stdlib headers,
// so we don't have to do it here. yay.

#include <znc/Buffer.h>
#include <znc/Chan.h>
#include <znc/Client.h>
#include <znc/Config.h>
#include <znc/Csocket.h>
#include <znc/FileUtils.h>
#include <znc/HTTPSock.h>
#include <znc/IRCNetwork.h>
#include <znc/IRCSock.h>
#include <znc/Listener.h>
#include <znc/Modules.h>
#include <znc/Nick.h>
#include <znc/Server.h>
#include <znc/Socket.h>
#include <znc/Template.h>
#include <znc/Threads.h>
#include <znc/User.h>
#include <znc/Utils.h>
#include <znc/WebModules.h>
#include <znc/znc.h>
#include <znc/ZNCDebug.h>
#include <znc/ZNCString.h>
