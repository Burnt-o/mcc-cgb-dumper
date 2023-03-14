#pragma once
#include "pch.h"

//Command handler adapted from https://stackoverflow.com/a/31690851



namespace command_handler
{


class CommandBase {
private:
    std::string m_name = "BASE_COMMAND";; //The internal name
    std::string m_help = "BASE_HELP_MESSAGE"; //The internal help line
public:
    virtual void execute(const std::string& line) {}




   virtual ~CommandBase() = default;

   void SetName(const std::string& val) { m_name = val; }
   void SetHelp(const std::string& val) { m_help = val; }
   std::string GetName() const { return m_name; }
   std::string GetHelp() const { return m_help; }

};




void init_commands(const std::vector<CommandBase*>& commandList);
void handle_commands();
}