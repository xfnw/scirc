# scirc

include config.mk

SRC = scirc.c
OBJ = ${SRC:.c=.o}

all: options scirc

options:
	@echo scirc build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk util.c

scirc: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f scirc ${OBJ} scirc-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p scirc-${VERSION}
	@cp -R LICENSE Makefile README config.mk scirc.c util.c scirc-${VERSION}
	@tar -cf scirc-${VERSION}.tar scirc-${VERSION}
	@gzip scirc-${VERSION}.tar
	@rm -rf scirc-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f scirc ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/scirc
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/scirc
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1

.PHONY: all options clean dist install uninstall
