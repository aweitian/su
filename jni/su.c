#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "checkperm.h"
#include "setlogin.h"

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
