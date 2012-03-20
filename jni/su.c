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

static int hasc(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-c")) {
			return 1;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	uid_t uid;

	// run-as always has suid, so this program can replace with run-as
	if (!strcmp(argv[0], "run-as") && !hasc(argc, argv)) {
		execvp("runas", argv);
		return -errno; 
	}

	uid = getuid();
	// root, shell, terminal can bypass permission checking
	if (uid != AID_ROOT && uid != AID_SHELL && uid != getterm()) {
		int i, flags = 0;
		char filepath[32];
		char cmdline[256];
		char lineptr[4096];
		FILE *stream = NULL;;

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
			flags = 1;
		} else if ((stream = fopen("/sdcard/su.allow", "r")) != NULL) {
			// check permission from su.allow
			while (fgets(lineptr, sizeof(lineptr) - 1, stream)) {
				rstrip(lineptr);
				if (strcmp(lineptr, cmdline) == 0) {
					flags = 1;
					fclose(stream);
					break;
				}
			}
		}

		// log it for unknown request
		if ((stream = fopen("/sdcard/su.log", "a")) != NULL) {
			fprintf(stream, "+++++++++++++++++++++++++++++++++++++++++++++++\n");
			fprintf(stream, "--------------------command--------------------\n");
			fprintf(stream, "parent=%s\n", cmdline);
			for (i = 0; i < argc; i++) {
				fprintf(stream, "argv[%d]=%s\n", i, argv[i]);
			}
			fflush(stream);
			// log it if there is no -c
			if (!hasc(argc, argv)) {
				fprintf(stream, "---------------------stdin---------------------\n");
				while ((i = read(0, lineptr, sizeof(lineptr))) > 0) {
					fwrite(lineptr, i, 1, stream);
					fflush(stream);
					if (i < sizeof(lineptr)) {
						break;
					}
				}
			}
			fprintf(stream, "===============================================\n\n");
			fclose(stream);
		}

		if (!flags) {
			fprintf(stderr, "no permission for \"%s\"\n", cmdline);
			return -EPERM;
		}
	}

	if (setuid(AID_ROOT) || setgid(AID_ROOT)) {
		perror("su");
		return -EPERM;
	}

	if (hasc(argc, argv)) {
		strcpy(argv[0], "sh");
		execvp("sh", argv);
	} else if (uid != AID_ROOT && !isatty(0)) {
		fprintf(stderr, "standard in must be a tty\n");
		return -EPERM;
	} else {
		execlp("sh", "sh", NULL);
	}

	perror("execlp");
	return -errno;
}
