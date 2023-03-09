#pragma once
#include "pch.h"




namespace command_handler
{


class CommandBase {
protected:
    std::string m_name;//The internal name
    std::string m_help;//The internal help line
public:
    //The public interface for name.
    const std::string& name = m_name;
    //The public interface for the help line.
    const std::string& help = m_help;
    virtual void execute(const std::string& line) {}

    CommandBase() {
        m_name = "BASE_COMMAND";
        m_help = "BASE_HELP_MESSAGE";
    }

};




void init_commands(const std::vector<CommandBase*>& commandList);
void handle_commands();
}