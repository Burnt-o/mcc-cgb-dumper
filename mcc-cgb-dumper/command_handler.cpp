#include "pch.h"
#include "command_handler.h"
namespace command_handler
{






    int split(const std::string& line, const std::string& seperator, std::vector<std::string>* values) {
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

   

    void help(const std::vector<CommandBase*>& commandList) {
        std::cout << "---------------Operating Instructions---------------" << std::endl;
        for (unsigned i = 0; i < commandList.size(); ++i) {
            std::cout << commandList[i]->GetHelp() << std::endl;
        }
        std::cout << "---------------------------------------------------" << std::endl << std::endl;
    }




    std::vector<CommandBase*> g_commandList;
    void init_commands(const std::vector<CommandBase*>& commandList)
    {

        g_commandList = commandList;
        help(g_commandList);
    }

    void handle_commands() {

        if (g_commandList.empty()) return;

        std::string ourCommand;
        std::string ourParameters;

        


            std::cin >> ourCommand;
            std::getline(std::cin, ourParameters);

            //Remove any preceeding whitespace.
            auto pos = ourParameters.find_first_not_of(' ');
            ourParameters = ourParameters.substr(pos != std::string::npos ? pos : 0); // Had to adjust this line as bind1st was deprecated
          


            bool foundCommand = false;
            for (unsigned i = 0; i < g_commandList.size(); ++i) {
                if (g_commandList[i]->GetName() == ourCommand) {
                    foundCommand = true;
                    g_commandList[i]->execute(ourParameters);
                    break;
                }
                else continue;
            }
            if (!foundCommand) {
                PLOG_ERROR << "The command: " << ourCommand << " was not reconized. Please try again.";
                help(g_commandList);
            }
        
    }

}