# History

## v0.5.6: (February 2017)

* Switch to using `/dev/urandom`.
* Fix memory lock error condition handling.
* Fix missing intialize buffer.
* More documentation / comment improvements.


## v0.5.5: (February 2016)

* Fix build for picky compilers.  (Not relevant to clang, maybe relevant to gcc?)
* Documentation / source comment improvements.
* Fix domain name for keyserver in readme.


## v0.5.4: (October 2013)

* Minor Makefile fix.
* Add newlines to option display.


## v0.5.3: (April 2013)

* Corrected an error in the docs.
* Converted changelog to Markdown.


## v0.5.2: (April 2013)

* Add finer-grained build tasks (`make compile`, and `make doc`).
* Add `make install` task.
* Update installation instructions to have some general advice for normal
  Unix-users, and to be more up to date for OS X users.


## v0.5.1: (December 2011)

* Incorporate B. Poettering's compilation fix, via ifdef.  Set
  `USE_RESTORE_SECRET_WORKAROUND` if you encounter an error about the
  definition of "restore_secret".
* Added "-M" option to require a memory lock.


## v0.5: (January 2006)

* introduction of memory locking and tty echo suppression
* a buffer overflow was fixed. It seems to be exploitable.


## v0.4: (October 2005)

* the security level now can be chosen freely in the range 8..1024 bits in
  steps of 8.


## v0.3: (July 2005)

* separation of ssss into ssss-split and ssss-combine
* introduction of a man page


## v0.2: (June 2005)

* introduction of the diffusion layer


## v0.1: (May 2005)

* initial release
