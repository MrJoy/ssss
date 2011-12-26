# SSSS

Shamir's Secret Sharing Scheme

## History

This is a fork of B. Poettering's SSSS ("Shamir's Secret Sharing Scheme").

## Intent

This fork exists to simplify the build process a bit, and make minor changes
to the tool that will facilitate its use in real-world usage scenarios.

## Original Author

Please read B. Poettering's page on SSSS before proceeding.

http://point-at-infinity.org/ssss/index.html

## Version

This fork is based on ssss-0.5.tar.gz (SHA1: 3f8f5046c2c5c3a2cf1a93f0a9446681852b190e).

## Changes

This fork will deviate from the original in the following ways:

1. The patch specified on the author's page to fix builds on some systems is applied.
2. ALL warnings/error messages/secondary output are directed to STDERR, and all shares/reconstructed secrets are directed to STDOUT.
3. Add option to make failed memlock attempts fatal.
4. Detailed instructions are included for MacOS X users on how to prepare their machine for building this tool.
5. A binary distribution for MacOS X will be provided.
6. Instead of just providing a SHA1 of the source, I will be using git signed tags to provide verifiability.

## Status

### Complete:

1. The patch specified on the author's page to fix builds on some systems is applied.
2. ALL warnings/error messages/secondary output are directed to STDERR, and all shares/reconstructed secrets are directed to STDOUT.
3. Add option to make failed memlock attempts fatal.

### Not Implemented Yet:

1. Detailed instructions are included for MacOS X users on how to prepare their machine for building this tool.
2. A binary distribution for MacOS X will be provided.
3. Instead of just providing a SHA1 of the source, I will be using git signed tags to provide verifiability.

## License

This fork follows the same license as the original software:  The GPLv2.  See LICENSE for more information.
