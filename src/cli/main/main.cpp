#include <iostream>
#include <string>
#include "cli_interpreter.h"
#include "plugin_loader/main/plugin_loader.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    std::cout << "mmemu - Multi Machine Emulator (CLI)\n";
    std::cout << "Version 0.1.0-dev\n";
    
    PluginLoader::instance().loadFromDir("./lib");

    std::cout << "Type 'help' for a list of commands.\n";

    CliContext ctx;
    CliInterpreter interpreter(ctx, [](const std::string& out) {
        std::cout << out;
    });
    
    std::string line;
    while (!ctx.quit) {
        if (interpreter.isAssemblyMode()) {
            std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << interpreter.getAsmAddr() << "> ";
        } else {
            std::cout << "> ";
        }
        
        if (!std::getline(std::cin, line)) break;
        interpreter.processLine(line);
    }

    PluginLoader::instance().unloadAll();
    return 0;
}
