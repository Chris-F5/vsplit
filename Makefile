CC=gcc

vsplit: vsplit.c
	$(CC) $^ -o $@

.PHONY: install

install: vsplit
	mkdir -p $(DESTDIR)/bin
	cp -f vsplit $(DESTDIR)/bin
	chmod 775 $(DESTDIR)/bin/vsplit
