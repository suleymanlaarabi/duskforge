# Repository Guidelines

## Project Purpose

Dusklight is a C/C++ PC reimplementation of *Twilight Princess*. It keeps original game logic where possible, then layers platform support, rendering/audio/input integration, UI, save tooling, and quality-of-life features around it. Do not add copyrighted game assets; runtime users provide their own supported disc image.

## Architecture Map

- `src/d/`, `src/f_*`, `src/m_*`, `src/JSystem/`, `src/Z2*`: reverse-engineered game systems, actors, framework, math, audio, and engine-like code. Preserve original structure and naming.
- `src/dusk/`: Dusklight-specific application code: launcher/prelaunch UI, settings, update checks, platform glue, save/data helpers, and modern UI behavior.
- `include/`: headers matching the source tree. Add declarations beside the subsystem they belong to.
- `res/`: shipped runtime resources such as fonts, RML UI documents/styles, and logos.
- `assets/`: build/package assets, icons, banners, and documentation images.
- `platforms/`: OS packaging and platform metadata for Windows, iOS/tvOS, etc.
- `tools/`: small developer/data scripts, mostly Python.
- `cmake/`, `CMakeLists.txt`, `CMakePresets.json`, `files.cmake`: build definitions and source registration.

## Build & Run

Use CMake presets. Typical Linux build:

```sh
cmake --preset linux-default-relwithdebinfo
cmake --build --preset linux-default-relwithdebinfo
```

Useful variants include `linux-default-debug`, `linux-clang-debug-asan`, `macos-default-relwithdebinfo`, and `windows-msvc-relwithdebinfo`. Run the executable with a local disc image:

```sh
build/{preset}/dusklight --dvd /path/to/game.iso
```

macOS uses `build/{preset}/Dusklight.app/Contents/MacOS/Dusklight --dvd /path/to/game.iso`.

## Development Rules

Keep changes inside the owning subsystem. Game-behavior work usually belongs in the historical module (`src/d/...`, `src/f_op/...`); launcher, settings, tools, or platform behavior usually belongs in `src/dusk/`.

When modifying original/decomp code for Dusklight, keep the original path visible and gate PC-only behavior with `#if TARGET_PC`. Use `#if AVOID_UB` only for undefined-behavior fixes. In original game code, use `JKR_NEW`, `JKR_DELETE`, and related heap macros instead of direct `new`/`delete`.

## Style

Format C/C++ with `.clang-format`: 4 spaces, 100 columns, left-aligned pointers, sorted includes. Keep existing prefixes and names (`d_`, `f_op_`, `m_Do_`, `Z2`, actor names) unless creating clearly new Dusklight code. Python scripts follow `.flake8`; long lines are allowed.

## Validation

There is no central unit-test suite. Validate by building the affected preset, then smoke-test the changed path in-game or in the relevant UI. For UI changes, check navigation, focus, controller/touch input if applicable, and at least one desktop-sized window. For scripts, run the script with representative input and inspect generated output.
