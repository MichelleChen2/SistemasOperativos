#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>




#define MAX_NOMBRE 20
#define MAX_INODOS 10
#define MAX_DATA 300000				//Cantidad máxima de escritura por bloque de datos
#define MAX_BLOCKS 10
#define N_A_BLOCK -1
#define ROOT 0
#define ROOT_PATH "/"


typedef struct block_data{
    char data [MAX_DATA];           //Bloque de datos  
}block_data_t;

typedef struct dentry{
    char name [MAX_NOMBRE];
    int ino;                		//int numero_inodo         
}dentry_t;

typedef struct inodo{
    char name [MAX_NOMBRE];			//path
    __ino_t ino;					//int numero_inodo
    __ino_t parent_ino; 
    __mode_t mode;					//int tipo de inodo (Reg, Dir, etc) | permisos
    __nlink_t nlink;				//int cant. hardlinks
    __uid_t uid;                	//User ID of the file's owner
    __gid_t gid;  					//Group ID of the file's group
    __off_t size;					//Size of file, in bytes
    __blksize_t blksize;       	//Optimal block size for I/O
    __blkcnt_t blocks;			//Number 512-byte blocks allocated
    __time_t atime;					//Time of last access
    __time_t mtime;					//Time of last modification
    __time_t ctime;				//Time of last status change
	int blockno;					//nro de bloque en el que estará la data 	///SÓLO PARA REGULARES __S_IFREG
	dentry_t dentries[MAX_INODOS];	//dentries del directorio					///SÓLO PARA DIRECTORIOS __S_IFDIR
}inodo_t;

typedef struct file_system{
    int i_bitmap [MAX_INODOS];		//Índice para el vector de inodos. EMPTY = 0, FULL = 1 
    inodo_t inodos [MAX_INODOS];
    int d_bitmap [MAX_BLOCKS];		//Índice para el vector de bloques de data. EMPTY = 0, FULL = 1
    block_data_t blocks [MAX_BLOCKS]; 
}file_system_t;



void set_inode(inodo_t *inode, char *name, __ino_t ino, __ino_t parent_ino, int type, int blockno);

void set_i_bitmap(file_system_t *fs, int ino);

void set_d_bitmap(file_system_t *fs, int idx);

void set_dentries(int dentry_no, inodo_t *inode, char *name, int ino);

/* 
 *Crea un virtual file system
 *Inicializa al inodo raíz como el inodo en la posición 0 tanto para el vector
  de inodos como para el vector de bloques de data
 */
file_system_t * crear_file_system();

bool verify_dentries(int parent_ino, int child_ino, file_system_t *fs);

int find_ino(inodo_t inodes[MAX_INODOS], int i_bitmap[MAX_INODOS], const char *name);

/* Devuelve el siguiente segmento en 'segment' y ajusta *path.
 * Ej: si *path == "/foo/bar/baz", tras la llamada:
    segment == "foo", *path == "/bar/baz" 
*/
bool has_next_segment(char **path, char *segment);

int find_ino_from_path(char *path, file_system_t *fs);

int find_free_inode(file_system_t *fs); 

int find_free_blockno(file_system_t *fs); 

int find_free_dentry_slot(inodo_t *parent_inode); 

void 
free_inode(file_system_t *fs, inodo_t *inode);

int same_dentry_name_in_parent_directory(int ino_dir_parent, char *name_new_file, file_system_t *fs); 

int 
find_parent_ino(file_system_t *fs, char *p );

int is_dentries_empty(inodo_t *inode);

void
update_hierarchy_size(file_system_t *fs, inodo_t *inode, size_t old_size, size_t new_size );


#endif /* __FILE_SYSTEM_H__ */