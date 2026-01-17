# ccal ![repo icon](assets/icon.png)

A flexible calculator that ships with a Windows GUI front end and a cross-platform CLI. Both layers share the same C parser so complex expressions behave identically. The calculator now includes a modular converter system for unit conversions that can be used with the command-line engine.

## Requirements

All platforms require the GNU compiler `gcc`.

> **Windows note:** the easiest path to `gcc` is through Cygwin.

### Apple

The CLI can be compiled on macOS. Install the toolchain via Homebrew:

```bash
brew install gcc
```

### Linux

The CLI runs on common Linux distributions. Install the essentials with `apt`:

```bash
sudo apt update
sudo apt install build-essential
```

To write changes to `help.txt`, you'll need `xxd`.

```bash
sudo apt install xxd
```

### Windows

First, use Windows Package Manager (`winget`) to install Cygwin:

```bash
winget install Cygwin.Cygwin
```

Next, download the Cygwin installer [setup-x86_64.exe](https://cygwin.com/install.html). Move it to
`/usr/bin` (or another location on your `PATH`) so it can be invoked from the command prompt, then
install `gcc`:

```bash
setup-x86_64.exe -P gcc-core
```

To write changes to `help.txt`, you'll need `xxd`.

```bash
setup-x86_64.exe -P xxd
```

Follow the installer prompts until you reach **Select Packages**.

> If `gcc-core` is not listed, search for it, mark it for installation, then continue.

## Compile Command Line Tool

The CLI builds on macOS, Linux, and Windows. To compile with the converter module:

### With Embedded Rules (Recommended)

This method embeds the JSON rules into the executable, so it works from any directory:

```bash
# First, generate all embedded rule headers automatically
.\build_rules.bat     # Windows
# or
./build_rules.sh      # macOS/Linux

# Then compile with embedded rules flag
gcc -DUSE_EMBEDDED_RULES ccal.c modules/converter.c -o ccal.exe
```

The build script automatically processes all JSON files in `rules/converter/` and generates:
- Individual header files: `modules/length_rules.h`, `modules/temperature_rules.h`, etc.
- Master header: `modules/rules.h` that includes all rule headers

### With External Rules Files

This method loads rules from JSON files at runtime:

```bash
gcc ccal.c modules/converter.c -o ccal.exe
```

**Note:** Without `-DUSE_EMBEDDED_RULES`, you must run ccal from the project directory where `rules/converter/` is accessible.

### Without Converter Module

To compile basic calculator only:

```bash
gcc ccal.c -o ccal.exe
```

To include updates to `help.txt`, regenerate the header before compiling:

```bash
xxd -i help.txt > help.h
```

This embeds the latest help text in the executable.

## Compile GUI

### Clone the repository

Start by cloning the code and switching into the working directory:

```bash
git clone https://github.com/jhauga/ccal.git
cd ccal
```

### Simplest build (no resources)

Compile the GUI without bundling extra resources:

```bash
gcc -DBUILDING_GUI ccal.c ccal_gui.c -o ccal_gui.exe -mwindows
```

### Build with resources (icon, etc.)

First compile the resource script into a COFF object:

```bash
windres ccal_gui.rc -O coff -o ccal_gui.res
```

Then link the GUI, resource object, and core engine together:

```bash
gcc -DBUILDING_GUI ccal_gui.c ccal.c ccal_gui.res -o ccal_gui.exe -mwindows
```

## GUI Usage (Windows)

1. Launch `ccal_gui.exe` from Explorer or the command line.
2. Enter expressions using the on-screen buttons or the keyboard.
3. Press `Enter` or click `=` to evaluate.
4. Press `d` to clear the current expression and result.
5. Use `Ctrl + C` to copy the result to the clipboard.

## Command Line Usage

To use the CLI, pass digits, arithmetic operators, and nesting characters (`(`, `)`, `{`, `}`, `[`, `]`) separated with spaces.

> **Important:** use `x` instead of `*`, and `p` instead of `^`, unless you pass `-q/--quote`.

### Acceptable Arithmetic Operators

```text
+  -> addition
-  -> subtraction
/  -> division
x  -> multiplication
*  -> multiplication (use -q/--quote)
p  -> exponentiation (power of)
^  -> exponentiation (use -q/--quote)
```

### Command Line Use Examples

To use the command line tool either input all elements of the equation separated with a space
character, or use the quote option `-q, --quote` which does not require elements to be separated
with a space character.

> If nested brackets cause argument parsing issues, escape them or wrap the expression in quotes. Square brackets generally require the fewest escapes.

Add two numbers:

```bash
> ccal 1 + 1
> 2
```

Perform a complex equation:

```bash
> ccal [ [ 34 x 11 ] / [ 10 - 5 ] ] - [ 4 x 53 x [ 30 / 10 ] ]
> -561.2
```

> Without `-q/--quote`, order of operations can break across spaced tokens, so expressions like:

```bash
> ccal [ [ 34 x 11 ] / [ 10 - 5 ] ] - [ 4 x 53 x [ 30 / 10 ] + [ 2 x [ 40 - 20 ] ] ] / [ 8 x 6 x [ 12 / 2 ] + 4 ]
> -2.058904109589041
```

will evaluate incorrectly (the correct answer is `72.4849`). Use `-q/--quote`—recommended—or add explicit parentheses to force the intended precedence:

**Quote option (recommended):**

```bash
> ccal -q "[[34x11]/[10-5]]-[4x53x[30/10]+[2x[40-20]]]/[8x6x[12/2]+4]"
> 72.48493150684931
```

**Manual grouping:**

```bash
> ccal [ [ 34 x 11 ] / [ 10 - 5 ] ] - [ [ [ 4 x 53 ] x [ 30 / 10 ] + [ 2 x [ 40 - 20 ] ] ] / [ 8 x 6 x [ 12 / 2 ] + 4 ] ]
> 72.48493150684931
```

Both approaches return the correct value.

> Terminals treat commas as separators, so wrap comma-separated numbers in quotes or use `-q/--quote`. Instead of:

```bash
> ccal 1,000 - 1
> Error: Invalid expression
```

Use the quote option or wrap comma numbers manually:

**Quote option:**

```bash
> ccal --quote "2,000-1,000"
> 1000
```

**Manual quoting:**

```bash
> ccal "2,000" - "1,000"
> 1000
```

to get the correct calculation.

### Calculate Exponentiation

To calculate exponentiation, use `p` as the operator. With `-q/--quote`, you can also use `^`.

```bash
> ccal 2 p 2
> 4

> ccal --quote "2^2"
> 4
```

## Unit Conversion Module

The ccal calculator includes a modular converter system for unit conversions. The converter is extensible and rule-based, allowing users to create custom conversion rules.

### Integrated Usage

The converter module is integrated directly into the ccal command-line tool. Use the module flag to perform conversions:

**Syntax:**

```bash
# With explicit rule name
ccal [/M|-m|--module] <module> <rule> <value> <from_unit> <to_unit>

# With auto-detection (embedded rules only)
ccal [/M|-m|--module] <module> <value> <from_unit> <to_unit>
```

**Examples:**

```bash
# Explicit rule name
ccal -m converter length 10 in cm
# Output: 10.000000 in = 25.400000 cm

# Auto-detection (when using embedded rules)
ccal -m converter 10 in cm
# Output: 10.000000 in = 25.400000 cm

# Temperature conversion with auto-detection
ccal -m converter 100 C F
# Output: 100.000000 C = 212.000000 F

# Convert 5 feet to meters using /M flag
ccal /M converter 5 ft m
# Output: 5.000000 ft = 1.524000 m

# Convert 100 kilometers to miles using --module flag
ccal --module converter length 100 km mi
# Output: 100.000000 km = 62.137100 mi
```

The integrated converter automatically:

- Loads conversion rules from embedded data (when compiled with `-DUSE_EMBEDDED_RULES`)
- Auto-detects the appropriate rule based on units (rule name optional)
- Performs the conversion calculation
- Formats and displays the result

**Auto-Detection:** When using embedded rules, the rule name is optional. The converter will automatically detect which rule to use based on the units:

```bash
# These are equivalent:
ccal -m converter length 10 in cm
ccal -m converter 10 in cm

# Both work for temperature too:
ccal -m converter temperature 100 C F
ccal -m converter 100 C F
```

### Module Structure

The converter system uses:

- **`modules/`** - Contains converter implementation code
  - `converter.c` - Core conversion engine
  - `converter.h` - Function declarations and structures
  - `length_rules.h`, `temperature_rules.h` - Embedded rules (generated from JSON)
  - `rules.h` - Master header that includes all rule files
- **`rules/`** - Contains JSON rule files for different conversion types
  - `converter/length.json` - Length/distance conversion rules
  - `converter/temperature.json` - Temperature conversion rules
  - Users can add custom rule files following the same format

**Embedded Rules:** When compiled with `-DUSE_EMBEDDED_RULES`, all JSON rules in `rules/converter/` are embedded directly into the executable using generated header files. This allows the converter to:
- Work from any directory without needing access to external JSON files
- Auto-detect which rule to use based on the units provided
- Support multiple conversion types (length, temperature, etc.) seamlessly

Use the `build_rules.bat` (Windows) or `build_rules.sh` (macOS/Linux) script to automatically generate headers for all rules.

### Conversion Rules Format

Conversion rules are defined in JSON files with the following structure:

```json
{
  "converter": ["mm", "cm", "m", "km", "in", "ft", "yd", "mi"],
  "length": [
    {
      "name": "in,inch",
      "to": [25.4, 2.54, 0.0254, 0.0000254, 1, 0.0833, 0.0278, 0.0000158]
    }
  ]
}
```

- **`converter`**: Array of unit abbreviations in order
- **`name`**: Comma-separated unit names/aliases (case-insensitive)
  - Creates CLI flags: `-in`, `--inch`
  - Creates GUI label: `inch(in)`
- **`to`**: Conversion factors in the same order as the `converter` array

### Compile Converter Module

The converter is integrated into ccal. To compile with embedded rules:

```bash
# Generate all rule headers automatically
.\build_rules.bat     # Windows
./build_rules.sh      # macOS/Linux

# Compile with embedded rules
gcc -DUSE_EMBEDDED_RULES ccal.c modules/converter.c -o ccal.exe
```

Or compile with external rule files:

```bash
gcc ccal.c modules/converter.c -o ccal.exe
```

**Adding New Rules:** To add a new conversion type:

1. Create a JSON file in `rules/converter/` (e.g., `weight.json`)
2. Run the build script: `.\build_rules.bat` or `./build_rules.sh`
3. Add the new rule name to the `rule_names[]` array in `auto_detect_rule()` function in [modules/converter.c](modules/converter.c)
4. Add a corresponding case in `load_embedded_conversion_rules()` function
5. Recompile with `-DUSE_EMBEDDED_RULES`

### Standalone Converter Usage

If you compiled the standalone converter tool, you can use it separately:

Convert a value from one unit to another:

```bash
> converter 10 inch cm
> 10.0000 in = 25.400000 cm
```

Convert and display all available units:

```bash
> converter 5 ft --all
> 5.0000 ft =
>   mm           : 1524.000000
>   cm           : 152.400000
>   m            : 1.524000
>   km           : 0.001524
>   in           : 60.000000
>   ft           : 5.000000
>   yd           : 1.665000
>   mi           : 0.000945
```

### Supported Units

#### Length

The length converter supports the following units with flexible naming:

- **Millimeter**: `mm`, `millimeter` → `-mm`, `--millimeter`
- **Centimeter**: `cm`, `centimeter` → `-cm`, `--centimeter`
- **Meter**: `m`, `meter` → `-m`, `--meter`
- **Kilometer**: `km`, `kilometer` → `-km`, `--kilometer`
- **Inch**: `in`, `inch` → `-in`, `--inch`
- **Foot**: `ft`, `foot` → `-ft`, `--foot`
- **Yard**: `yd`, `yard` → `-yd`, `--yard`
- **Mile**: `mi`, `mile` → `-mi`, `--mile`

#### Temperature

The temperature converter supports the following units:

- **Celsius**: `C`, `celsius` → `-C`, `--celsius`
- **Fahrenheit**: `F`, `fahrenheit` → `-F`, `--fahrenheit`
- **Kelvin**: `K`, `kelvin` → `-K`, `--kelvin`

Temperature conversions use both multiplication factors and offsets to handle the different scales correctly.

All unit names are case-insensitive and support multiple aliases.

### Creating Custom Conversion Rules

Users can create their own conversion rules by adding JSON files to the `rules/converter/` directory. Follow this syntax structure:

#### Rule File Syntax

```json
{
  "converter": ["unit1", "unit2", "unit3", ...],
  "rule_name": [
    {
      "name": "unit_name,alias1,alias2",
      "to": [factor_to_unit1, factor_to_unit2, factor_to_unit3, ...],
      "offset": [offset_for_unit1, offset_for_unit2, offset_for_unit3, ...]
    }
  ]
}
```

**Required Fields:**

- **`"converter"`**: Array of unit abbreviations in the order they appear in conversion arrays
  - Example: `["mm", "cm", "m", "km"]` for length
  - These are the short names displayed in output

- **`"rule_name"`**: Array of conversion unit definitions (use the rule file name without `.json`)
  - Must match your filename: `weight.json` → `"weight": [...]`

- **`"name"`**: Comma-separated aliases for each unit (case-insensitive)
  - First name is typically the primary identifier
  - Additional names are alternative ways to reference the same unit
  - Example: `"in,inch,inches"` allows users to type any of these

- **`"to"`**: Array of conversion factors from this unit to each unit in `converter` array
  - Length must match `converter` array length
  - Order must match `converter` array order
  - Each value converts 1 unit of the current unit to the corresponding target unit
  - Example: For inches, `[25.4, 2.54, 0.0254]` converts to [mm, cm, m]

**Optional Fields:**

- **`"offset"`**: Array of offset values for conversions (typically used for temperature scales)
  - Use when conversion requires: `result = (value * factor) + offset`
  - If omitted, offset defaults to 0 for all conversions
  - Example: Temperature conversions like Celsius to Fahrenheit need both factor and offset
