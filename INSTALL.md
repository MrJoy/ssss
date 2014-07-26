# Installing

## Prerequisites

1. You will very much want to be on a system with a proper `/dev/random`.
2. A proper C toolchain (e.g. GCC) is required.
3. `libgmp` is required.
4. [xmltoman](http://sourceforge.net/projects/xmltoman/) is optional, but
  without it, no man pages will be produced.


## Unix-like OSes

1. Ensure you have a proper C compiler (e.g. GCC).
2. Ensure you have `libgmp` installed.
3. Ensure you have [xmltoman](http://sourceforge.net/projects/xmltoman/)
  installed, to generate man pages.  If you do not have this, the build will
  produce a warning but you can ignore that and proceed.
4. Run `make`
5. Run `sudo make install`


## MacOS X

These instructions have been tested on OS X from 10.7 (Lion), through 10.10 (Yosemite).

### Basics:

1. Ensure you have installed the latest XCode, including the Command Line Tools.
2. Install [MacPorts](http://www.macports.org/), or [HomeBrew](http://mxcl.github.io/homebrew/).

### MacPorts:

1. Run `sudo port install gmp` to install `libgmp`.
2. _TODO: Installation instructions for `xmltoman`, since it's not available via Macports..._
3. By default, the build process may not see libgmp, so you will need to
  compile with this commandline:

    `LIBRARY_PATH=$LIBRARY_PATH:/opt/local/lib C_INCLUDE_PATH=$C_INCLUDE_PATH:/opt/local/include make`

### Homebrew:

1. Run `brew install gmp xmltoman` to install `libgmp` and `xmltoman`
2. Run `make`

### Installation:

1. Run `sudo make install`
