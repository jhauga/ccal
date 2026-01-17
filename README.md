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

```bash
gcc ccal.c modules/converter.c -o ccal.exe
```

To compile without the converter module (basic calculator only):

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
ccal [/M|-m|--module] <module> <rule> <value> <from_unit> <to_unit>
```

**Examples:**

```bash
# Convert 10 inches to centimeters
ccal -m converter length 10 in cm
# Output: 10.000000 in = 25.400000 cm

# Convert 5 feet to meters using /M flag
ccal /M converter length 5 ft m
# Output: 5.000000 ft = 1.524000 m

# Convert 100 kilometers to miles using --module flag
ccal --module converter length 100 km mi
# Output: 100.000000 Km = 62.137100 mi
```

The integrated converter automatically:

- Loads conversion rules from `rules/converter/<rule>.json`
- Performs the conversion calculation
- Formats and displays the result

### Module Structure

The converter system uses:

- **`modules/`** - Contains converter implementation code
  - `converter.c` - Core conversion engine
  - `converter.h` - Function declarations and structures
- **`rules/`** - Contains JSON rule files for different conversion types
  - `converter/length.json` - Length/distance conversion rules
  - Users can add custom rule files following the same format

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

The converter is now integrated into ccal. Compile with:

```bash
gcc ccal.c modules/converter.c -o ccal.exe
```

To compile the converter as a standalone tool (for testing):

```bash
gcc -DCONVERTER_STANDALONE modules/converter.c -o converter.exe
```

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

### Supported Units (Length)

The length converter supports the following units with flexible naming:

- **Millimeter**: `mm`, `millimeter` → `-mm`, `--millimeter`
- **Centimeter**: `cm`, `centimeter` → `-cm`, `--centimeter`
- **Meter**: `m`, `meter` → `-m`, `--meter`
- **Kilometer**: `km`, `kilometer` → `-km`, `--kilometer`
- **Inch**: `in`, `inch` → `-in`, `--inch`
- **Foot**: `ft`, `foot` → `-ft`, `--foot`
- **Yard**: `yd`, `yard` → `-yd`, `--yard`
- **Mile**: `mi`, `mile` → `-mi`, `--mile`

All unit names are case-insensitive and support multiple aliases.

### Creating Custom Conversion Rules

Users can create their own conversion rules by adding JSON files to the `rules/converter/` directory. The format must match the structure shown above:

1. Define the `converter` array with unit abbreviations
2. Create conversion entries with:
   - `name`: Unit names/aliases (comma-separated)
   - `to`: Conversion factors matching the order in `converter`

Example for creating a temperature converter at `rules/converter/temperature.json`:

```json
{
  "converter": ["C", "F", "K"],
  "temperature": [
    {
      "name": "C,celsius",
      "to": [1, 1.8, 1],
      "offset": [0, 32, 273.15]
    },
    {
      "name": "F,fahrenheit",
      "to": [0.55555555556, 1, 0.55555555556],
      "offset": [-17.77777777778, 0, 255.37222222222]
    },
    {
      "name": "K,kelvin",
      "to": [1, 1.8, 1],
      "offset": [-273.15, -459.67, 0]
    }
  ]
}
```
