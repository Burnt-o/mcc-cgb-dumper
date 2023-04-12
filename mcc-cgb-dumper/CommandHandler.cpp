#include "pch.h"
#include "CommandHandler.h"

#include "GlobalKill.h"



    int CommandBase::splitString(const std::string& line, const std::string& seperator, std::vector<std::string>* values) const {
        std::string tString = "";
        unsigned counter = 0;
        for (unsigned l = 0; l < line.size(); ++l) {
            for (unsigned i = 0; i < seperator.size(); ++i) {
                if (line[l + i] == seperator[i]) {
                    if (i == seperator.size() - 1) {
                        values->push_back(tString);
                        tString = "";
                        ++counter;
                    }
                    else continue;
                }
                else {
                    tString.push_back(line[l]);
                    break;
                }
            }
        }
        if (tString != "")values->push_back(tString);
        return counter;
    }

   

    void CommandHandler::help() const {
        std::cout << "---------------Operating Instructions---------------" << std::endl;
        for (auto& command : mCommandList)
        {
            std::cout << command->GetHelp() << std::endl;
        }
        std::cout << "---------------------------------------------------" << std::endl << std::endl;
    }




    void CommandHandler::handleCommands() const {


            if (mCommandList.empty()) return;

            std::string userCommand;
            std::string userParameters;

            std::cin >> userCommand;
            std::getline(std::cin, userParameters);

            //Remove any preceeding whitespace.
            auto pos = userParameters.find_first_not_of(' ');
            userParameters = userParameters.substr(pos != std::string::npos ? pos : 0); // Had to adjust this line as bind1st was deprecated


            bool foundCommand = false;
            // Loop through our commandlist and check if we have a match
            for (auto& command : mCommandList)
            {
                if (command->GetName() == userCommand)
                {
                    // A match! Execute the command
                    foundCommand = true;
                    command->execute(userParameters);
                    break;
                }
                else continue;
            }

            // No command at all was found, user must have made a mistake
            if (!foundCommand) {
                PLOG_ERROR << "The command: " << userCommand << " was not reconized. Please try again.";
                help();
            }

        
    }

