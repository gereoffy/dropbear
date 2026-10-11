#ifndef DROPBEAR_SVR_SFTP_H_
#define DROPBEAR_SVR_SFTP_H_

/* Built-in minimal SFTP v3 server, like OpenSSH "internal-sftp".
 * With DROPBEAR_SFTP_INTERNAL, a command (subsystem path, forced command,
 * or DROPBEAR_SFTP_CHROOT_SERVER) of exactly SFTP_INTERNAL_NAME runs it. */

#define SFTP_INTERNAL_NAME "internal-sftp"

#if DROPBEAR_SFTP_INTERNAL && DROPBEAR_VFORK
#error "internal-sftp needs fork(), it cannot be used with DROPBEAR_VFORK"
#endif

/* Serve SFTP on infd/outfd until EOF. Returns the process exit code. */
int sftp_server_main(int infd, int outfd, const char *user);

/* Called in the forked session child instead of exec(). Does not return. */
void svr_sftp_internal(void);

#endif /* DROPBEAR_SVR_SFTP_H_ */
