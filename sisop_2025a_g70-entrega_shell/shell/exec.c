#include "exec.h"
#include "defs.h"
#include <fcntl.h>
#include <string.h>

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	// Your code here

	for (int i = 0; i < eargc; i++) {
		char key[ARGSIZE] = { 0 };
		char value[ARGSIZE] = { 0 };

		int idx = 0;
		char *env = eargv[i];
		if ((idx = block_contains(env, '=')) > 0) {
			get_environ_key(env, key);
			get_environ_value(env, value, idx);
			if (setenv(key, value, 1) < 0)
				perror("Setting in enviroment variable failed");
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	if (flags & O_CREAT)
		return open(file, flags | O_CLOEXEC, S_IWUSR | S_IRUSR);
	return open(file, flags | O_CLOEXEC);
}

static void
redirect_exec_fds(char *file, int new_fd, int flags)
{
	int fd = open_redir_fd(file, flags);
	if (fd == -1) {
		perror("open_redir_fd failed");
		exit(EXIT_FAILURE);
	}
	dup2(fd, new_fd);
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC: {
		e = (struct execcmd *) cmd;

		set_environ_vars(e->eargv, e->eargc);
		execvp(e->argv[0], e->argv);
#ifndef SHELL_NO_INTERACTIVE
		perror("execvp failed");
#endif

		exit(EXIT_FAILURE);
	}

	case BACK: {
		// runs a command in background
		//
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		r = (struct execcmd *) cmd;
		if (strlen(r->in_file) > 0) {
			redirect_exec_fds(r->in_file, STDIN_FILENO, O_RDONLY);
		}
		if (strlen(r->out_file) > 0)
			redirect_exec_fds(r->out_file,
			                  STDOUT_FILENO,
			                  O_CREAT | O_WRONLY | O_TRUNC);
		if (strlen(r->err_file) > 0) {
			if (strcmp(r->err_file, "&1") == 0) {
				dup2(STDOUT_FILENO, STDERR_FILENO);
			} else {
				redirect_exec_fds(r->err_file,
				                  STDERR_FILENO,
				                  O_CREAT | O_WRONLY | O_TRUNC);
			}
		}
		r->type = EXEC;
		exec_cmd((struct cmd *) r);
		break;
	}

	case PIPE: {
		// pipes two commands
		p = (struct pipecmd *) cmd;

		int fds[2];
		if (pipe(fds) == -1) {
			perror("pipe failed");
			exit(EXIT_FAILURE);
		}

		pid_t l_pid = fork();
		setpgid(0, 0);
		if (l_pid == -1) {
			perror("fork failed");
			exit(EXIT_FAILURE);
		}

		if (l_pid == 0) {
			close(fds[READ]);
			dup2(fds[WRITE], STDOUT_FILENO);
			close(fds[WRITE]);
			exec_cmd(p->leftcmd);
		}

		close(fds[WRITE]);
		pid_t r_pid = fork();
		setpgid(0, 0);
		if (r_pid == -1) {
			perror("fork failed");
			exit(EXIT_FAILURE);
		}

		if (r_pid == 0) {
			dup2(fds[READ], STDIN_FILENO);
			close(fds[READ]);
			exec_cmd(p->rightcmd);
		}
		close(fds[READ]);
		waitpid(l_pid, NULL, 0);
		waitpid(r_pid, NULL, 0);

		// free the memory allocated
		// for the pipe tree structure
		free_command(parsed_pipe);
		_exit(EXIT_SUCCESS);
		break;
	}
	}
}
