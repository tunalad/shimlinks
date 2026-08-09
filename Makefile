# shimlinks - run commands with dotfile/state dirs pointing somewhere else
# See LICENSE file for copyright and license details.

include config.mk

SRC = main.cpp yaml/Yaml.cpp util.cpp config.cpp resolve.cpp install.cpp
OBJ = ${SRC:.cpp=.o}
TARGET = shimlinks
MANPAGE = ${TARGET}.1

all: ${TARGET}

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

yaml/Yaml.o: yaml/Yaml.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# comment out if building on OpenBSD
-include ${OBJ:.o=.d}

$(OBJ): config.mk

${TARGET}: ${OBJ}
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

clean:
	rm -f ${TARGET} ${OBJ} ${OBJ:.o=.d} ${TARGET}-${VERSION}.tar.gz

dist: clean
	mkdir -p ${TARGET}-${VERSION}
	cp -R LICENSE Makefile config.mk README.md ${MANPAGE} main.cpp yaml util.cpp config.cpp resolve.cpp install.cpp config.hpp util.hpp resolve.hpp install.hpp ${TARGET}-${VERSION}
	tar -cf ${TARGET}-${VERSION}.tar ${TARGET}-${VERSION}
	gzip ${TARGET}-${VERSION}.tar
	rm -rf ${TARGET}-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${TARGET} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${TARGET}
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" ${MANPAGE} > ${DESTDIR}${MANPREFIX}/man1/${MANPAGE}
	rm -f ${DESTDIR}${MANPREFIX}/man1/${MANPAGE}.gz
	gzip -9 ${DESTDIR}${MANPREFIX}/man1/${MANPAGE}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${TARGET}
	rm -f ${DESTDIR}${MANPREFIX}/man1/${MANPAGE}.gz

.PHONY: all clean dist install uninstall
