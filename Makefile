ssss: ssss.c
	gcc -W -Wall -O2 -lgmp -o ssss ssss.c && strip ssss

clean:
	rm -rf ssss
