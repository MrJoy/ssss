# Installing

## Prerequisites

1. You will need to be on a system with a proper `/dev/random`.
2. You will need a proper C toolchain (e.g. GCC).
3. You will need libgmp to compile the binaries.
4. You will need [xmltoman](http://sourceforge.net/projects/xmltoman/) to generate man pages.  If you do not have this, the build will fail but you can ignore that and proceed.

## Compiling on MacOS X

These instructions are appropriate to OS X 10.7 (Lion).

1. Ensure you have installed XCode 4.x.
2. Install [MacPorts](http://www.macports.org/).
3. Run `sudo port install gmp` to install libgmp.
4. _TODO: Installation instructions for xmltoman..._
5. By default, the build process won't see libgmp, so you will need to compile with this commandline:

        LIBRARY_PATH=$LIBRARY_PATH:/opt/local/lib C_INCLUDE_PATH=$C_INCLUDE_PATH:/opt/local/include make
