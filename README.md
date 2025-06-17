# ccal

Simple GUI and command line calculator, allowing for arithmetic equations.
This project was mainly utilized to learn `c`, but turned out to be pretty 
useful as it allows for long mathmematicl expressions.

## Requirements:

For Apple, Linux, and Windows the GNU compile tool `gcc` is required.
**NOTE** - for Windows `cygwin` will be used to install gcc.

### Apple:

Using `Homebrew` to install.

```
brew install gcc
```

### Linux:

Using common distro package manager `apt`.

```
sudo apt update 
sudo apt install build-essential
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

then follow the prompts until you reach the "Select Packages" window of the GUI installer. 

**NOTE** - if you do not see the `gcc-core` package in the results, perform a search for `gcc-core`,
then select it, click next, and finish the install process.


## Compile GUI:

### Simpliest Method:

This compiles without any resources.

```
gcc -DBUILDING_GUI ccal.c ccal_gui.c -o ccal_gui.exe -mwindows
```

### Include Resources:

Compile with resources (*app icon*).

Compile `.rc` to get a `.res` object.

```
windres ccal_gui.rc -O coff -o ccal_gui.res
```

Build, linking resource object.

```
gcc -DBUILDING_GUI ccal_gui.c ccal.c ccal_gui.res -o ccal_gui.exe -mwindows
```

## Compile Command Line Tool:

```
gcc ccal.c -o ccal.exe
```

## Command Line Usage:

To use as a command line tool; pass digits, arithmetic operators, and nesting characters 
(`(`, `)`, `{`, `}`, `[`, `]`) separated with a space.

**IMPORTANT NOTE** - use `x` instead of `*` for multiplcation.

### Command Line Use Examples:

Add two numbers:

    > ccal 1 + 1
    > 2

Perform a complex equation:

    > ccal ( ( 34 x 11 ) / [ 10 - 5 ] ) - ( 4 x 53 x ( 30 / 10 ) + ( 2 x ( 40 - 20 ) ) ) / ( 8 x 6 x ( 12 / 2 ) + 4 )
    > -2.0589