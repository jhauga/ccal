# ccal ![repo icon](assets/icon.png)

A flexible calculator that ships with a Windows GUI front end and a cross-platform CLI. Both layers share the same C parser so complex expressions behave identically.

## Requirements:

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

The CLI builds on macOS, Linux, and Windows. Compile the core engine:

```bash
gcc ccal.c -o ccal.exe
```

To include updates to `help.txt`, regenerate the header before compiling:

```bash
xxd -i help.txt > help.h
```

This embeds the latest help text in the executable.

## Compile GUI:

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

Comma formatting is applied automatically while you type, and backspace/delete skip over commas so numbers stay nicely grouped.

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
