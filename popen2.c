#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


pid_t popen2(const char *command, int *infp, int *outfp)
{
	int p_stdin[2], p_stdout[2];
	pid_t pid;

	if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
		return -1;

	pid = fork();

	switch (pid) {
		case -1:
			perror("fork");
			exit(EXIT_FAILURE);

		case 0:
			// Child process
			dup2(p_stdin[0], STDIN_FILENO);
			close(p_stdin[STDIN_FILENO]);
			close(p_stdin[STDOUT_FILENO]);

			dup2(p_stdout[1], STDOUT_FILENO);
			close(p_stdout[STDOUT_FILENO]);
			close(p_stdout[STDIN_FILENO]);

			execl("/bin/sh", "sh", "-c", command, NULL);
			exit(EXIT_SUCCESS);

		default:
			// Parent process
			*infp = p_stdin[1];
			*outfp = p_stdout[0];

			close(p_stdin[STDIN_FILENO]);
			close(p_stdout[STDOUT_FILENO]);

			return pid;
	}
}

