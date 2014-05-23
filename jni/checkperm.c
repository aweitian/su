/* vim: set sw=4 ts=4:
 * Author: Liu DongMiao <liudongmiao@gmail.com>
 * Created  : Thu 11 Aug 2011 08:19:15 PM CST
 * Modified : Wed 21 Mar 2012 03:21:33 AM CST
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <poll.h>

#include "checkperm.h"

#ifndef ALLOW
#define ALLOW "me.piebridge."
#endif

#ifndef TERM
#define TERM "jackpal.androidterm"
#endif

#ifndef AID_ROOT
#define AID_ROOT 0
#endif

#ifndef AID_SHELL
#define AID_SHELL 2000
#endif

#ifndef BUFSIZ
#define BUFSIZ 4096
#endif

static char *rstrip(char *s)
{
	char *e;
	e = s + strlen(s) - 1;
	while (e >= s && isspace(*e)) *(e--) = '\0';
	return s;
}

// get the uid of terminal
static uid_t getterm()
{
	struct stat sb;
	if (!stat("/data/data/" TERM, &sb)) {
		return sb.st_uid;
	};
	return (uid_t) AID_ROOT;
}

int istrust() {
	uid_t uid;
	static int trust = -1;
	if (trust != -1) {
		return trust;
	}
	uid = getuid();
	trust = (uid == AID_ROOT || uid == AID_SHELL || uid == getterm());
	return trust;
}

int checkperm(int argc, char **argv) {
	int i, allowed = 0;
	char filepath[32];
	char cmdline[256];
	char lineptr[BUFSIZ];
	FILE *stream = NULL;

	if (istrust()) {
		return 0;
	}

	// get the parent's name
	sprintf(filepath, "/proc/%u/cmdline", getppid());
	stream = fopen(filepath, "r");
	if (stream == NULL) {
		perror("invalid parent");
		return -EINVAL;
	}
	fgets(cmdline, sizeof(cmdline) - 1, stream);
	fclose(stream);

	// check allow
	if (memcmp(cmdline, ALLOW, sizeof(ALLOW) - 1) == 0) {
		allowed = 1;
	} else if ((stream = fopen("/sdcard/su.allow", "r")) != NULL) {
		// check permission from su.allow
		while (fgets(lineptr, BUFSIZ - 1, stream)) {
			rstrip(lineptr);
			if (strncmp(lineptr, cmdline, strlen(lineptr)) == 0) {
				allowed = 1;
				break;
			}
		}
		fclose(stream);
	}

	// log it
	if ((stream = fopen("/sdcard/su.log", "a")) != NULL) {
		fprintf(stream, "+++++++++++++++++++++++++++++++++++++++++++++++\n");
		fprintf(stream, "--------------------command--------------------\n");
		fprintf(stream, "parent=%s, allow=%d\n", cmdline, allowed);
		for (i = 0; i < argc; i++) {
			fprintf(stream, "argv[%d]=%s\n", i, argv[i]);
		}
		fclose(stream);
		if (!allowed && hasinput()) {
			process(NULL, 0);
		}
	}

	if (!allowed) {
		fprintf(stderr, "no permission for \"%s\"\n", cmdline);
		return -EPERM;
	}
	return 0;
}

int hasinput() {
	int pollfd;
	struct pollfd fds;
	fds.fd = 0;
	fds.events = POLLIN;
	pollfd = poll(&fds, 1, 250);
	if (pollfd == 1) {
		return 1;
	} else {
		return 0;
	}
}

void process(ssize_t (*w)(int fildes, const void *buf, size_t nbyte), int fildes) {
	char *buf[BUFSIZ];
	FILE *stream = NULL;
	stream = fopen("/sdcard/su.log", "a");
	if (stream != NULL) {
		fprintf(stream, "---------------------stdin---------------------\n");
		fflush(stream);
	} else if (!w) {
		return;
	}
	while (1) {
		if (fgets((char *)buf, BUFSIZ - 1, stdin) == NULL) {
			if (feof(stdin)) {
				break;
			}
		} else {
			if (stream != NULL) {
				fprintf(stream, "%s", (char *)buf);
				fflush(stream);
			}
			if (w) {
				w(fildes, buf, strlen((char *)buf));
			}
			if (strcmp((char *)&buf, "exit\n") == 0 || strcmp((char *)&buf, "exit\r\n") == 0) {
				break;
			}
		}
	}
	if (stream != NULL) {
		fprintf(stream, "===============================================\n\n");
		fclose(stream);
		fflush(stream);
	}
}
