// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "global_kill.h"
#include "command_handler.h"
#include "commands.h"
#include "dumper.h"








void init_logging()
{
	AllocConsole();
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);

	static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;

	plog::Severity logging_level;
#ifdef _DEBUG
	logging_level = plog::verbose;
#else
	logging_level = plog::info;
#endif // _DEBUG

	plog::init(logging_level, &consoleAppender);
}



void stop_logging()
{
	PLOG_VERBOSE << "Shutting down logging";
	HWND myConsole = GetConsoleWindow();
	FreeConsole();
	PostMessage(myConsole, WM_CLOSE, 0, 0);
}


// Main Execution Loop
void RealMain() {

	{ // Enclosing scope for used resources
		init_logging();
		PLOG_INFO << "mcc-cgb-dumper initializing";

		// Construct the CustomGameRefresher (automatically forces the game to refresh the CGB)
		std::shared_ptr<CustomGameRefresher> customGameRefresher;
		try
		{
			customGameRefresher = std::make_shared<CustomGameRefresher>();
		}
		catch(std::runtime_error& ex)
		{
			PLOG_ERROR << "customGameRefresher failed to initialize, error: ";
			PLOG_ERROR << ex.what();

			PLOG_ERROR << "Enter any command to exit";
			std::string dontCare;
			std::cin>>dontCare;
			stop_logging();
			return;
		}

		// Construct the AutoDumper (automatically parses CG info to json file whenever the CGB is refreshed)
		std::shared_ptr<AutoDumper> autoDumper;
		try
		{
			autoDumper = std::make_shared<AutoDumper>();
		}
		catch (std::runtime_error& ex)
		{
			PLOG_ERROR << "autoDumper failed to initialize, error: ";
			PLOG_ERROR << ex.what();

			PLOG_ERROR << "Enter any command to exit";
			std::string dontCare;
			std::cin >> dontCare;
			stop_logging();
			return;
		}

		std::cout << "Refresh function hooked: Custom Game info will be dumped to json whenever CGB is refreshed. " << std::endl;
		

		std::vector<std::unique_ptr<CommandBase>> commandList;
		commandList.emplace_back(std::make_unique<CommandExit>());
		commandList.emplace_back(std::make_unique<CommandLoggingLevel>());
		commandList.emplace_back(std::make_unique<CommandSetJsonPath>(autoDumper));
		commandList.emplace_back(std::make_unique<CommandForceRefresh>(customGameRefresher));
		commandList.emplace_back(std::make_unique<CommandAutoRefreshEnable>(customGameRefresher));
		commandList.emplace_back(std::make_unique<CommandAutoRefreshDisable>(customGameRefresher));
		

		CommandHandler commandHandler(commandList);
		commandHandler.help(); // Print list of commands

		std::cout << "Logging level set to: " << plog::severityToString(plog::get()->getMaxSeverity()) << std::endl;

		// Run/wait for commands, until global kill is set by "exit" command
		while (!global_kill::is_kill_set()) {
			PLOG_VERBOSE << "Ready to process new command.";
			commandHandler.handleCommands();
		}
	}
	// Any used resources should be out of scope now, ie unique_ptrs and shared_ptrs should be dead
	// So we just need to cleanup anything that we have to do manually
	stop_logging();
}


// This thread is created by the dll when loaded into the process, see RealMain() for the actual event loop.
// Do NOT put any allocations in this function because the call to FreeLibraryAndExitThread()
// will occur before they fall out of scope and will not be cleaned up properly! This is very
// important for being able to hotload the DLL multiple times without restarting the game.
DWORD WINAPI MainThread(HMODULE hDLL) {
	RealMain();

	Sleep(200);
	FreeLibraryAndExitThread(hDLL, NULL);
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	DWORD dwThreadID;

	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		CreateThread(NULL, 0x1000, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, &dwThreadID);
	}

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;

}

