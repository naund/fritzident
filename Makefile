CC = gcc
CFLAGS= -Wall  
BINDIR = $(DESTDIR)/usr/sbin
NAME = fritzident

fritzident: debug.o main.o netinfo.o userinfo.o
	cc -o fritzident debug.o main.o netinfo.o userinfo.o -lsystemd

%.o: %.c
	$(CC) -c $(CFLAGS)  $<

install:
	install -d $(BINDIR)
	install --mode=755 $(NAME) $(BINDIR)/

clean:
	rm -f *.o fritzident

uninstall:
	rm $(BINDIR)/$(NAME)
