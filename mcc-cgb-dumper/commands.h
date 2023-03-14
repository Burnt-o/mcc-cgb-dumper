#pragma once
#include "pch.h"
#include "command_handler.h"
#include "dumper.h"

class CommandForceDump : public command_handler::CommandBase {
public:
	CommandForceDump() {
		SetName("forcedump");
		SetHelp("forcedump ~ Forces a dump of CGB data. ");
	}

	void execute(const std::string& line) final
	{
		dumper::dump();
		std::cout << "Dump forced" << std::endl;
	}
};

class CommandAutoDumpEnable : public command_handler::CommandBase {
public:
	CommandAutoDumpEnable() {
		SetName("autodump_enable");
		SetHelp("autodump_enable <interval time in seconds> ~ Automatically dump CGB data every x seconds. ");
	}

	void execute(const std::string& line) final
	{
		try
		{
			dumper::AutoDumper::Enable(std::stoi(line));
			std::cout << "autodump enabled, " << line << " second interval" << std::endl;
		}
		catch (std::invalid_argument e)
		{
			std::cout << "Error converting interval to int. Whole numbers only : " << e.what() << std::endl;
		}
	}
};

class CommandAutoDumpDisable : public command_handler::CommandBase {
public:
	CommandAutoDumpDisable() {
		SetName("autodump_disable");
		SetHelp("autodump_disable ~ Disables automatic dumping. ");
	}

	void execute(const std::string& line) final
	{
		dumper::AutoDumper::Disable();
		std::cout << "autodump disabled" << std::endl;
	}
};


class CommandExit : public command_handler::CommandBase {

public:
	CommandExit() {
		SetName("exit");
		SetHelp("exit ~ Will cause the program to terminate. ");
	}

	void execute(const std::string& line) final
	{
		std::cout << "Exiting" << std::endl;
		global_kill::kill_me();
	}
};

class CommandLoggingLevel : public command_handler::CommandBase {
private:
	enum StringValue {
		evNotDefined,
		evVerbose,
		evDebug,
		evInfo,
		evError,
		evEnd
	};
	std::map<std::string, StringValue> stringmap = { {"verbose", evVerbose }, {"debug", evDebug}, { "info", evInfo}, {"error", evError}};

public:
	CommandLoggingLevel() {
		SetName("logging_level");
		SetHelp("logging_level <verbose, debug, info, error> ~ Set the verbosity level of logging. ");
	}

	void execute(const std::string& line) final
	{

		switch (stringmap[line])
		{
		case evVerbose:
			plog::get()->setMaxSeverity(plog::verbose);
			break;

		case evDebug:
			plog::get()->setMaxSeverity(plog::debug);
			break;

		case evInfo:
			plog::get()->setMaxSeverity(plog::info);
			break;

		case evError:
			plog::get()->setMaxSeverity(plog::error);
			break;

		default:
			std::cout << "Error processing logging_level parameter: " << line << std::endl;
			return;
		}

		std::cout << "Log level set to " << line << std::endl;
	}
};

