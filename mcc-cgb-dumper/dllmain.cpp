// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "global_kill.h"
#include "command_handler.h"
#include "commands.h"









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
	init_logging();
	PLOG_INFO << "mcc-cgb-dumper initializing";

	std::vector<command_handler::CommandBase*> commandList;
	commandList.emplace_back(new CommandForceDump());
	commandList.emplace_back(new CommandExit());
	commandList.emplace_back(new CommandLoggingLevel());
	commandList.emplace_back(new CommandAutoDumpEnable());
	commandList.emplace_back(new CommandAutoDumpDisable());

	command_handler::init_commands(commandList);

	// Run/wait for commands, until global kill is set by "exit" command
	while (!global_kill::is_kill_set()) {
		PLOG_VERBOSE << "Ready to process new command.";
		command_handler::handle_commands();
	}

	// wait for autodumper to gracefully end
	dumper::AutoDumper::GetThreadRef().join();

	// shutdown
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

