#pragma once
#include "pch.h"

//Command handler adapted from https://stackoverflow.com/a/31690851



// Base class for custom commands, defined over in commands.h
class CommandBase {
private:
    std::string m_name = "BASE_COMMAND";; //The internal name
    std::string m_help = "BASE_HELP_MESSAGE"; //The internal help line

protected:
    // Helper function for splitting parameter text
    int splitString(const std::string& line, const std::string& seperator, std::vector<std::string>* values) const;
public:
    virtual void execute(const std::string& line) {}
    virtual ~CommandBase() = default;

   void SetName(const std::string& val) { m_name = val; }
   void SetHelp(const std::string& val) { m_help = val; }
   std::string GetName() const { return m_name; }
   std::string GetHelp() const { return m_help; }

};



// Stores a command list of custom commands. handleCommands will std::cin for user input and check for a match.
class CommandHandler {
private:
    const std::vector<std::unique_ptr<CommandBase>>& mCommandList;

public:
    // Constructor
    explicit CommandHandler(const std::vector<std::unique_ptr<CommandBase>>& commandList) : mCommandList(commandList) {}
    void help() const;
    void handleCommands() const;
    void inputThread();
};

