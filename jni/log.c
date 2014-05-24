/* vim: set sw=4 ts=4:
 * Author: Liu DongMiao <liudongmiao@gmail.com>
 * Created  : TIMESTAMP
 * Modified : TIMESTAMP
 */

#include <stdio.h>
#include <poll.h>
#include <unistd.h>

#include "log.h"

void logargv(char *parent, int allowed, int argc, char **argv) {
	FILE *stream;
	if ((stream = fopen(LOGPATH, "a")) != NULL) {
		int i;
		fprintf(stream, "+++++++++++++++++++++++++++++++++++++++++++++++\n");
		fprintf(stream, "--------------------command--------------------\n");
		fprintf(stream, "parent=%s, allow=%d\n", parent, allowed);
		for (i = 0; i < argc; i++) {
			fprintf(stream, "argv[%d]=%s\n", i, argv[i]);
		}
		fclose(stream);
	}
}

void logstdin(ssize_t (*canprocess)(void), ssize_t (*process)(void *data, const void *buf, size_t nbyte), void *data) {
	FILE *stream = NULL;
	stream = fopen(LOGPATH, "a");
	if (stream != NULL) {
		fprintf(stream, "---------------------stdin---------------------\n");
		fflush(stream);
	}
	do {
		int pollfd;
		struct pollfd fds;
		fds.fd = 0;
		fds.events = POLLIN;
		pollfd = poll(&fds, 1, 250);
		if (pollfd > 0) {
			ssize_t nbytes;
			char *buf[BUFSIZ];
			nbytes = read(STDIN_FILENO, buf, BUFSIZ);
			if (nbytes < 0) {
				break;
			} else if (nbytes == 0) {
				/* eof */
				break;
			}
			if (stream != NULL) {
				fwrite(buf, nbytes, 1, stream);
				fflush(stream);
			}
			if (process) {
				process(data, buf, nbytes);
			}
		} else if (pollfd == 0) {
			/* timeout */
			continue;
		} else if (pollfd < 0) {
			break;
		}
	} while (canprocess && canprocess());

	if (stream != NULL) {
		fprintf(stream, "===============================================\n\n");
		fclose(stream);
		fflush(stream);
	}
}
