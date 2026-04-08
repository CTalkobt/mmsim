# mmemu LLM Skills

This directory contains specialized instructions and workflows ("skills") for an LLM to effectively use the mmemu MCP (Model Context Protocol) server tools.

## Available Skills

- **[DEBUGGING](DEBUGGING.md)**: Strategies for state inspection, breakpoints, and execution tracing.
- **[PROGRAM INJECTION](PROGRAM_INJECTION.md)**: How to inject and run custom assembly snippets.
- **[HARDWARE INSPECTION](HARDWARE_INSPECTION.md)**: Monitoring memory-mapped I/O and hardware registers.
- **[MACHINE MANAGEMENT](MACHINE_MANAGEMENT.md)**: Creating, loading, and resetting machine instances.
- **[MEMORY MANIPULATION](MEMORY_MANIPULATION.md)**: Searching, filling, and copying memory regions.
- **[USER INTERACTION](USER_INTERACTION.md)**: Key injections and interacting with BASIC or menus.
- **[LOGGING AND INSTRUMENTATION](LOGGING_AND_INSTRUMENTATION.md)**: Using internal logging to analyze hardware chip state.

## How to use

These skills should be provided to the LLM as part of its system instructions or as a supplemental context when it is interacting with the mmemu MCP server. They help the model understand the nuances of the underlying hardware and the most efficient way to use the available tools.
