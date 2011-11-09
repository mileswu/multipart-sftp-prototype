/*
 * $Id: sftpdir.c,v 1.11 2009/04/28 10:35:30 bagder Exp $
 *
 * Sample doing an SFTP directory listing.
 *
 * The sample code has default values for host name, user name, password and
 * path, but you can specify them on the command line like:
 *
 * "sftpdir 192.168.0.1 user password /tmp/secretdir"
 */

#include <libssh2.h>
#include <libssh2_sftp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

static void kbd_callback(const char *name, int name_len,
                         const char *instruction, int instruction_len,
                         int num_prompts,
                         const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                         LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                         void **abstract)
{
    (void)name;
    (void)name_len;
    (void)instruction;
    (void)instruction_len;
    if (num_prompts == 1) {
   	 const char *password="password";
        responses[0].text = strdup(password);
        responses[0].length = strlen(password);
    }
    (void)prompts;
    (void)abstract;
} /* kbd_callback */ 

void start_session(LIBSSH2_SESSION *session, int *sock) {
    struct sockaddr_in sin;
	 int rc;

	 // Devel defaults
    const char *username="username";
    const char *password="password";
    //unsigned long hostaddr = htonl(0x7F000001);

	 // Make socket
    *sock = socket(AF_INET, SOCK_STREAM, 0);

    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);
    //sin.sin_addr.s_addr = hostaddr;
	 inet_pton(AF_INET, "78.129.175.48", &sin.sin_addr);

    if (connect(*sock, (struct sockaddr*)(&sin),
            sizeof(struct sockaddr_in)) != 0) {
        fprintf(stderr, "failed to connect!\n");
        return NULL;
    }

    rc = libssh2_session_startup(session, *sock);
    if(rc) {
        fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
        return NULL;
    }

	 // Fingerprint
    /*fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
    printf("Fingerprint: ");
    for(i = 0; i < 20; i++) {
        printf("%02X ", (unsigned char)fingerprint[i]);
    }
    printf("\n");*/

	 // Authenticate
	 if(libssh2_userauth_keyboard_interactive(session, username, &kbd_callback)) {
//    if (libssh2_userauth_password(session, username, password)) {
        printf("Authentication by password failed.\n");
    }
	 printf("Connected\n");
}

int main(int argc, char *argv[])
{
    const char *sftppath="/tmp/secretdir/test.txt";
    int i;
    int rc;
    LIBSSH2_SFTP *sftp_session;
    LIBSSH2_SFTP_HANDLE *sftp_handle;
	 

	 // Init
    rc = libssh2_init (0);
    if (rc != 0) {
        fprintf (stderr, "libssh2 initialization failed (%d)\n", rc);
        return 1;
    }

    LIBSSH2_SESSION *session;
    session = libssh2_session_init();
    if(!session)
        return NULL;
	 int sock;
	 start_session(session, &sock);

	 // Init SFTP
    fprintf(stderr, "libssh2_sftp_init()!\n");
    sftp_session = libssh2_sftp_init(session);

    if (!sftp_session) {
        fprintf(stderr, "Unable to init SFTP session\n");
        goto shutdown;
    }

    /* Since we have not set non-blocking, tell libssh2 we are blocking */
    libssh2_session_set_blocking(session, 1);

	 sftp_handle = libssh2_sftp_open(sftp_session, sftppath, LIBSSH2_FXF_READ, LIBSSH2_SFTP_S_IRWXU);
	 if(!sftp_handle) {
        fprintf(stderr, "Unable to open file\n");
        goto shutdown;
    }

	 LIBSSH2_SFTP_ATTRIBUTES att;
	 libssh2_sftp_fstat(sftp_handle, &att);
	 printf("Size - %d\n", att.filesize);

	 struct timeval tp, tp2;
	 
	 FILE *f = fopen("out", "wb");
    for(i=0; i<att.filesize; i++)
		 fputc(0, f);
	 rewind(f);

	 gettimeofday(&tp, NULL);
    libssh2_session_set_blocking(session, 0);
	 printf("Starting file xfer\n");

	 int buffer_size = 100*1024;
	 size_t amount_dl = 0;
	 char *buffer = malloc(sizeof(char) * buffer_size);
	 for(;;) {
         size_t s = libssh2_sftp_read(sftp_handle, buffer, buffer_size);

			
			if(s == LIBSSH2_ERROR_EAGAIN) {}
			else if(s == 0)
				break;
			else if(s < 0) {
				fprintf(stderr, "Failure of sftp read\n");
				break;
			}
			else {
				amount_dl += s;
				int percentage = amount_dl*100/att.filesize;
				printf("\rProgress: %d", percentage);
				fwrite((void *)buffer, s, 1, f);
			}

		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(sock, &fd);
		struct timeval timeout;
		timeout.tv_sec = 30; timeout.tv_usec = 0;
		select(sock+1, &fd, NULL, NULL, &timeout);
    }
	 printf("\n");
	 
	 gettimeofday(&tp2, NULL);
	 double time = tp2.tv_sec - tp.tv_sec + ((double)(tp2.tv_usec - tp.tv_usec))/1000000.0;
	 double speed = ((double)att.filesize) / time;
	 printf("Speed: %f KB/s\n", speed/1024, time);

	 close(f);
	 libssh2_sftp_close(sftp_handle);

	 


    libssh2_sftp_shutdown(sftp_session);

 shutdown:

    libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
    libssh2_session_free(session);

    //close(sock);
    printf("all done\n");
    libssh2_exit();


    return 0;
}
