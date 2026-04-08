# Skill: Memory Manipulation in mmemu

This skill covers techniques for searching, filling, and moving data within the machine's memory space using specialized MCP tools.

## Search and Analysis

-   **Finding Data:** Use `search_memory` to locate specific byte sequences. This is useful for finding:
    -   Sprite data (often recognizable patterns of bits).
    -   Character sets (standard Commodore or custom).
    -   Known string constants.
-   **Hex vs ASCII:** Use `is_hex=true` (default) for byte patterns and `is_hex=false` if searching for readable text.
-   **Start Address:** If you know the general area (e.g., $C000+ for user code), provide `start_addr` to speed up the search.

## Bulk Operations

-   **Clearing Memory:** Use `fill_memory` to quickly zero out a range (e.g., `value=0`) or fill it with a specific character (e.g., `value=32` for spaces in screen memory).
-   **Moving Data:** Use `copy_memory` to move blocks of data (like graphics or buffers) without writing a loop in 6502 assembly. This is an "instant" operation in the emulator.
-   **Double Buffering:** Use `swap_memory` to instantly switch two regions of memory, useful for analyzing double-buffered graphics techniques.

## Tools Used
- `search_memory`
- `fill_memory`
- `copy_memory`
- `swap_memory`
