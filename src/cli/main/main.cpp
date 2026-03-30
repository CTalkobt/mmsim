#include <iostream>
#include <string>
#include "cli_interpreter.h"
#include "plugin_loader/main/plugin_loader.h"
#include "plugin_command_registry.h"
#include "include/util/logging.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    std::cout << "mmemu - Multi Machine Emulator (CLI)\n";
    std::cout << "Version 0.1.0-dev\n";
    
    LogRegistry::instance().init();

    PluginLoader::instance().setCommandRegisterFn([](const PluginCommandInfo* info) {
        PluginCommandRegistry::instance().registerCommand(info);
    });
    
    // Register built-ins to prevent collisions
    const char* builtIns[] = {"help", "?", "list", "create", "reset", "step", "setpc", "regs", "m", "f", "copy", "search", "searcha", "disasm", "asm", "key", "load", "quit", "q", nullptr};
    for (int i = 0; builtIns[i]; ++i) {
        PluginCommandRegistry::instance().registerBuiltIn(builtIns[i]);
    }
    
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
