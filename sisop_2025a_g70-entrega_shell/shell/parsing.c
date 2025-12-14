#include "parsing.h"


// parses an argument of the command stream input
static char *
get_token(char *buf, int idx)
{
	char *tok;
	int i;

	tok = (char *) calloc(ARGSIZE, sizeof(char));
	i = 0;

	while (buf[idx] != SPACE && buf[idx] != END_STRING) {
		tok[i] = buf[idx];
		i++;
		idx++;
	}

	return tok;
}

// parses and changes stdin/out/err if needed
static bool
parse_redir_flow(struct execcmd *c, char *arg)
{
	int inIdx, outIdx;

	// flow redirection for output
	if ((outIdx = block_contains(arg, '>')) >= 0) {
		switch (outIdx) {
		// stdout redir
		case 0: {
			strcpy(c->out_file,
			       arg + 1);  /// file para redirección de la salida
			break;
		}
		// stderr redir
		case 1: {  /// Se presupone que va a tener algo de la forma "2>" siempre al comienzo del string
			strcpy(c->err_file,
			       &arg[outIdx + 1]);  /// file para redirección de la salida del error
			break;
		}
		}

		free(arg);  /// Acá se libera arg porque sino se perdería el puntero
		            /// al mismo, ya que se está asignando "arg + 1" para los files
		c->type = REDIR;

		return true;
	}

	// flow redirection for input
	if ((inIdx = block_contains(arg, '<')) >= 0) {
		// stdin redir
		strcpy(c->in_file,
		       arg + 1);  /// file para redirección de la entrada

		c->type = REDIR;
		free(arg);

		return true;
	}

	return false;
}

// parses and sets a pair KEY=VALUE
// environment variable
static bool
parse_environ_var(struct execcmd *c,
                  char *arg)  /// token de asignación de variable de entorno
{
	// sets environment variables apart from the
	// ones defined in the global variable "environ"

	if (block_contains(arg, '=') > 0) {
		// checks if the KEY part of the pair
		// does not contain a '-' char which means
		// that it is not a environ var, but also
		// an argument of the program to be executed
		// (For example:
		// 	./prog -arg=value
		// 	./prog --arg=value		///opciones o argumentos de comando
		// )
		if (block_contains(arg, '-') < 0) {
			c->eargv[c->eargc++] =
			        arg;  /// se agrega el arg de la forma "KEY=VALUE" (expanded environment arguments)
			return true;
		}
	}

	return false;
}

// this function will be called for every token, and it should
// expand environment variables. In other words, if the token
// happens to start with '$', the correct substitution with the
// environment value should be performed. Otherwise the same
// token is returned. If the variable does not exist, an empty string should be
// returned within the token
//
// Hints:
// - check if the first byte of the argument contains the '$'
// - expand it and copy the value in 'arg'
// - remember to check the size of variable's value
//		It could be greater than the current size of 'arg'
//		If that's the case, you should realloc 'arg' to the new size.
static char *
expand_environ_var(char *arg)
{
	if (!arg || arg[0] != '$')
		return arg;

	if (arg[1] == '?') {
		sprintf(arg, "%d", status);
		return arg;
	}

	char *value = getenv(arg + 1);  /// Expansión de la variable de entorno

	if (!value) {
		free(arg);
		return "";
	}
	size_t value_len = strlen(value);
	size_t arg_len = strlen(arg);

	if (value_len > arg_len) {
		char *block = realloc(arg, value_len + 1);
		if (!block) {
			free(arg);
			return "";
		}
		arg = block;
	}

	strcpy(arg, value);

	return arg;
}

// parses one single command having into account:
// - the arguments passed to the program
// - stdin/stdout/stderr flow changes
// - environment variables (expand and set)
static struct cmd *
parse_exec(char *buf_cmd)
{
	struct execcmd *c;
	char *tok;
	int idx = 0, argc = 0;

	c = (struct execcmd *) exec_cmd_create(buf_cmd);

	while (buf_cmd[idx] != END_STRING) {
		tok = get_token(
		        buf_cmd,
		        idx);  /// el "token" es obtenido hasta el primer SPACE o END_STRING
		idx = idx + strlen(tok);

		if (buf_cmd[idx] != END_STRING)
			idx++;

		if (parse_redir_flow(c, tok))
			continue;

		if (parse_environ_var(c, tok))
			continue;

		tok = expand_environ_var(tok);

		if (strcmp(tok, "") ==
		    0)  /// ESTA LÍNEA FUE AGREGADA, NO SE SI SEA PERMITIDO EN
			/// ESTA FUNCION, PUES NO DECÍA "your code here"
			continue;

		c->argv[argc++] =
		        tok;  /// Acá se agregaría (como argumento) el
		              /// token que no sea ninguno de los anteriores,
		              /// por lo que el "comando inicial" será también un arg
	}

	c->argv[argc] =
	        (char *) NULL;  /// Se finaliza la cadena de argumentos
	                        /// a ejecutar con "NULL", requerimiento para execvp
	c->argc = argc;

	return (struct cmd *) c;
}

// parses a command knowing that it contains the '&' char
static struct cmd *
parse_back(char *buf_cmd)
{
	int i = 0;
	struct cmd *e;

	while (buf_cmd[i] != '&')
		i++;

	buf_cmd[i] = END_STRING;

	e = parse_exec(buf_cmd);

	return back_cmd_create(e);
}

// parses a command and checks if it contains the '&'
// (background process) character
static struct cmd *
parse_cmd(char *buf_cmd)
{
	if (strlen(buf_cmd) == 0)
		return NULL;

	int idx;

	// checks if the background symbol is after
	// a redir symbol, in which case
	// it does not have to run in in the 'back'
	if ((idx = block_contains(buf_cmd, '&')) >= 0 && buf_cmd[idx - 1] != '>')
		return parse_back(buf_cmd);

	return parse_exec(buf_cmd);
}

// // parses the command line
// // looking for the pipe character '|'
struct cmd *
parse_line(char *buf)
{
	struct cmd *r, *l;

	char *right = split_line(
	        buf, '|');  /// despues de esto buf ya tiene un
	                    /// "END_STRING" al finalizar el lado izquierdo

	int idx;
	if ((idx = block_contains(right, '|')) >= 0) {
		r = parse_line(right);
	} else {
		r = parse_cmd(
		        right);  /// tiene que ser pasado únicamente el str del comando y sus argumentos
	}

	l = parse_cmd(
	        buf);  /// tiene que ser pasado únicamente el str del comando y sus argumentos

	return pipe_cmd_create(l, r);
}
