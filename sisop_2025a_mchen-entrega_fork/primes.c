#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define WRITE 1
#define READ 0

int filtrado(int *);

int
main(int argc, char *argv[])
{
	if (argc < 1) {
		exit(-1);
	}

	int n = atoi(argv[1]);

	if (n < 2) {
		perror("Ivalido n < 2");
		exit(-1);
	}

	int pipe_padre_hijo[2];
	int pip = pipe(pipe_padre_hijo);

	if (pip < 0) {
		perror("Error de Pipe");
		exit(-1);
	}

	ssize_t cant_bytes;
	pid_t pid = fork();
	if (pid > 0) {  // Padre
		close(pipe_padre_hijo[READ]);
		for (int i = 2; i <= n; i++) {
			cant_bytes = write(pipe_padre_hijo[WRITE], &i, sizeof(i));
			if (cant_bytes < 0) {
				close(pipe_padre_hijo[WRITE]);
				perror("Error write");
				exit(-1);
			}
		}
		close(pipe_padre_hijo[WRITE]);

		wait(NULL);

	} else if (pid == 0) {  // Hijo
		close(pipe_padre_hijo[WRITE]);
		filtrado(pipe_padre_hijo);
		close(pipe_padre_hijo[READ]);
		exit(0);

	} else {  // Error en fork
		close(pipe_padre_hijo[WRITE]);
		close(pipe_padre_hijo[READ]);
		exit(EXIT_FAILURE);
	}

	return 0;
}

int
filtrado(int *pipe_leer)
{
	int pipe_leer_valor_pasado;
	if (read(pipe_leer[READ],
	         &pipe_leer_valor_pasado,
	         sizeof(pipe_leer_valor_pasado)) <= 0) {
		close(pipe_leer[READ]);
		close(pipe_leer[WRITE]);
		exit(0);
	}
	printf("primo %d\n", pipe_leer_valor_pasado);

	int pipe_nuevo[2];
	int pip_nuevo = pipe(pipe_nuevo);

	if (pip_nuevo < 0) {
		perror("Error de Pipe");
		exit(-1);
	}

	ssize_t cant_bytes;
	pid_t pid_nuevo = fork();

	if (pid_nuevo > 0) {  // Padre
		close(pipe_nuevo[READ]);
		int leido;
		while (read(pipe_leer[READ], &leido, sizeof(leido)) > 0) {
			if (leido % pipe_leer_valor_pasado != 0) {
				cant_bytes = write(pipe_nuevo[WRITE],
				                   &leido,
				                   sizeof(leido));
				if (cant_bytes < 0) {
					close(pipe_leer[READ]);
					close(pipe_leer[WRITE]);
					close(pipe_nuevo[WRITE]);
					perror("Error write");
					exit(-1);
				}
			}
		}
		close(pipe_leer[READ]);
		close(pipe_leer[WRITE]);
		close(pipe_nuevo[WRITE]);
		wait(NULL);

	} else if (pid_nuevo == 0) {  // Hijo
		close(pipe_nuevo[WRITE]);
		close(pipe_leer[READ]);
		filtrado(pipe_nuevo);
		close(pipe_leer[WRITE]);
		close(pipe_nuevo[READ]);
		exit(0);

	} else {  // Error de frok
		close(pipe_leer[READ]);
		close(pipe_leer[WRITE]);
		close(pipe_nuevo[WRITE]);
		close(pipe_nuevo[READ]);
		perror("Error Fork");
		exit(-1);
	}
	return 0;
}