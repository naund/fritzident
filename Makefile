CC ?= gcc
CFLAGS ?= -Wall -O2 
LDFLAGS +=  `pkg-config --libs libsystemd`  


BINDIR = $(DESTDIR)/usr/sbin
SYSTEMDDIR = /lib/systemd/system
MANDIR = $(DESTDIR)/usr/share/man/man8
NAME = fritzident



fritzident: debug.o main.o netinfo.o userinfo.o
	cc -o fritzident debug.o main.o netinfo.o userinfo.o $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS)  $<

install-man:
	install -d -m644 fritzident.8 $(MANDIR)/fritzident.8

install-systemd:
	install -d -m644 fritzident.service $(SYSTEMDDIR)/fritzident.service	
	install -d -m644 fritzident.socket $(SYSTEMDDIR)/fritzident.socket	

install-bin:
	install -d $(BINDIR)
	install --mode=755 $(NAME) $(BINDIR)/

install: install-man install-systemd install-bin

clean:
	rm -f *.o fritzident

uninstall:
	rm $(BINDIR)/$(NAME)
