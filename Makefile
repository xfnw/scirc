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

check:
	@echo checking with cppcheck --enable=all
	@cppcheck --enable=all --suppress=missingIncludeSystem -DVERSION=test .

clean:
	@echo cleaning
	@rm -f scirc ${OBJ} scirc-${VERSION}.tar.zst

dist: clean
	@echo creating dist tarball
	@mkdir -p scirc-${VERSION}
	@cp -R LICENSE Makefile README.md config.mk scirc.c util.c scirc.1 scirc-${VERSION}
	@tar -caf scirc-${VERSION}.tar.zst scirc-${VERSION}
	@rm -rf scirc-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin ${DESTDIR}${PREFIX}/man/man1
	@cp -f scirc ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/scirc
	@cp -f scirc.1 ${DESTDIR}${PREFIX}/man/man1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/scirc

.PHONY: all options clean dist install uninstall
