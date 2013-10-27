CFLAGS=-O2

fritzident: debug.o main.o netinfo.o userinfo.o
	cc -o fritzident debug.o main.o netinfo.o userinfo.o

install: fritzident
	sh ./install.sh

clean:
	rm -f *.o fritzident
