#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"
#include <signal.h>
#include <stdio.h>

#define STACKSIZE SIGSTKSZ
#define MEN_BACK_TERMINADO "==> terminado: PID="


static stack_t stack_alt;
char prompt[PRMTLEN] = { 0 };

// converts a int to string for Write in STDOUT
static size_t
to_string(int n, char *str)
{
	int i = 0;
	char buff[20];
	if (n == 0) {
		str[i++] = '0';
		str[i] = '\0';
		return i;
	}

	while (n > 0) {
		buff[i++] = (n % 10) + '0';
		n /= 10;
	}

	int j = 0;
	while (i > 0) {
		str[j++] = buff[--i];
	}
	str[j] = '\0';

	return j;
}


// executes when back processes finish
static void
sigchld_handler(int signum)
{
	(void) signum;
	pid_t pid;
	int status;
	char pid_str[20];
	ssize_t ret;
	size_t len;

	while ((pid = waitpid(0, &status, WNOHANG)) > 0) {
		len = to_string(pid, pid_str);

#ifndef SHELL_NO_INTERACTIVE
		if (isatty(1)) {
			ret = write(STDOUT_FILENO,
			            MEN_BACK_TERMINADO,
			            sizeof(MEN_BACK_TERMINADO) - 1);
			(void) ret;
			ret = write(STDOUT_FILENO, pid_str, len);
			(void) ret;
			ret = write(STDOUT_FILENO, "\n", 1);
			(void) ret;
		}
#endif
	}
}

// frees the alternate stack
static void
free_stack()
{
	stack_t stack_alt_desactivado;
	stack_alt_desactivado.ss_flags = SS_DISABLE;
	stack_alt_desactivado.ss_size = 0;
	stack_alt_desactivado.ss_sp = NULL;

	if (sigaltstack(&stack_alt_desactivado, NULL) < 0) {
#ifndef SHELL_NO_INTERACTIVE
		perror("sigalstack failed");
#endif
		exit(EXIT_FAILURE);
	}

	free(stack_alt.ss_sp);
}

// runs a shell command
static void
run_shell()
{
	char *cmd;

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");
	stack_t stack_orig;

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}

	// initializes alternate stack
	stack_alt.ss_sp = malloc(STACKSIZE);

	if (!stack_alt.ss_sp) {
#ifndef SHELL_NO_INTERACTIVE
		perror("malloc failed");
#endif
		exit(EXIT_FAILURE);
	}
	stack_alt.ss_size = STACKSIZE;
	stack_alt.ss_flags = 0;


	if (sigaltstack(&stack_alt, &stack_orig) < 0) {
#ifndef SHELL_NO_INTERACTIVE
		perror("sigalstack failed");
#endif
		exit(EXIT_FAILURE);
	}

	// initializes struct sigaction
	struct sigaction action_back;
	memset(&action_back, 0, sizeof(action_back));
	action_back.sa_handler = sigchld_handler;
	action_back.sa_flags = SA_RESTART | SA_ONSTACK | SA_NOCLDSTOP;

	sigaction(SIGCHLD, &action_back, NULL);
}

int
main(void)
{
	init_shell();

	run_shell();

	free_stack();
	return 0;
}
