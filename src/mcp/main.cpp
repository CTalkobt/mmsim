#include <cstdio>
#include <cstring>

// mmemu - Multi Machine Emulator
// MCP / agent entry point — placeholder until implementation begins
//
// The MCP server communicates with AI agents via JSON-RPC 2.0 over
// stdin/stdout, following the Model Context Protocol specification.

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    fprintf(stderr, "mmemu - Multi Machine Emulator (MCP Agent)\n");
    fprintf(stderr, "Version 0.1.0-dev\n");
    fprintf(stderr, "Ready — awaiting MCP JSON-RPC on stdin\n");
    return 0;
}
