#pragma once
#include "pch.h"
#include "CommandHandler.h"
#include "CustomGameRefresher.h"
#include "AutoDumper.h"



class CommandExit : public CommandBase {

public:
	CommandExit() {
		SetName("exit");
		SetHelp("exit ~ Will cause the program to terminate. ");
	}

	void execute(const std::string& line) final
	{
		std::cout << "Exiting" << std::endl;
		GlobalKill::killMe();
	}
};


class CommandLoggingLevel : public CommandBase {

public:
	CommandLoggingLevel() {
		SetName("logging");
		SetHelp("logging <verbose, debug, info, error> ~ Set the verbosity level of logging. ");
	}

	void execute(const std::string& line) final
	{

		plog::Severity severity = plog::severityFromString(line.c_str());

		if (severity == plog::Severity::none)
		{
			std::cout << "logging error: could not parse parameter: " << line << std::endl;
		}
		else
		{
			plog::get()->setMaxSeverity(severity);
			std::cout << "Log level set to " << plog::severityToString(severity) << std::endl;
		}

	}
};

class CommandSetJsonPath : public CommandBase {

private: 
	std::shared_ptr<AutoDumper> autoDumperInstance;

public:
	explicit CommandSetJsonPath(std::shared_ptr<AutoDumper> instance) : autoDumperInstance(instance) {
		SetName("set_dump_dir");
		SetHelp("set_dump_dir <path> ~ Set the directory that the CGB json data should be dumped to. ");
	}

	void execute(const std::string& line) final
	{

		if (this->autoDumperInstance.get()->setJsonDumpPath(line))
		{
			std::cout << "Successfully set json dump directory to " << line << std::endl;
		}
		else
		{
			std::cout << "Failed to set json dump directory to " << line << std::endl;
		}
	}
};


class CommandForceRefresh : public CommandBase {
private:
	std::shared_ptr<CustomGameRefresher> customGameRefresherInstance;

public:
	explicit CommandForceRefresh(std::shared_ptr<CustomGameRefresher> instance) : customGameRefresherInstance(instance) {
		SetName("force_refresh");
		SetHelp("force_refresh ~ Force the custom game browser to refresh. ");
	}

	void execute(const std::string& line) final
	{
		std::cout << "Forcing CGB refresh." << std::endl << std::endl;
		customGameRefresherInstance.get()->forceRefresh();
	}
};



class CommandAutoRefreshEnable : public CommandBase {
private:
	std::shared_ptr<CustomGameRefresher> customGameRefresherInstance;

public:
	explicit CommandAutoRefreshEnable(std::shared_ptr<CustomGameRefresher> instance) : customGameRefresherInstance(instance) {
		SetName("autorefresh_enable");
		SetHelp("autorefresh_enable <interval time in seconds> ~ Automatically refresh the CGB window every x seconds. ");
	}

	void execute(const std::string& line) final
	{
		try
		{
			customGameRefresherInstance.get()->enableAutoRefresh(std::stoi(line));
			std::cout << "autodump enabled, " << line << " second interval" << std::endl;
		}
		catch (std::invalid_argument e)
		{
			std::cout << "autodump_enable error: could not convert parameter to int. Whole numbers only : " << e.what() << std::endl;
		}
	}
};

class CommandAutoRefreshDisable : public CommandBase {
private:
	std::shared_ptr<CustomGameRefresher> customGameRefresherInstance;

public:
	explicit CommandAutoRefreshDisable(std::shared_ptr<CustomGameRefresher> instance) : customGameRefresherInstance(instance) {
		SetName("autorefresh_disable");
		SetHelp("autorefresh_disable ~ Disables automatic refreshing. ");
	}

	void execute(const std::string& line) final
	{
		customGameRefresherInstance.get()->disableAutoRefresh();
		std::cout << "autorefresh disabled" << std::endl;
	}
};