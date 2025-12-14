# fisopfs

Repositorio para el esqueleto del [TP: filesystem](https://fisop.github.io/website/tps/filesystem) del curso Mendez-Fresia de **Sistemas Operativos (7508) - FIUBA**

> Sistema de archivos tipo FUSE.

## Respuestas teóricas

Utilizar el archivo `fisopfs.md` provisto en el repositorio

## Compilar

```bash
$ make
```

## Ejecutar

### Setup

Primero hay que crear un directorio de prueba:

```bash
$ mkdir prueba
```

### Iniciar el servidor FUSE

En el mismo directorio que se utilizó para compilar la solución, ejectuar:

```bash
$ ./fisopfs prueba/
```

Hay una flag `--filedisk NAME` para indicar que archivo se
 quiere utilizar como archivo de persistencia en disco. 
 El valor por defecto es "persistence_file.fisopfs"

```bash
$ ./fisopfs prueba/ --filedisk nuevo_disco.fisopfs
```

### Verificar directorio

```bash
$ mount | grep fisopfs
```

### Utilizar el directorio de "pruebas"

En otra terminal, ejecutar:

```bash
$ cd prueba
$ ls -al
```

### Limpieza

```bash
$ sudo umount prueba
```

## Docker

Existen tres _targets_ en el archivo `Makefile` para utilizar _docker_.

- `docker-build`: genera la imagen basada en "Ubuntu 20.04" con las dependencias de FUSE
- `docker-run`: crea un _container_ basado en la imagen anterior ejecutando `bash`
   - acá se puede ejecutar `make` y luego `./fisopfs -f ./prueba`
- `docker-exec`: permite vincularse al mismo _container_ anterior para poder realizar pruebas
   - acá se puede ingresar al directorio `prueba`

## Linter

```bash
$ make format
```

Para efectivamente subir los cambios producidos por el `format`, hay que `git add .` y `git commit`.


## Pruebas:

### Pruebas del TDA del Super Bloque:

En la terminal se pueden compilar con el siguiente comando:

```bash
$ gcc -std=c99 -Wall -Wconversion -Wtype-limits -pedantic -Werror -O2 -g pruebas_fs.c file_system.c -o ejecutable  
```
En la terminal se pueden ejecutar con valgrind con el siguiente comando:
```bash
$ valgrind --leak-check=full --track-origins=yes --show-reachable=yes --error-exitcode=2 --show-leak-kinds=all --trace-children=yes ./ejecutable

```

### Prueba de archivo binario:

Para probar que una imagen por ejemplo puede ser almacenada en el file system (como contenido binario), se tiene que:
1. Inicializar el programa como de costumbre (en docker), levantando el file system (FS) en una segunda terminal
2. Copiar una imagen en cualquier parte o path deseado dentro del FS, por ejemplo:
```bash
$ cp ../nombre_archivo_a_copiar.png /fisopfs/archivo_montaje_fs/nombre_archivo_imagen_destino.png
```
(Por ejemplo si la imagen estaba previamente puesta en el directorio **fisopfs**)

3. Al momento de cerrar el programa (FS), este archivo será persistido en el archivo que almacena el file system
4. Para corroborar que dicho archivo realmente se encuentra persistido, cuando se inicie de nuevo el file system
   estando en el directorio que contiene a dicha imagen se puede ejecutar el siguiente comando:
```bash
$ feh nombre_archivo.png
```
   
   con lo cual se realizará un display de la misma.