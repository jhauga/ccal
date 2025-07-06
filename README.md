# ccal![repo icon](assets/icon.png)

Simple GUI (*Windows OS only*) and command line calculator, allowing for long mathematical expressions.

## Requirements:

For Apple, Linux, and Windows the GNU compile tool `gcc` is required.

**NOTE** - for Windows `cygwin` will be used to install gcc.

### Apple:

**NOTE** - command line tool only.

Using `Homebrew` to install.

```
brew install gcc
```

### Linux:

**NOTE** - command line tool only.

Using common distro package manager `apt`.

```
sudo apt update 
sudo apt install build-essential
```

To write changes to `help.txt`, you'll need `xxd`.

```
sudo apt install xxd
```

### Windows:

First use Windows package manager `winget` to install `cygwin`.

```
winget install Cygwin.Cygwin
```

Next download the makeshift package manager [setup-x86_64.exe](https://cygwin.com/install.html) for 
`cygwin`, moving it to `/usr/bin` or another location where it can be called from command line, 
and install gcc using:

```
setup-x86_64.exe -P gcc-core
```

To write changes to `help.txt`, you'll need `xxd`.

```
setup-x86_64.exe -P xxd
```

then follow the prompts until you reach the "Select Packages" window of the GUI installer. 

**NOTE** - if you do not see the `gcc-core` package in the results, perform a search 
for `gcc-core`, then select it, click next, and finish the install process.

## Compile Command Line Tool:

**NOTE** - Compatible with Mac OS, Linux, and Windows.

To use the command line tool, compile engine ccal.c:

```
gcc ccal.c -o ccal.exe
```

and done.

**NOTE** - to include changes to `help.txt`, run:

```
xxd -i help.txt > help.h
```

before compiling.

## Compile GUI:

**NOTE** - Compatible with Windows only.

### Clone Repo:

First clone this repository, and change into the directory:

```
git clone https://github.com/jhauga/ccal.git
cd ccal
```

### Simplest Compile Method: 

This compiles without any resources:

```
gcc -DBUILDING_GUI ccal.c ccal_gui.c -o ccal_gui.exe -mwindows
```

and done.

### Compile Including Resources:

Compile with resources (*app icon*).

Compile `.rc` to get a `.res` object.

```
windres ccal_gui.rc -O coff -o ccal_gui.res
```

Then - build, linking resource object:

```
gcc -DBUILDING_GUI ccal_gui.c ccal.c ccal_gui.res -o ccal_gui.exe -mwindows
```

and done.

## GUI Usage:

**NOTE** - Windows only.

To use the GUI version of the application, double click or call the compiled
file. Once the window opens, click the buttons or use the keyboard to input
the mathematical expression. Press `Enter` or click `=` to calculate the
expression. Press `d` to clear the contents of the expression and calculation.
Use `ctrl + c` to copy the calculation to the clipboard.

## Command Line Usage:

To use as a command line tool; pass digits, arithmetic operators, and nesting characters 
(`(`, `)`, `{`, `}`, `[`, `]`) separated with a space.

**IMPORTANT NOTE** - use `x` instead of `*` for multiplication when `-q, --quote` is not used. 

**IMPORTANT NOTE** - use `p` instead of `^` for exponentiation when `-q, --quote` is not used. 

###  Acceptable Arithmetic Operators:

    +  ->  addition
    -  ->  subtraction
    /  ->  division
    x  ->  multiplication
    *  ->  multiplication (NOTE - use -q, --quote to use)
    p  ->  exponentiation (power of)
    ^  ->  exponentiation (NOTE - use -q, --quote to use) 

### Command Line Use Examples:

To use the command line tool either input all elements of the equation separated with a space
character, or use the quote option `-q, --quote` which does not require elements to be separated
with a space character.

**NOTE** - if errors occur with nest characters, escape or wrap in double quotes. 

**NOTE** - best to use `[` and `]` as nest characters

Add two numbers:

    > ccal 1 + 1
    > 2

Perform a complex equation:

    > ccal [ [ 34 x 11 ] / [ 10 - 5 ] ] - [ 4 x 53 x [ 30 / 10 ] ]
    > -561.2

**NOTE** - issue with order of operations, so something such as:

    > ccal [ [ 34 x 11 ] / [ 10 - 5 ] ] - [ 4 x 53 x [ 30 / 10 ] + [ 2 x [ 40 - 20 ] ] ] / [ 8 x 6 x [ 12 / 2 ] + 4 ]
    > -2.058904109589041

will be incorrect (*correct is `72.4849`*). So instead, use the quote option `-q, --quote` (*suggested*), 
or manually apply order of operations with nesting characters to force the correct order of operations:

#### Quote Option (*suggested*):

    > ccal -q "[[34x11]/[10-5]]-[4x53x[30/10]+[2x[40-20]]]/[8x6x[12/2]+4]"
    > 72.48493150684931
    
#### Manually Apply Order of Operations:
    
    > ccal [ [ 34 x 11 ] / [ 10 - 5 ] ] - [ [ [ 4 x 53 ] x [ 30 / 10 ] + [ 2 x [ 40 - 20 ] ] ] / [ 8 x 6 x [ 12 / 2 ] + 4 ] ]
    > 72.48493150684931

in order to get the correct answer.

**NOTE** - terminal handling of command line arguments requires numbers with commas to be wrapped 
in double quotes, but this can be bypassed using the quote option `-q, --quote`. So instead of:

    > ccal 1,000 - 1
    > Error: Invalid expression

use the quote option `-q, --quote` (*suggested*), or manually wrap comma numbers:

#### Quote Option (*suggested*):

    > ccal --quote "2,000-1,000"
    > 1000
    
#### Manually Wrap Comma Numbers:

    > ccal "2,000" - "1,000"
    > 1000

to get the correct calculation.

#### Calculate Exponentiation:

To calculate the exponentiation (*power of*) use `p` as the operator. If using the `-q, --quote`
option, both the `p` and `^` character can be used as the operator.

    > ccal 2 p 2
    > 4

#### Quote Option (*suggested*):

    > ccal --quote "2^2"
    > 4