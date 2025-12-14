#ifndef __PFS_H__
#define __PFS_H__

#include <stdio.h>

#define BLANCO "\x1b[37;1m"
#define VERDE "\x1b[32;1m"
#define ROJO "\x1b[31;1m"
#define AMARILLO "\x1b[33;1m"
#define NORMAL "\x1b[0m"

#define TILDE "✓"
#define CRUZ "✗"

int __pfs_cantidad_de_pruebas_corridas = 0;
int __pfs_cantidad_de_pruebas_fallidas = 0;

void pfs_afirmar(int afirmacion, const char *descripcion)
{
	if (afirmacion) {
		printf(VERDE TILDE " ");
	} else {
		__pfs_cantidad_de_pruebas_fallidas++;
		printf(ROJO CRUZ " ");
	}
	printf(BLANCO "%s\n", descripcion);
	fflush(stdout);
	__pfs_cantidad_de_pruebas_corridas++;
}

void pfs_nuevo_grupo(const char *descripcion)
{
	printf(AMARILLO "\n%s\n", descripcion);
	while (*(descripcion++))
		printf("=");
	printf(BLANCO "\n");
}

int pfs_mostrar_reporte()
{
	printf("\n---------------------------------\n"
	       "%i pruebas corridas, %i errores - %s\n" NORMAL,
	       __pfs_cantidad_de_pruebas_corridas,
	       __pfs_cantidad_de_pruebas_fallidas,
	       __pfs_cantidad_de_pruebas_fallidas == 0 ? "OK" : "D:");
	return __pfs_cantidad_de_pruebas_fallidas;
}

#endif // __PFS_H__