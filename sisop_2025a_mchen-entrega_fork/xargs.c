#ifndef NARGS
#define NARGS 4
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>


void
ejecutar_subprograma(char **args)
{
	pid_t pid = fork();

	if (pid == 0) {  // hijo
		execvp(args[0], args);
		exit(0);

	} else {  // padre
		wait(NULL);
	}
}

int
main(int argc, char *argv[])
{
	char *linea_leida = NULL;
	char *argumentos[NARGS + 2] = { 0 };
	char *buffer = NULL;

	// Apunta al comando el primer argumetno de la lista de argumentos
	argumentos[0] = argv[1];

	size_t tamanio;
	ssize_t carac_leidos;
	size_t n = 1;

	fflush(stdin);
	while ((carac_leidos = getline(&linea_leida, &tamanio, stdin)) > 0) {
		linea_leida[carac_leidos - 1] = '\0';
		buffer = realloc(argumentos[n],
		                 (strlen(linea_leida) + 1) * sizeof(char));

		if (!buffer) {
			perror("Error Realloc");
			exit(-1);
		}
		argumentos[n] = buffer;
		strncpy(argumentos[n], linea_leida, carac_leidos);

		// Creo un proceso una vez que tenga NARGS argumentos
		if (n == NARGS) {
			n = 0;

			ejecutar_subprograma(argumentos);

			// Reseteo la lista de argumentos
			for (size_t i = 1; i <= NARGS; i++) {
				memset(argumentos[i],
				       0,
				       (strlen(argumentos[i]) + 1) * sizeof(char));
			}
		}
		// Reseteo el buffer
		memset(linea_leida, 0, strlen(linea_leida) * sizeof(char));
		n++;
	}

	// En caso de haber argumentos sin procesar
	if (n != 1) {
		for (size_t i = n + 1; i <= NARGS; i++) {
			free(argumentos[i]);
		}
		argumentos[n] = NULL;
		ejecutar_subprograma(argumentos);
	}

	// Libero memoria pedida
	for (size_t i = 1; i <= n; i++) {
		free(argumentos[i]);
	}
	free(linea_leida);
	return 0;
}
