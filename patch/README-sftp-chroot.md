# dropbear sftp chroot + internal-sftp patchek

Az OpenSSH `Match Group sftponly / ChrootDirectory %h / ForceCommand internal-sftp`
megfelelője dropbearhez. A dropbear 2026.94 forrására készült, két független
patchben. A fejlesztés a github forkban megy, a `sftp-chroot` és az
`internal-sftp` branchen (https://github.com/gereoffy/dropbear), innen készülnek
a `0001-*.patch` fájlok `git format-patch`-csel. A régi `dropbear-*.patch`
fájlok az első változatok, már nem ezeket kell használni.

| Patch | Mit ad | Kapcsoló |
|---|---|---|
| `0001-Add-sftp-chroot-for-members-of-a-group.patch` | a csoport tagjai a home-jukba chrootolva csak sftp-t használhatnak | `DROPBEAR_SFTP_CHROOT` |
| `0001-Add-a-built-in-sftp-server-like-OpenSSH-internal-sft.patch` | beépített SFTP v3 szerver, `"internal-sftp"` parancsnévvel | `DROPBEAR_SFTP_INTERNAL` |

Mindkét patch önállóan is felmegy a tiszta forrásra, és együtt is, bármelyik
sorrendben. A két sorrend azonos forrást ad. Ha a kapcsoló nincs bekapcsolva,
a dropbear változatlanul fordul, és a patch kódja nem kerül a binárisba.

Mindkét kapcsoló a dropbear szokásos módján működik: a `default_options.h`-ban
0 az alapértéke, a `localoptions.h`-ban kell 1-re állítani. A dropbear lintje
(`make check`) nem enged `#ifdef`-et a `DROPBEAR_*` opciókra, ezért nem a
csoportnév definiálása kapcsolja be a chrootot. A csoportnév és a szerver
útvonal alapértéke `"sftponly"` és `SFTPSERVER_PATH`, csak akkor kell megadni,
ha ettől eltérő.

| Konfiguráció | Patchek | `localoptions.h` |
|---|---|---|
| chroot, külső programmal | chroot | `DROPBEAR_SFTP_CHROOT 1`<br>`DROPBEAR_SFTP_CHROOT_SERVER "/.sftpserver"` |
| beépített sftp, chroot nélkül | internal | `DROPBEAR_SFTP_INTERNAL 1`<br>`SFTPSERVER_PATH "internal-sftp"` |
| chroot, beépített sftp-vel | mindkettő | `DROPBEAR_SFTP_CHROOT 1`<br>`DROPBEAR_SFTP_CHROOT_SERVER "internal-sftp"`<br>`DROPBEAR_SFTP_INTERNAL 1` |

A két patch nem tud egymásról, csak a `"internal-sftp"` parancsnéven keresztül
kapcsolódnak. A chroot patch a chrootolt user parancsát mindig
`DROPBEAR_SFTP_CHROOT_SERVER`-re állítja. Az internal patch minden olyan
parancsot, ami pontosan `"internal-sftp"`, a beépített szerverrel szolgál ki,
még a chroot ág exec-je előtt. Ha a `DROPBEAR_SFTP_CHROOT_SERVER`
`"internal-sftp"`, de az internal patch hiányzik vagy a `DROPBEAR_SFTP_INTERNAL`
0, a dropbear egy `internal-sftp` nevű programot próbál indítani. Ez nem
sikerül, a session `Child failed` hibával áll le.

## 1. chroot patch (`sftp-chroot` branch)

Kódja teljes egészében `#if DROPBEAR_SFTP_CHROOT` alatt van. A `sysoptions.h`
`#error`-t ad, ha `DROPBEAR_SFTPSERVER` 0, mert akkor a chrootolt user semmit
sem tudna csinálni.

1. **Sikeres auth után, még rootként** (`svr-auth.c`, `send_msg_userauth_success`,
   a `svr_switch_user()` előtt): ha a user tagja a `DROPBEAR_SFTP_CHROOT_GROUP`
   csoportnak (elsődleges gid vagy `/etc/group` tagság), a dropbear ellenőrzi,
   hogy a home root tulajdonú és nem group/other írható. Még a chroot előtt
   `initgroups()`-szal beállítja a kiegészítő csoportokat és `tzset()`-tel
   betölti az időzónát, mert a chrootban nincs `/etc/group` és
   `/etc/localtime`. Ezután `chroot(home)`. Ha a home nem felel meg, a
   kapcsolat bontódik és syslogba kerül az ok. A `pw_dir` ezután `/`, a userre
   váltás a megszokott módon történik, csak az `initgroups()` marad ki, hogy
   ne törölje a már beállított csoportokat.
2. **Session kérések** (`svr-chansession.c`): chrootolt usernek csak az `sftp`
   subsystem engedett. Shell, exec, pty kérés elutasítva. A session gyerek
   kihagyja a `seedrandom()` újraseedelést, mert a chrootban nincs
   `/dev/urandom`, és az sftp szervernek nem kell a PRNG.
3. **Parancs**: a chrootolt user parancsa a forced command feldolgozása után
   mindig `DROPBEAR_SFTP_CHROOT_SERVER` lesz. Sem `-c`, sem az authorized_keys
   `command=` opciója nem írhatja felül, és a `LOG_COMMANDS` is ezt logolja.
4. **Forwarding**: a `svr_sftp_chroot_user()` makró (`auth.h`, a kapcsoló
   nélkül 0-ra fordul) alapján a `-L` és `-R` tcp forward, a unix stream
   forward mindkét iránya, az agent és az X11 forward is elutasítva chrootolt
   usernek (`svr-tcpfwd.c`, `svr-streamfwd.c`, `svr-agentfwd.c`, `svr-x11fwd.c`).
5. **Exec**: shell nélkül, közvetlenül `execv(DROPBEAR_SFTP_CHROOT_SERVER)`.
   Külső programnál a chrootban nincs libc, ezért a binárisnak statikusnak kell
   lennie (lásd lent). Az internal patch-csel ez a lépés el sem érhető.
6. **Syslog** (`dbutil.c`): `openlog` `LOG_NDELAY` flaggel, hogy a `/dev/log`
   socket a chroot előtt nyíljon, különben a chroot utáni logsorok elvesznének.

Nem chrootolt userekre semmi nem változik.

Eltérés a régi `dropbear-sftp-chroot.patch`-től: a `DROPBEAR_SFTP_CHROOT`
kapcsoló, a chroot előtti `initgroups()` és `tzset()`, a `seedrandom()`
kihagyása, és a 4. pont szerinti teljes forwarding-tiltás. A v1
(`dropbear-sftp-chroot.patch.v1`) még a subsystem ágban cserélte az
útvonalat, a 3. pont szerinti felülírás a log miatt jobb.

## 2. internal-sftp patch (`internal-sftp` branch)

Új fájlok: `src/svr-sftp.c`, `src/svr-sftp.h`. A `Makefile.in`-be bekerül a
`svr-sftp.o`, a `default_options.h`-ba a `DROPBEAR_SFTP_INTERNAL 0`. Az
`svr-chansession.c` `execchild()` függvénye a chdir után, exec helyett
`svr_sftp_internal()`-t hív, ha a parancs pontosan `"internal-sftp"`.

A parancs bárhonnan jöhet:
- `SFTPSERVER_PATH "internal-sftp"`: az sftp subsystem;
- `-c internal-sftp` vagy `command="internal-sftp"` az authorized_keys-ben;
- `DROPBEAR_SFTP_CHROOT_SERVER "internal-sftp"` (a chroot patch-csel).

Ha a `SFTPSERVER_PATH` marad egy külső program, a normál userek azt kapják.
A beépített szerver ilyenkor csak akkor fut, ha valaki kifejezetten az
`internal-sftp` parancsot kéri. Ez a user jogaival fut, és nem ad többet
annál, amit az sftp subsystem amúgy is ad.

A szerver tulajdonságai:

- SFTP protokoll v3 (draft-ietf-secsh-filexfer-02, amit az OpenSSH is beszél),
  minden v3 kérés: open/close/read/write, stat/lstat/fstat, setstat/fsetstat,
  opendir/readdir, remove, mkdir/rmdir, realpath, rename, readlink, symlink.
- OpenSSH kiterjesztések: `posix-rename`, `statvfs`/`fstatvfs` (`df`),
  `hardlink` (`ln`), `fsync`, `limits` (az újabb OpenSSH kliens ebből tudja,
  hogy 255 KB-os read/write kéréseket küldhet).
- A `realpath` az OpenSSH-hoz hasonlóan megengedi, hogy az utolsó
  útvonalkomponens hiányozzon. Az OpenSSH kliens `put -r`-je erre épít, a sima
  libc `realpath()`-szal a rekurzív feltöltés nem működik.
- APPEND-del nyitott fájlba `write()`-tal ír, nem `pwrite()`-tal, mert Linuxon
  az `O_APPEND` mellett a `pwrite()` figyelmen kívül hagyja az offsetet.
- Csak libc-t használ, saját, határellenőrzött csomagparserrel. Hibás csomagnál
  (rossz hossz, csonka string, NUL a path-ban) a folyamat kilép. Max. 256 KB
  csomag, 256 nyitott handle.
- A chmod-ból a setuid/setgid bitek ki vannak maszkolva.
- A FIFO-k `O_NONBLOCK`-kal nyílnak, így egy FIFO nem akaszthatja meg a szervert.
- Indulás előtt a gyerekfolyamat törli a session kulcsokat a memóriából
  (a hostkey-t és a prng állapotát a dropbear már amúgy is törli), és lezárja
  a hálózati socketet és a többi csatorna fd-it. A syslog AF_UNIX socketje
  nyitva marad.
- Chrootban: owner/group számként jelenik meg (`ls -l`), mert nincs passwd.
  Az idő UTC, mert nincs `/etc/localtime`.
- Logolás csak syslogba megy (`-E` módban a gyerek stderr-je a kliens
  csatornája, oda nem írunk). Ha `LOG_COMMANDS 1`, minden módosító művelet
  logolódik (open flagekkel, close olvasott/írt bájtszámmal, remove, rename,
  mkdir, rmdir, symlink, hardlink, setstat), a session végén pedig az
  összesítés.
- A `DROPBEAR_VFORK`-kal (uClinux, MMU nélkül) nem használható, ezt a build
  `#error`-ral jelzi.

Nem támogatott: SFTP v4+ (a kliensek automatikusan v3-ra állnak), `copy-data`,
`expand-path`, `users-groups-by-id` (ezek nélkül az OpenSSH kliens hiba nélkül
működik).

## Build

```
cd dropbear-2026.94
patch -p1 < 0001-Add-sftp-chroot-for-members-of-a-group.patch
patch -p1 < 0001-Add-a-built-in-sftp-server-like-OpenSSH-internal-sft.patch
cp localoptions.h .        # benne:
#   #define DROPBEAR_SFTP_INTERNAL 1
#   #define DROPBEAR_SFTP_CHROOT 1
#   #define DROPBEAR_SFTP_CHROOT_SERVER "internal-sftp"
./configure --enable-bundled-libtom --disable-zlib --disable-pam \
  --disable-lastlog --disable-utmp --disable-utmpx --disable-wtmp --disable-wtmpx CFLAGS="-O2"
make PROGRAMS=dropbear
```

### Csak chroot, külső sftp szerverrel

Csak a chroot patch kell, a `localoptions.h`-ban `DROPBEAR_SFTP_CHROOT 1` és
`DROPBEAR_SFTP_CHROOT_SERVER "/.sftpserver"`. A statikus gesftpserver (Green
End SFTP Server):

```
cd sftpserver-2
./configure
make gesftpserver LDFLAGS=-static CFLAGS="-O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2"
```

A linker NSS warningjai (`getpwuid` stb.) normálisak: a chrootban nincs passwd,
a fájlok tulajdonosa számként jelenik meg.

## User beállítása

```
groupadd sftponly
usermod -aG sftponly USER
chown root:root /home/USER
chmod 755 /home/USER             # root tulajdon, nem írható, de olvasható
mkdir /home/USER/files
chown USER /home/USER/files      # ide írhat
```

A home a chroot gyökere: a user `/`-ként látja, a `files/` az írható része.
Az internal-sftp használatakor a chrootba nem kell sem bináris, sem `/dev`,
sem `/etc`, a régi `/home/USER/.sftpserver` binárisok törölhetők.

Külső szervernél:

```
cp gesftpserver /home/USER/.sftpserver   # root:root 755, statikus
```

Több usernél a `.sftpserver` lehet ugyanaz a fájl hardlinkkel.

## Ellenőrzés

```
sftp USER@host          # ls, cd files, put, get, df: megy
ssh USER@host           # elutasítva
ssh -L 1234:x:22 USER@host -N   # elutasítva
```

Syslogban: `User USER chrooted to '/home/USER'`, majd
`User USER executing 'internal-sftp'`, `sftp session opened for user USER`, és a
végén `sftp session closed for user USER: read N bytes, wrote M bytes`.
Külső szervernél: `User USER executing '/.sftpserver'`.

### A protokollkód tesztelése önállóan

A `svr-sftp.c` dropbear nélkül is lefordul, így bármelyik gépen, root nélkül
tesztelhető az OpenSSH klienssel:

```
cc -DSFTP_STANDALONE -O2 -Wall -o sftp-test src/svr-sftp.c
sftp -D ./sftp-test        # az aktuális könyvtáron, chroot nélkül
```

## Új dropbear kiadásnál

A forkban a két branch rebase-elhető az upstream `master`-re, vagy a
patchekkel:

```
patch -p1 --dry-run < 0001-Add-sftp-chroot-for-members-of-a-group.patch
patch -p1 --dry-run < 0001-Add-a-built-in-sftp-server-like-OpenSSH-internal-sft.patch
```

- chroot patch: `auth.h`, `dbutil.c`, `default_options.h`, `sysoptions.h`,
  `svr-auth.c`, `svr-chansession.c`, `svr-tcpfwd.c`, `svr-streamfwd.c`,
  `svr-agentfwd.c`, `svr-x11fwd.c`;
- internal patch: `Makefile.in`, `default_options.h`, `svr-chansession.c`,
  plusz az új `svr-sftp.c` és `svr-sftp.h`.

Ha valamelyik hunk nem megy fel, a meglévő fájlokban a változtatás mindenhol
pár sor, kézzel átvihető. Az új fájlok a dropbear belsejéből csak a `ses`,
`opts`, `m_burn`, `m_close` és `_dropbear_log` neveket használják.
