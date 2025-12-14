#include "builtin.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return !cmd || (strcmp(cmd, "exit") == 0);
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	if (strncmp("cd", cmd, 2) != 0) {
		return false;
	};

	int i = 2;
	while (cmd[i] == SPACE)
		i++;

	char *directory = NULL;
	if (cmd[i] == END_LINE || cmd[i] == END_STRING) {
		directory = getenv("HOME");

	} else {
		directory = cmd + i;
	}

	if (chdir(directory) < 0) {
		status = 2;
#ifndef SHELL_NO_INTERACTIVE
		perror("invalid directory");
#endif
		return true;
	};

	if (!getcwd(prompt, ARGSIZE)) {
		status = 1;
#ifndef SHELL_NO_INTERACTIVE
		perror("getcwd failed");
#endif
		return true;
	}
	status = 0;
	return true;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strncmp("pwd", cmd, 3) != 0)
		return false;

	char cwd[ARGSIZE] = { 0 };

	if (!getcwd(cwd, ARGSIZE)) {
#ifndef SHELL_NO_INTERACTIVE
		perror("pwd failed");
#endif
		return false;
	}

	printf("%s\n", cwd);
	return true;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here
	(void) cmd;
	return 0;
}
