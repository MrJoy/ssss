all: ssss-split ssss-combine ssss.1 ssss.1.html

ssss-split: ssss.c
	$(CC) -W -Wall -O2 -lgmp -o ssss-split ssss.c
	strip ssss-split

ssss-combine: ssss-split
	ln -f ssss-split ssss-combine

ssss.1: ssss.manpage.xml
	if [ `which xmltoman` ]; then xmltoman ssss.manpage.xml > ssss.1; else echo "WARNING: xmltoman not found, skipping generate of man page."; fi

ssss.1.html: ssss.manpage.xml
	if [ `which xmlmantohtml` ]; then xmlmantohtml ssss.manpage.xml > ssss.1.html; else echo "WARNING: xmlmantohtml not found, skipping generation of HTML documentation."; fi

clean:
	rm -rf ssss-split ssss-combine ssss.1 ssss.1.html
