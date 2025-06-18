# ccal

Simple GUI (*Windows OS only*) and command line calculator, allowing for arithmetic equations.
This project was mainly utilized to learn `c`, but turned out to be pretty 
useful as it allows for long mathmematicl expressions.

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

## Compile Command Line Tool:

**NOTE** - Compatible with Mac OS, Linux, and Windows.

### Clone Repo:

First clone this repository, and change into the directory:

```
git clone https://github.com/jhauga/ccal.git
cd ccal
```

### Compile:

To use the command line tool, compile engine ccal.c:

```
gcc ccal.c -o ccal.exe
```

and done.

## Compile GUI:

**NOTE** - Compatible with Windows only.

### Clone Repo:

First clone this repository, and change into the directory:

```
git clone https://github.com/jhauga/ccal.git
cd ccal
```

### Compile Without Resources:

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

## Command Line Usage:

To use as a command line tool; pass digits, arithmetic operators, and nesting characters 
(`(`, `)`, `{`, `}`, `[`, `]`) separated with a space.

**IMPORTANT NOTE** - use `x` instead of `*` for multiplcation.

### Command Line Use Examples:

**NOTE** - if errors occure with nest characters, escape or wrap in double quotes.

Add two numbers:

    > ccal 1 + 1
    > 2

Perform a complex equation:

    > ccal ( ( 34 x 11 ) / ( 10 - 5 ) ) - ( 4 x 53 x ( 30 / 10 ) )
    > -561.2

**NOTE** - issue with order of operations, so something such as:

    > ccal ( ( 34 x 11 ) / [ 10 - 5 ] ) - ( 4 x 53 x ( 30 / 10 ) + ( 2 x ( 40 - 20 ) ) ) / ( 8 x 6 x ( 12 / 2 ) + 4 )
    > -2.0589

will be incorrect.