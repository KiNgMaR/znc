/*
 * Copyright (C) 2009 laszlo.tamas.szabo
 * Copyright (C) 2012-2014 Ingmar Runge
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

#include "znc_msvc.h"
#include "znc_service.h"
#include "znc/Utils.h"
#include "znc/znc.h"
#include <getopt.h>

static const struct option_a g_LongOpts[] = {
	{ "help",		no_argument,	0,	'h' },
	{ "install",	no_argument,	0,	'i' },
	{ "manual",		no_argument,	0,	'm' },
	{ "uninstall",	no_argument,	0,	'u' },
	{ "datadir",	required_argument, 0, 'd' },
	{ 0, 0, 0, 0 }
};

static void GenerateHelp(const char *appname) {
	CUtils::PrintMessage("USAGE: " + std::string(appname) + " [commands / options]");
	CUtils::PrintMessage("Commands are:");
	CUtils::PrintMessage("\t-h, --help         List available command line options (this page)");
	CUtils::PrintMessage("\t-i, --install      Install ZNC service.");
	CUtils::PrintMessage("\t-h, --uninstall    Uninstall ZNC service");
	CUtils::PrintMessage("The only option is:");
	CUtils::PrintMessage("\t-d, --datadir      Set a different znc repository (default is read from ServiceDataDir in HKLM\\SOFTWARE\\ZNC)\n");
	CUtils::PrintMessage("You do not normally have to use --datadir.");
}

int main(int argc, char *argv[])
{
	CZNCWindowsService Svc;

	int startup = CZNCWin32Helpers::RuntimeStartUp();

	if (startup != 0)
	{
		// :TODO: ReportEvent
		return startup;
	}

	CUtils::SeedPRNG();
	CDebug::SetStdoutIsTTY(false);

	// process command line options first, only accept a single command,
	// for example "--install" and "--uninstall" at the same time is meaningless 

	int iArg, iOptIndex = -1;
	enum { cmdNone, cmdErr, cmdHelp, cmdInstall, cmdUninstall } Command = cmdNone;
	bool bManualStartupInstall = false;

	while ((iArg = getopt_long_a(argc, argv, "hiumd:", g_LongOpts, &iOptIndex)) != -1) {
		switch (iArg) {
		case 'h':
			Command = cmdHelp;
			break;
		case 'i':
			if (Command == cmdNone)
				Command = cmdInstall;
			else
				Command = cmdErr;
			break;
		case 'u':
			if (Command == cmdNone)
				Command = cmdUninstall;
			else
				Command = cmdErr;
			break;
		case 'd':
			Svc.SetDataDir(optarg_a);
			break;
		case 'm':
			bManualStartupInstall = true;
			break;
		}
	}

	CZNCWin32Helpers::SetWindowsServiceMode();

	SERVICE_TABLE_ENTRYW ServiceTable[] = {
		{ZNC_SERVICE_NAME, Svc.ServiceMain},
		{NULL, NULL}
	};

	// it was started by the Windows Service Control Manager
	if (StartServiceCtrlDispatcherW(ServiceTable) != NULL)
		return 0;
	else if (GetLastError() == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
		// it was started as a console app
		switch (Command) {
		case cmdHelp:
			GenerateHelp(argv[0]);
			return 0;
		case cmdInstall:
			if(CZNCWindowsService::InstallService(bManualStartupInstall) == 0)
			{
				std::cout << "InstallService result: OK" << std::endl;
				return 0;
			}
			else
			{
				std::cerr << "InstallService result: Error" << std::endl;
				return 1;
			}
			break;
		case cmdUninstall:
			if(CZNCWindowsService::UninstallService() == 0)
			{
				std::cout << "UninstallService result: OK" << std::endl;
				return 0;
			}
			else
			{
				std::cerr << "UninstallService result: Error" << std::endl;
				return 1;
			}
			break;

		// unknown command or none specified
		case cmdErr:
			CUtils::PrintMessage("There are conflicting options: --install and --uninstall\n");
		case cmdNone:
			GenerateHelp(argv[0]);
			return 1;
		}
	}

	return 0;
}
