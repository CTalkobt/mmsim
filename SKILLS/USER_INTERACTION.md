# Skill: User Interaction in mmemu

This skill details how to interact with the simulated machine's keyboard and user inputs via MCP.

## Key Injections

-   **Typing Strings:** To type words (like `RUN` or `LOAD`), you must call `press_key` twice for each character: once with `down=true` and once with `down=false`.
-   **Timing:** For some machines (like C64), you may need to `run_cpu(count=1000)` between characters to allow the machine's KERNAL to poll the keyboard matrix.
-   **Special Keys:** Use common names like `RETURN`, `SPACE`, `SHIFT_L`, `CTRL`, `CLR_HOME`.
-   **Shifted Keys:** To get shifted characters (like `"`, `*`, or shifted PETSCII graphics), you must:
    1.  `press_key(key="SHIFT_L", down=true)`
    2.  `press_key(key="character", down=true)`
    3.  `press_key(key="character", down=false)`
    4.  `press_key(key="SHIFT_L", down=false)`

## Interacting with BASIC

-   **Listing Programs:** Type `L`, `I`, `S`, `T`, `RETURN`.
-   **Running Code:** Type `R`, `U`, `N`, `RETURN`.
-   **New Session:** Type `N`, `E`, `W`, `RETURN` to clear the current BASIC program.

## Navigation and Input

-   **Cursor Keys:** Use `CRSR_UP`, `CRSR_DOWN`, `CRSR_LEFT`, `CRSR_RIGHT` for navigating menus or the screen editor.
-   **Break/Escape:** Use `STOP` (on Commodore) or `ESC` (on Apple/Atari) to interrupt running programs.

## Tools Used
- `press_key`
- `run_cpu`
- `step_cpu`
