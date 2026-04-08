# Skill: Logging and Instrumentation in mmemu

This skill details how to use internal logging to analyze the state of simulated hardware components through MCP.

## Logger Management

-   **Discovery:** Use `list_loggers` to see all active chips and systems currently being logged (e.g., `vic2`, `sid`, `cia`, `kernal`).
-   **Level Control:** Use `set_log_level` to change how much information is being output.
    -   `target="all", level="info"`: Standard status updates for all components.
    -   `target="vic2", level="debug"`: Full detail on video frame timing and register changes.
    -   `target="sid", level="warn"`: Only show potential errors or bad hardware configurations.

## Troubleshooting Hardware

-   **Interrupt Issues:** If the machine isn't responding, check `set_log_level(target="cpu", level="debug")` to see if it's trapped in a loop or handling excessive interrupts (`triggerIrq`).
-   **No Sound:** Check `set_log_level(target="sid", level="debug")` to confirm that the chip is receiving data from the CPU.
-   **ROM Failures:** Check `set_log_level(target="kernal", level="info")` to see standard bootstrap and loading messages.

## Best Practices

-   **Don't over-log:** High log levels (especially `debug`) on the CPU can generate vast amounts of output, potentially slowing down the session. Use it selectively.
-   **Filter by Target:** Always specify the `target` chip or system instead of setting `all` to `debug` to avoid being overwhelmed by unrelated information.

## Tools Used
- `list_loggers`
- `set_log_level`
