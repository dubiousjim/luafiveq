.include "config"

SO_VERSION?= 1

NAMES= bitlib io pairs
PLUSNAMES= metafield iter err hash struct faststring

OBJS= ${NAMES:S/$/-${LUA_VERSION_NUM}.o/}
LIBS= ${NAMES:S/$/-${LUA_VERSION_NUM}.so/}
APIOBJ= api-${LUA_VERSION_NUM}.o
PLUSOBJS= ${PLUSNAMES:S/$/-${LUA_VERSION_NUM}.o/} apiplus-${LUA_VERSION_NUM}.o
PLUSLIBS= ${PLUSNAMES:S/$/-${LUA_VERSION_NUM}.so/}
GLUEOBJS= fiveq-${LUA_VERSION_NUM}.o fiveqplus-${LUA_VERSION_NUM}.o

# Targets [with defaults]
#
# all
# fiveq [LUA_VERSION_NUM=501]
# fiveqplus [LUA_VERSION_NUM=501]
# bitlib io pairs module metafield iter err hash struct faststring
# install install-501 install-502
# clean


all:
	@${MAKE} fiveq fiveqplus LUA_VERSION_NUM=501
	@${MAKE} fiveq fiveqplus LUA_VERSION_NUM=502

${NAMES} ${PLUSNAMES} fiveq fiveqplus: ${.TARGET}-${LUA_VERSION_NUM}.so ${.TARGET}-${LUA_VERSION_NUM}.a

${OBJS} ${PLUSOBJS} ${APIOBJ} ${GLUEOBJS}: ${.PREFIX:S/^/src\//S/-${LUA_VERSION_NUM}//}.c
	${CC} ${CFLAGS} -I src -o $@ -c ${.ALLSRC}

module-502.o:
	${CC} ${CFLAGS} -I src -o $@ -c src/module.c

moduleplus-${LUA_VERSION_NUM}.o:
	${CC} ${CFLAGS} -I src -DLUA_FIVEQ_PLUS -o $@ -c src/module.c

fiveq-501.a: ${OBJS} ${APIOBJ} ${.PREFIX}.o
	ar crs $@ $>

fiveqplus-501.a: ${OBJS} ${PLUSOBJS} ${.PREFIX}.o
	ar crs $@ $>

fiveq-502.a: ${APIOBJ} ${.PREFIX}.o
	ar crs $@ $>

fiveqplus-502.a: ${PLUSOBJS} ${.PREFIX}.o
	ar crs $@ $>

fiveq-501.so: ${OBJS} ${APIOBJ} ${.TARGET:S/.so$/.o/}
	${CC} ${LDFLAGS} -shared -Wl,-soname,${@:S/-${LUA_VERSION_NUM}//}.${SO_VERSION} -o $@ $>

fiveqplus-501.so: ${OBJS} ${PLUSOBJS} ${.TARGET:S/.so$/.o/} moduleplus-501.o
	${CC} ${LDFLAGS} -shared -Wl,-soname,${@:S/-${LUA_VERSION_NUM}//}.${SO_VERSION} -o $@ $>

fiveq-502.so: ${APIOBJ} ${.TARGET:S/.so$/.o/} module-502.o
	${CC} ${LDFLAGS} -shared -Wl,-soname,${@:S/-${LUA_VERSION_NUM}//}.${SO_VERSION} -o $@ $>

fiveqplus-502.so: ${PLUSOBJS} ${.TARGET:S/.so$/.o/} moduleplus-502.o
	${CC} ${LDFLAGS} -shared -Wl,-soname,${@:S/-${LUA_VERSION_NUM}//}.${SO_VERSION} -o $@ $>

install-${LUA_VERSION_NUM}: ${GLUEOBJS:S/.o$/.so/}
	install -d -m755 "${DESTDIR}${LUA_INCDIR}"
	install -m444 src/fiveq.h src/unsigned.h "${DESTDIR}${LUA_INCDIR}"
	install -d -m755 "${DESTDIR}${LUA_MODLIBDIR}"
	install -m444 fiveq-${LUA_VERSION_NUM}.a "${DESTDIR}${LUA_LIBDIR}/libfiveq.a"  # add -s to strip
	install -m444 fiveqplus-${LUA_VERSION_NUM}.a "${DESTDIR}${LUA_LIBDIR}/libfiveqplus.a"  # add -s to strip
	install -m444 fiveq-${LUA_VERSION_NUM}.so "${DESTDIR}${LUA_MODLIBDIR}/fiveq.so"  # add -s to strip
	install -m444 fiveqplus-${LUA_VERSION_NUM}.so "${DESTDIR}${LUA_MODLIBDIR}/fiveqplus.so"  # add -s to strip
	
install:
	@${MAKE} install-501 LUA_VERSION_NUM=501
	@${MAKE} install-502 LUA_VERSION_NUM=502

clean:
	rm -f core core.* *.o *.so *.a




uninstall-${LUA_VERSION_NUM}:
	rm -f "${DESTDIR}${LUA_INCDIR}/fiveq.h" "${DESTDIR}${LUA_INCDIR}/unsigned.h"
	rm -f "${DESTDIR}${LUA_LIBDIR}/"*fiveq*
	rm -f "${DESTDIR}${LUA_MODLIBDIR}/"*fiveq*

uninstall:
	@${MAKE} uninstall-501 LUA_VERSION_NUM=501
	@${MAKE} uninstall-502 LUA_VERSION_NUM=502

