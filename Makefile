all: compile doc

compile: ssss-split ssss-combine

doc: ssss.1 ssss.1.html

ssss-split: ssss.c
	$(CC) -W -Wall -O2 -o ssss-split ssss.c -lgmp
	strip ssss-split

ssss-combine: ssss-split
	ln -f ssss-split ssss-combine

ssss.1: ssss.manpage.xml
	if [ `which xmltoman` ]; then xmltoman ssss.manpage.xml > ssss.1; else echo "WARNING: xmltoman not found, skipping generate of man page."; fi
	if [ -e ssss.1 ]; then cp ssss.1 ssss-split.1; cp ssss.1 ssss-combine.1; fi

ssss.1.html: ssss.manpage.xml
	if [ `which xmlmantohtml` ]; then xmlmantohtml ssss.manpage.xml > ssss.1.html; else echo "WARNING: xmlmantohtml not found, skipping generation of HTML documentation."; fi

clean:
	rm -rf ssss-split ssss-combine ssss.1 ssss-split.1 ssss-combine.1 ssss.1.html

install:
	if [ -e ssss.1 ]; then install -o root -g wheel -m 644 ssss.1 ssss-split.1 ssss-combine.1 /usr/share/man/man1; else echo "WARNING: No man page was generated, so none will be installed."; fi
	install -o root -g wheel -m 755 ssss-split ssss-combine /usr/bin

lint:
	splint +unix-strict-lib +skip-sys-headers -preproc -unrecog '-Dmpz_t=void*' -incondefs -usedef -noeffect -type -mustdefine *.c

test: compile
	./test.sh
