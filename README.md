# SSSS

Shamir's Secret Sharing Scheme


## History

This is a fork of B. Poettering's SSSS ("Shamir's Secret Sharing Scheme").


## Intent

This fork exists to simplify the build process a bit, and make minor changes
to the tool that will facilitate its use in real-world usage scenarios.


## What You Need To Know

Please read B. Poettering's page on SSSS before proceeding.  SSSS, like all
cryptographic algorithms, can lead to a false sense of security if improperly
used.  Be sure you understand the role it serves, and the limitations it has
before employing it.

[http://point-at-infinity.org/ssss/index.html](http://point-at-infinity.org/ssss/index.html)


## Version

This fork is based on `ssss-0.5.tar.gz` (SHA1: 3f8f5046c2c5c3a2cf1a93f0a9446681852b190e).


## Changes

This fork will deviate from the original in the following ways:

1. The patch specified on the author's page to fix builds on some systems is
  applied.
2. ALL warnings/error messages/secondary output are directed to STDERR, and all
  shares/reconstructed secrets are directed to STDOUT.
3. Add option to make failed memlock attempts fatal.
4. Detailed instructions are included for MacOS X users on how to prepare their
  machine for building this tool.
5. A binary distribution for MacOS X will be provided.
6. Instead of just providing a SHA1 of the source, I will be using git signed
  tags to provide verifiability.


## Verification and Installation (VERY IMPORTANT!)

Before installing, when cloning this repo, use a signed tag and verify the
signature first!

```bash
# First, you need to make sure you have the signing key in your GPG keychain:
gpg --keyserver sks-keyservers.net --recv-key 7DCB12A7

# Now, clone the repo, and verify it:
git clone http://github.com/MrJoy/ssss.git
git tag -v v0.5.4
# You should see output to the effect of:
#   gpg: Signature made <date and time> using RSA key ID 7DCB12A7
#   gpg: Good signature from "Jon Frisby <jfrisby@mrjoy.com>"
#
# If you do NOT see 'Good Signature from "Jon Frisby <jfrisby@mrjoy.com>" and
# the RSA key ID 7DCB12A7 then do not proceed with installation!
#
git checkout v0.5.4 # This will put you in 'detached head' mode, which is fine
                    # but don't make changes as you are no longer on a branch!
# Proceed with build steps from here.  See `INSTALL.md` for details.
```

You can always find my GPG key signature at [MrJoy.com](http://MrJoy.com).


## Status

### Complete:

1. The patch specified on the author's page to fix builds on some systems is
  applied.
2. ALL warnings/error messages/secondary output are directed to STDERR, and all
  shares/reconstructed secrets are directed to STDOUT.
3. Add option to make failed memlock attempts fatal.
4. Detailed instructions are included for MacOS X users on how to prepare their
  machine for building this tool.
5. Instead of just providing a SHA1 of the source, I will be using git signed
  tags to provide verifiability.

### Not Implemented Yet:

1. A binary distribution for MacOS X will be provided.

## License

This fork follows the same license as the original software:  The GPLv2.  See
LICENSE for more information.
