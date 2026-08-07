# Changelog

All notable changes to the ccal calculator project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.1.0-dev] - local-weather branch

Weather-driven restyle of the GUI, applied from `render_weather` directives
(background `day`, tone `hot`, precipitation `stormy`). Branch-only work; not
part of any release line.

### Added

- `local-weather-style/STYLE.md` - Styling contract for the GUI
  - Architecture summary and GUI build commands
  - Table of every styling anchor with its pre-restyle value
  - `PAL_*` palette contract and brush lifecycle rules
  - Mapping from tone, precipitation, and day/night onto the palette
  - Contrast floor and the procedure for the next restyle
- `ccal_gui.c` palette block - All colours as `PAL_*` macros in one place, so a
  restyle rewrites a single block and never touches layout
- `DrawWeatherButton()` - Owner-draw painter for buttons, with pressed and
  keyboard-focus states
- `ButtonFaceColor()` - Maps a control ID to a face colour, giving digits,
  operators, and the `=` key distinct tones
- `WM_DRAWITEM` case - Dispatches button painting to `DrawWeatherButton()`
- `WM_CTLCOLOREDIT` and `WM_CTLCOLORSTATIC` cases - Colour the input box and
  the result label
- `WM_ERASEBKGND` case - Paints the backdrop and, when precipitation is stormy,
  faint diagonal streaks behind the controls
- Cached `hBackdropBrush` and `hFieldBrush` globals - Created in `WM_CREATE`,
  released in `WM_DESTROY`, so the frequent paint messages allocate no GDI

### Changed

- `AddButton()` - Style flag moved from `BS_PUSHBUTTON` to `BS_OWNERDRAW`; the
  system theme repainted over any colour set on a themed push button
- `DrawHistory()` - Panel fill, entry text, hover fill, hover border, and column
  dividers all read from the palette instead of inline `RGB()` literals
- `wc.hbrBackground` and `histWc.hbrBackground` - Set to `NULL`; the backdrop is
  painted in `WM_ERASEBKGND` and a class brush flashed the system grey first

### Notes

- No SVG assets were copied. The directive gates assets on a game-like context,
  and the calculator is a utility form, so weather reads through palette and
  backdrop only.
- Operator face colour was lightened one step to hold the 4.5:1 contrast floor
  against the primary text colour.

## [2.0.1] - Current Release

### Added

- `copyResults()` - Wrapped clipboard ops in `if (OpenClipboard(hwnd))`, added `GlobalFree(hMem)` on failure
- Message loop - Added app-local Ctrl+C pre-dispatch intercept — fires only for this app's own message queue, never touches other processes

### Removed 

- `WM_HOTKEY` case -	Removed entirely (inline duplicate clipboard code, same OpenClipboard bug)
- `WM_SETFOCUS` -	Removed `RegisterHotKey` call
- `WM_ACTIVATE` -	Removed entirely (only contained hotkey register/unregister)
- `WM_DESTROY` -	Removed `UnregisterHotKey` (nothing to unregister)

## [2.0.0]

### Added

- Modular converter system for unit conversions
  - Core converter engine in `modules/converter.c`
  - Header file `modules/converter.h` for function declarations
  - JSON-based rule system for defining conversions
  - Length conversion module with support for 8 units (mm, cm, m, km, in, ft, yd, mi)
  - Flexible unit naming with case-insensitive aliases
  - Command-line flags auto-generated from rule definitions
  - Support for converting to single unit or displaying all conversions
  - Extensible architecture allowing users to create custom conversion rules
- **Integration with ccal command-line tool**
  - Module flags: `/M`, `-m`, `--module` for invoking converter
  - Syntax: `ccal [flag] <module> <rule> <value> <from_unit> <to_unit>`
  - Example: `ccal -m converter length 10 in cm`
  - Automatic rule file loading from `rules/converter/` directory
  - Formatted output showing conversion result
- New directories:
  - `modules/` - Contains modular feature implementations
  - `rules/` - Contains JSON rule files for modules
  - `rules/converter/` - Conversion rule definitions
- Initial conversion rule file: `rules/converter/length.json`

### Changed

- Updated README.md with comprehensive converter documentation
  - Module structure explanation
  - Integration usage examples with all flag variants (CLI only)
  - Compilation instructions for both integrated and standalone builds
  - Rule format specification
  - Usage examples for length conversions
  - Guide for creating custom conversion rules
- Modified compilation instructions to include converter module
  - CLI standard build: `gcc ccal.c modules/converter.c -o ccal.exe`
  - CLI basic calculator only: `gcc ccal.c -o ccal.exe`

## [1.0.0] - Initial Release

- Windows GUI calculator (`ccal_gui.exe`)
- Cross-platform command-line calculator (`ccal.exe`)
- Shared C parser for both GUI and CLI
- Support for basic arithmetic operations (+, -, *, /, ^)
- Multiple bracket styles: (), [], {}
- Unary minus support
- Decimal precision formatting
- Comma and currency symbol handling
- Quote mode (`-q`, `--quote`) for complex expressions
- Comprehensive help documentation
