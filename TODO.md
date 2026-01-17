# ccal TODO

## High Priority

### Module System

- [ ] Implement modules into GUI
- [ ] Add module template/scaffolding tool
- [ ] Create geometry module (area, perimeter, volume calculations)
- [ ] Document module API and creation guidelines

### Converter Enhancements

- [ ] Add weight/mass conversions (kg, lb, oz, g, etc.)
- [ ] Add volume conversions (L, gal, ml, cup, etc.)
- [ ] Add area conversions (m², ft², acre, hectare, etc.)
- [ ] Add pressure conversions (Pa, psi, bar, atm, etc.)
- [ ] Add time conversions (s, min, hr, day, etc.)
- [ ] Implement `--all` flag in integrated converter (show all conversions)
- [ ] Add precision/decimal control flag for converter output

## Medium Priority

### Build & Development

- [ ] Create unified cross-platform build script (Python or Make)
- [ ] Add automated tests for converter module
- [ ] Add automated tests for calculator parser
- [ ] Set up GitHub Actions CI/CD workflow
- [ ] Add CMake support for cross-platform builds
- [ ] Create installation script for system-wide access

### Code Quality

- [ ] Add comprehensive error handling for edge cases
- [ ] Implement memory leak detection in test suite
- [ ] Add input validation for malformed JSON rules
- [ ] Refactor duplicate code in expression parser
- [ ] Add debug/verbose logging mode
- [ ] Improve error messages with suggestions

### Documentation

- [ ] Create examples for custom module development
- [ ] Add API reference documentation
- [ ] Add troubleshooting guide to README
- [ ] Create video/GIF demos for README
- [ ] Document internal architecture
- [ ] Add contributing guidelines (CONTRIBUTING.md)

## Low Priority

### GUI Improvements

- [ ] Add keyboard shortcuts help dialog
- [ ] Implement themes/dark mode
- [ ] Add calculation history panel
- [ ] Add copy/paste for entire expressions
- [ ] Add expression formatting/pretty print
- [ ] Implement scientific notation toggle
- [ ] Add unit converter UI tab

### CLI Features

- [ ] Support configuration file (.ccalrc)
- [ ] Add calculation history with `--history` flag
- [ ] Implement interactive REPL mode
- [ ] Add color output for errors/results
- [ ] Support compound units (e.g., "5ft 6in to cm")
- [ ] Add symbolic constants (pi, e, golden ratio)
- [ ] Implement variable storage (`x = 5`, then use `x`)

### Converter Features

- [ ] Add currency conversion (with API integration)
- [ ] Add data size conversions (B, KB, MB, GB, etc.)
- [ ] Add angle conversions (deg, rad, grad)
- [ ] Add energy conversions (J, cal, kWh, BTU)
- [ ] Add speed conversions (m/s, mph, km/h, knots)
- [ ] Implement batch conversion from file
- [ ] Add conversion formula display with `--show-formula`

## Future Ideas

- [ ] Web interface (WASM compilation)
- [ ] Mobile app version
- [ ] Plugin system for third-party modules
- [ ] Cloud sync for calculation history
- [ ] Integration with spreadsheet applications
- [ ] Voice input support (experimental)
