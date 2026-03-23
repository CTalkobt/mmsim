# CHANGELOG.md - mmsim (Multi-Machine Simulator)

All notable changes to this project will be documented in this file.

## [0.1.0-dev] - 2026-03-23

### Added
- **Project Foundation (Phase 0):**
    - Created directory skeleton for core libraries: `src/libmem/`, `src/libcore/`, `src/libdevices/`, `src/libtoolchain/`, and `src/libdebug/`.
    - Created `roms/` and `tests/` directories.
    - Added `.gitkeep` and `.gitignore` files to ensure proper tracking of empty directories and ignored ROM files.
- **Library Build System:**
    - Extensively updated `Makefile` to support modular builds.
    - Added targets for building static archives (`make libs`) and running tests (`make test`).
    - Configured binaries (`mmemu-cli`, `mmemu-mcp`, `mmemu-gui`) to link against internal libraries.
- **ROM Loader Utility:**
    - Implemented `romLoad` in `src/libcore/rom_loader.h/cpp` for validated binary loading.
- **Documentation:**
    - Created `GEMINI.md` for project overview and quick-start instructions.
    - Created `STYLEGUIDE.md` to establish project-wide coding conventions.

### Changed
- **Style Alignment:**
    - Updated all architectural specifications (`.plan/arch.md`) and roadmaps (`.plan/todo.md`) to use `camelCase` for variable and function names as per the new style guide.
    - Refactored placeholder headers in `src/` to follow `camelCase` and `PascalCase` conventions.
    - Renamed several identifiers in internal stubs for consistency.

### Fixed
- Fixed build system to correctly handle library object dependencies and include paths.
