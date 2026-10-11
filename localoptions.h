/* localoptions.h - dropbear ugrodeszka szerver, lecsupaszitott build
 * Az itt nem szereplo opciok a default_options.h erteket kapjak.
 * Build: ./configure --disable-zlib --disable-pam --disable-lastlog \
 *                    --disable-wtmp --disable-wtmpx --disable-utmp --disable-utmpx \
 *                    --enable-static
 *        make PROGRAMS=dropbear
 */

/* --- modok --- */
#define NON_INETD_MODE 1
#define INETD_MODE 0            /* vagy forditva, ha tcpserver/inetd alol futtatod */
#define DROPBEAR_REEXEC 1       /* ASLR ujra-randomizalas kapcsolatonkent */
#define DEBUG_TRACE 0
#define DROPBEAR_SMALL_CODE 0   /* x86 szerveren nem kell a kis kod, gyorsabb crypto */

/* --- csatornak / forwarding --- */
#define DROPBEAR_X11FWD 0
#define DROPBEAR_SVR_LOCALTCPFWD 1    /* kliens -L: ezen keresztul lepsz tovabb */
#define DROPBEAR_SVR_REMOTETCPFWD 0   /* kliens -R: nem kell */
#define DROPBEAR_SVR_LOCALSTREAMFWD 0 /* unix socket fwd (CVE-2025-14282 ezt erintette) */
#define DROPBEAR_SVR_REMOTESTREAMFWD 0
#define DROPBEAR_SVR_AGENTFWD 0
#define DROPBEAR_SFTPSERVER 1
#define SFTPSERVER_PATH "/usr/local/libexec/gesftpserver"

/* kliens oldali dolgok, csak hogy biztos ne forduljanak be */
#define DROPBEAR_CLI_LOCALTCPFWD 0
#define DROPBEAR_CLI_REMOTETCPFWD 0
#define DROPBEAR_CLI_AGENTFWD 0
#define DROPBEAR_CLI_PROXYCMD 0
#define DROPBEAR_CLI_NETCAT 0
#define DROPBEAR_CLI_COMPRESSION 0
#define DROPBEAR_USER_ALGO_LIST 0

/* --- KEX: csak curve25519 --- */
#define DROPBEAR_CURVE25519 1
#define DROPBEAR_ECDH 0               /* NIST gorbek */
#define DROPBEAR_DH_GROUP14_SHA1 0
#define DROPBEAR_DH_GROUP14_SHA256 0  /* 1-re, ha kell klasszikus DH tartalek */
#define DROPBEAR_DH_GROUP16 0
#define DROPBEAR_DH_GROUP1 0
#define DROPBEAR_SNTRUP761 0          /* PQ hibrid: uj, kiforratlan */
#define DROPBEAR_MLKEM768 0

/* --- host key / pubkey tipusok --- */
#define DROPBEAR_ED25519 1
#define DROPBEAR_RSA 1                /* atmenetileg, a meglevo kulcsokhoz; kesobb 0 */
#define DROPBEAR_RSA_SHA1 0           /* csak rsa-sha2-256/512 */
#define DROPBEAR_DEFAULT_RSA_SIZE 3072
#define DROPBEAR_ECDSA 0
#define DROPBEAR_DSS 0
#define DROPBEAR_SK_KEYS 0            /* U2F/FIDO kulcsok */
#define DROPBEAR_DELAY_HOSTKEY 0      /* host key legyen generalva elore, ne futasidoben */

/* --- cipher / MAC: chacha20-poly1305 + aes256-ctr tartalek --- */
#define DROPBEAR_CHACHA20POLY1305 1
#define DROPBEAR_AES256 1
#define DROPBEAR_AES128 0
#define DROPBEAR_3DES 0
#define DROPBEAR_ENABLE_CTR_MODE 1
#define DROPBEAR_ENABLE_CBC_MODE 0
#define DROPBEAR_ENABLE_GCM_MODE 0
#define DROPBEAR_SHA2_256_HMAC 1
#define DROPBEAR_SHA2_512_HMAC 0
#define DROPBEAR_SHA1_HMAC 0
#define DROPBEAR_SHA1_96_HMAC 0

/* --- auth --- */
#define DROPBEAR_SVR_PASSWORD_AUTH 1
#define DROPBEAR_SVR_PAM_AUTH 0
#define DROPBEAR_SVR_PUBKEY_AUTH 1
#define DROPBEAR_SVR_PUBKEY_OPTIONS 1 /* authorized_keys: restrict,port-forwarding stb. */
#define DROPBEAR_SVR_DROP_PRIVS 1

/* --- brute force limitek --- */
#define MAX_UNAUTH_PER_IP 3
#define MAX_UNAUTH_CLIENTS 20
#define MAX_AUTH_TRIES 3
#define MAX_PUBKEY_QUERIES 5
#define UNAUTH_CLOSE_DELAY 5          /* mp, sikertelen auth utan; DoS kockazat, ne tul nagy */
#define DEFAULT_IDLE_TIMEOUT 1800
#define DEFAULT_KEEPALIVE 60

/* --- egyeb --- */
#define DO_HOST_LOOKUP 0
#define DO_MOTD 0
#define LOG_COMMANDS 1                /* csak az egyszeri parancsot logolja, hasznos audit */

/* --- sftp chroot + beepitett sftp (sajat patchek: dropbear-sftp-chroot.patch, dropbear-internal-sftp.patch) --- */
#define DROPBEAR_SFTP_CHROOT 1
#define DROPBEAR_SFTP_CHROOT_GROUP "sftponly"
#define DROPBEAR_SFTP_CHROOT_SERVER "internal-sftp"   /* beepitett, a chrootba nem kell semmi */
#define DROPBEAR_SFTP_INTERNAL 1      /* beepitett sftp szerver: "internal-sftp" (chroot es SFTPSERVER_PATH is hasznalhatja) */
#define DEFAULT_RECV_WINDOW 2097152
#define TRANS_MAX_PAYLOAD_LEN 32768
