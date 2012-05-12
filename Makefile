include config

SO_VERSION?= 1

NAMES= bitlib io pairs
PLUSNAMES= metafield iter err hash struct faststring

OBJS= $(patsubst %,%-${LUA_VERSION_NUM}.o,${NAMES})
LIBS= $(patsubst %,%-${LUA_VERSION_NUM}.so,${NAMES})
APIOBJ= api-${LUA_VERSION_NUM}.o
PLUSOBJS= $(patsubst %,%-${LUA_VERSION_NUM}.o,${PLUSNAMES}) apiplus-${LUA_VERSION_NUM}.o
PLUSLIBS= $(patsubst %,%-${LUA_VERSION_NUM}.so,${PLUSNAMES})
GLUEOBJS= fiveq-${LUA_VERSION_NUM}.o fiveqplus-${LUA_VERSION_NUM}.o

# Targets [with defaults]
#
# all
# fiveq [LUA_VERSION_NUM=501]
# fiveqplus [LUA_VERSION_NUM=501]
# install install-501 install-502
# clean


all:
	@${MAKE} fiveq fiveqplus LUA_VERSION_NUM=501
	@${MAKE} fiveq fiveqplus LUA_VERSION_NUM=502

fiveq: fiveq-${LUA_VERSION_NUM}.so fiveq-${LUA_VERSION_NUM}.a

fiveqplus: fiveqplus-${LUA_VERSION_NUM}.so fiveqplus-${LUA_VERSION_NUM}.a

module-502.o:
	${CC} ${CFLAGS} -I src -o $@ -c src/module.c

moduleplus-${LUA_VERSION_NUM}.o:
	${CC} ${CFLAGS} -I src -DLUA_FIVEQ_PLUS -o $@ -c src/module.c

%-${LUA_VERSION_NUM}.o: src/%.c
	${CC} ${CFLAGS} -I src -o $@ -c $^

fiveq-501.a: ${OBJS} ${APIOBJ} fiveq-501.o
	ar crs $@ $^

fiveqplus-501.a: ${OBJS} ${PLUSOBJS} fiveqplus-501.o
	ar crs $@ $^

fiveq-502.a: ${APIOBJ} fiveq-502.o
	ar crs $@ $^

fiveqplus-502.a: ${PLUSOBJS} fiveqplus-502.o
	ar crs $@ $^

fiveq-501.so: ${OBJS} ${APIOBJ} fiveq-501.o
	${CC} ${LDFLAGS} -shared -Wl,-soname,${@:-${LUA_VERSION_NUM}.so=.so.${SO_VERSION}} -o $@ $^

fiveqplus-501.so: ${OBJS} ${PLUSOBJS} fiveqplus-501.o moduleplus-501.o
	${CC} ${LDFLAGS} -shared -Wl,-soname,${@:-${LUA_VERSION_NUM}.so=.so.${SO_VERSION}} -o $@ $^

fiveq-502.so: ${APIOBJ} fiveq-502.o module-502.o
	${CC} ${LDFLAGS} -shared -Wl,-soname,${@:-${LUA_VERSION_NUM}.so=.so.${SO_VERSION}} -o $@ $^

fiveqplus-502.so: ${PLUSOBJS} fiveqplus-502.o moduleplus-502.o
	${CC} ${LDFLAGS} -shared -Wl,-soname,${@:-${LUA_VERSION_NUM}.so=.so.${SO_VERSION}} -o $@ $^

install-${LUA_VERSION_NUM}: ${GLUEOBJS:.o=.so}
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
	rm -vif "${DESTDIR}${LUA_INCDIR}/fiveq.h" "${DESTDIR}${LUA_INCDIR}/unsigned.h"
	rm -vif "${DESTDIR}${LUA_LIBDIR}/"*fiveq*
	rm -vif "${DESTDIR}${LUA_MODLIBDIR}/"*fiveq*

uninstall:
	@${MAKE} uninstall-501 LUA_VERSION_NUM=501
	@${MAKE} uninstall-502 LUA_VERSION_NUM=502

