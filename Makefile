INSTALL=install
PREFIX=/usr/local

reflink: reflink.c
	$(CC) $(LDFLAGS) -o $@ $<

install: reflink
	$(INSTALL) reflink $(PREFIX)/bin/reflink

