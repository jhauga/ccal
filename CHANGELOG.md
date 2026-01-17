# Changelog

All notable changes to the ccal calculator project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - Current Release

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
