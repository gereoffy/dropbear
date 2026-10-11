
#Build: make PROGRAMS=dropbear, a kliens/dropbearkey/scp nem is fordul. Ha a --enable-static s a -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE -pie flageket is hozzteszed a CFLAGS-hoz, a reexec miatti ASLR tnyleg r valamit.

#./configure --enable-harden --enable-bundled-libtom --disable-zlib --disable-pam --disable-lastlog --disable-utmp --disable-utmpx --disable-wtmp --disable-wtmpx
# make STATIC=1 PROGRAMS=dropbear

./configure --enable-harden --enable-bundled-libtom --disable-zlib --disable-pam --disable-lastlog --disable-utmp --disable-utmpx --disable-wtmp --disable-wtmpx CFLAGS="-O2"
make clean
make PROGRAMS=dropbear
strip dropbear

