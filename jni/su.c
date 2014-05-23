#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "checkperm.h"
#include "setlogin.h"

#ifndef BUFSIZ
#define BUFSIZ 8192
#endif

#define READ  0
#define WRITE 1

static int su(int argc, char **argv, int optind, char *command, char *shell) {
	if (command != NULL) {
		execlp(shell, "sh", "-c", command, NULL);
	} else if (optind >= argc) {
		execlp(shell, "sh", NULL);
	} else if (strcmp(argv[optind], "-c")) {
		execvp(argv[optind], &argv[optind]);
	} else if (optind + 1 < argc) {
		execlp(shell, "sh", "-c", argv[optind + 1], NULL);
	} else {
		errno = EINVAL;
		perror("su");
		return -errno;
	}

	perror("execlp");
	return -errno;
}

static int su2(int argc, char **argv, int optind, char *command, char *shell) {
	int status, fildes2, fildes[2];
	if (pipe(fildes) < -1) {
		perror("pipe");
		return -1;
	}
	if (fork() == 0) {
		close(STDIN_FILENO);
		dup(fildes[READ]);
		close(fildes[READ]);
		close(fildes[WRITE]);
		su(argc, argv, optind, command, shell);
	}
	close(STDOUT_FILENO);
	fildes2 = dup(fildes[WRITE]);
	close(fildes[READ]);
	close(fildes[WRITE]);
	process(write, fildes2);
	close(fildes2);
	wait(&status);
	return status;
}


int main(int argc, char **argv) {
	int ch;
	char *command = NULL, *login = "root", *shell = "sh";

	if ((ch = checkperm(argc, argv))) {
		return ch;
	}

	while ((ch = getopt(argc, argv, "c:ls:")) != -1) {
		switch ((char)ch) {
			case 'c':
				command = optarg;
				break;
			case 'l':
				break;
			case 's':
				shell = optarg;
				break;
			default:
				break;
		}
	}

	if (optind < argc && !strcmp(argv[optind], "-")) {
		++optind;
	}

	if (optind < argc) {
		login = argv[optind++];
	}

	if ((ch = setlogin(login))) {
		return ch;
	}

	if (istrust() || !hasinput()) {
		return su(argc, argv, optind, command, shell);
	} else {
		return su2(argc, argv, optind, command, shell);
	}
}
