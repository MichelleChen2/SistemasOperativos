#define FUSE_USE_VERSION 30
#define _POSIX_C_SOURCE 200809L

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include "file_system.h"

#define MAX_PATH 4096
#define DEFAULT_FILE_DISK "persistence_file.fisopfs"

char *filedisk = DEFAULT_FILE_DISK;

file_system_t *fs;  // Estructura virtual file system global


static void *
fisopfs_init(struct fuse_conn_info *conn)
{
	printf("[debug] fisopfs_init\n");

	long fbytes_no = 0;

	FILE *file = fopen(filedisk, "rb");
	if (!file) {
		fprintf(stderr, "fopen failed in fisopfs_init\n");
	} else {
		// Se revisa que el archivo no se encuentre vacío
		fseek(file, 0, SEEK_END);  // ubica el puntero en el final del archivo
		fbytes_no = ftell(file);  // cant. bytes en el archivo
		rewind(file);             // vuelve al comienzo del archivo
	}

	if (fbytes_no < sizeof(file_system_t)) {
		fs = crear_file_system();
		if (!fs) {
			fprintf(stderr, "Not posible to create a Virtual File System\n");
			fclose(file);
			return NULL;
		}
		printf("[debug] File system successfully created in "
		       "fisopfs_init\n");
	} else {
		file_system_t *fs_aux = malloc(sizeof(file_system_t));
		if (!fs_aux) {
			fprintf(stderr, "Failed malloc in fisopfs_init\n");
			fclose(file);
			return NULL;
		}

		int read_no = fread(fs_aux, sizeof(file_system_t), 1, file);
		fclose(file);

		if (read_no != 1) {
			fprintf(stderr, "Wrong number of elements read in fisopfs_init\n");
			free(fs_aux);
			return NULL;
		}

		fs = fs_aux;

		printf("[debug] File system successfully lodaded in "
		       "fisopfs_init\n");
	}

	return fs;  // ESTO POR SI DESPUES SE DESEA UTILZAR EL "private_data"
}


static void
fisopfs_destroy(void *private_data)
{
	printf("[debug] fisopfs_destroy\n");

	FILE *file = fopen(filedisk, "wb");
	if (!file) {
		fprintf(stderr,
		        "No file to save the file system in fisopfs_destroy\n");
		return;
	}

	size_t written = fwrite(fs, sizeof(file_system_t), 1, file);
	if (written != 1) {
		fprintf(stderr, "Wrong number of elements written in fisopfs_destroy\n");
	} else {
		printf("[debug] File system successfully saved\n");
	}

	fclose(file);

	free(fs);

	if (strcmp(filedisk, DEFAULT_FILE_DISK) != 0) {
		free(filedisk);
		filedisk = NULL;
	}
}


static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr - path: %s\n", path);
	char *p = strdup(path);

	if (!p) {
		fprintf(stderr, "strdup failed in fisopfs_getattr\n");
		return -ENOMEM;
	}

	int ino = find_ino_from_path(p, fs);
	free(p);

	if (ino < 0) {
		fprintf(stderr, "ino not found in fisopfs_getattr\n");
		return ino;
	}

	inodo_t *i_aux = &fs->inodos[ino];

	memset(st, 0, sizeof(struct stat));

	printf("[debug] TOTAL SIZE in GETATTR - path: %ld\n", i_aux->size);

	st->st_uid = i_aux->uid;
	st->st_mode = i_aux->mode;
	st->st_size = i_aux->size;
	st->st_nlink = i_aux->nlink;
	st->st_gid = i_aux->gid;
	st->st_atime = i_aux->atime;
	st->st_mtime = i_aux->mtime;
	st->st_ctime = i_aux->ctime;
	st->st_blocks = (i_aux->size + (MAX_DATA - 1)) / MAX_DATA;
	st->st_blksize = i_aux->blksize;

	return 0;
}

static int
fisopfs_open(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_open - path: %s\n", path);

	char *p = strdup(path);

	if (!p) {
		fprintf(stderr, "strdup failed in fisopfs_readdir\n");
		return -ENOMEM;
	}

	int ino = find_ino_from_path(p, fs);
	free(p);

	if (ino < 0) {
		fprintf(stderr, "ino not found in fisopfs_readdir\n");
		return ino;
	}

	inodo_t *inode = &fs->inodos[ino];

	if (!S_ISREG(inode->mode))
		return -EISDIR;

	struct fuse_context *ctx = fuse_get_context();
	uid_t uid = ctx->uid;
	gid_t gid = ctx->gid;

	// Se almacenan los flags solicitados por el "open syscall"
	int access_mode = fi->flags & O_ACCMODE;
	// Verificar permisos de lectura/escritura según UID/GID y modo
	if (uid == inode->uid) {
		// Propietario
		if ((access_mode == O_RDONLY && !(inode->mode & S_IRUSR)) ||
		    (access_mode == O_WRONLY && !(inode->mode & S_IWUSR)) ||
		    (access_mode == O_RDWR && !(inode->mode & S_IRUSR) &&
		     !(inode->mode & S_IWUSR)))
			return -EACCES;
	} else if (gid == inode->gid) {
		// Mismo grupo
		if ((access_mode == O_RDONLY && !(inode->mode & S_IRGRP)) ||
		    (access_mode == O_WRONLY && !(inode->mode & S_IWGRP)) ||
		    (access_mode == O_RDWR && !(inode->mode & S_IRGRP) &&
		     !(inode->mode & S_IWGRP)))
			return -EACCES;
	} else {
		// Otros
		if ((access_mode == O_RDONLY && !(inode->mode & S_IROTH)) ||
		    (access_mode == O_WRONLY && !(inode->mode & S_IWOTH)) ||
		    (access_mode == O_RDWR && !(inode->mode & S_IROTH) &&
		     !(inode->mode & S_IWOTH)))
			return -EACCES;
	}
	// Se almacena el número de inodo, pudiendose usar este en "read, write, release, etc"
	/// VER SI SE PUEDE IMPLEMENTAR CON ESTO, CON ESO NO SE TENDRÍA
	/// QUE REALIZAR DE VUELTA VERIFICACIONES REPETITIVAS
	fi->fh = ino;

	inode->atime = time(NULL);

	return 0;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir - path: %s\n", path);

	// Los directorios '.' y '..'
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	char *p = strdup(path);

	if (!p) {
		fprintf(stderr, "strdup failed in fisopfs_readdir\n");
		return -ENOMEM;
	}

	int ino = find_ino_from_path(p, fs);
	free(p);

	if (ino < 0) {
		fprintf(stderr, "ino not found in fisopfs_readdir\n");
		return ino;
	}

	inodo_t *dir = &fs->inodos[ino];

	if (!S_ISDIR(dir->mode))
		return -ENOTDIR;

	for (int i = 0; i < MAX_INODOS; i++) {
		if (strcmp(dir->dentries[i].name, "") != 0)
			filler(buffer, dir->dentries[i].name, NULL, 0);
	}

	dir->atime = time(NULL);
	dir->ctime = time(NULL);
	return 0;
}


static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);

	char *p = strdup(path);

	if (!p) {
		fprintf(stderr, "strdup failed in fisopfs_read\n");
		return -ENOMEM;
	}

	int ino = find_ino_from_path(p, fs);
	if (ino < 0)
		return ino;

	if (!S_ISREG(fs->inodos[ino].mode))
		return -EISDIR;

	inodo_t *inode = &fs->inodos[ino];
	size_t file_size = inode->size;

	if (offset >= file_size)
		return 0;

	size_t bytes_2_read = file_size - offset;
	if (bytes_2_read > size)
		bytes_2_read = size;

	int blockno = inode->blockno;
	if (blockno < 0 || blockno > MAX_BLOCKS)
		return -EIO;  // Error, bloque invalido

	block_data_t *block = &fs->blocks[blockno];

	memcpy(buffer, block->data + offset, bytes_2_read);

	inode->atime = time(NULL);
	inode->ctime = time(NULL);

	return bytes_2_read;
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_create - path: %s\n", path);

	// Copia de Path
	char *p = strdup(path);

	// Si falla strdup
	if (!p) {
		fprintf(stderr, "strdup failed in fisopfs_create\n");
		return -EPERM;
	}

	// Busco el último / del Path
	char *last_segment = strrchr(p, '/');
	if (!last_segment) {
		free(p);
		fprintf(stderr, "invalid path in fisopfs_create\n");
		return -EINVAL;
	}

	// Copio el nombre del archivo que viene después del último /
	char *file_name = strdup(last_segment + 1);
	// Fin de str en la posición del último / y obtengo el Parent Path
	*last_segment = '\0';

	int parent_ino = find_parent_ino(fs, p);
	if (parent_ino < 0) {
		free(p);
		free(file_name);
		fprintf(stderr, "inexistent path in fisopfs_create\n");
	}

	printf("[debug] fisopfs_create - File Name: %s, Parent Inode: %d\n",
	       path,
	       parent_ino);

	// Verifico que el Inodo Parent es del tipo Directorio
	inodo_t *parent_inode = &fs->inodos[parent_ino];

	if (!S_ISDIR(parent_inode->mode)) {
		free(file_name);
		free(p);
		fprintf(stderr, "it isn't type Directory fisopfs_create\n");
		return -ENOTDIR;
	}

	// Verficar que el nuevo nombre no existe ya en alguno de los dentries del directorio padre
	// touch actuliza el tiempo de modificación en caso de que exista el archivo
	int inode_same_name_pos =
	        same_dentry_name_in_parent_directory(parent_ino, file_name, fs);
	if (inode_same_name_pos >= 0) {
		inodo_t *same_name_file = &fs->inodos[inode_same_name_pos];
		same_name_file->atime = time(NULL);
		same_name_file->mtime = time(NULL);
		same_name_file->ctime = time(NULL);
		free(p);
		free(file_name);
		return 0;
	}

	int free_blockno = find_free_blockno(fs);

	if (free_blockno < 0 || free_blockno > MAX_BLOCKS) {
		free(p);
		free(file_name);
		fprintf(stderr, "no blockno for new file, failed creation in fisopfs_create\n");
		return -ENOSPC;
	}

	int free_inode = find_free_inode(fs);

	if (free_inode < 0) {
		free(p);
		free(file_name);
		fprintf(stderr, "no space for new file, failed creation in fisopfs_create\n");
		return -ENOSPC;
	}

	int free_dentry_slot_in_parent = find_free_dentry_slot(parent_inode);

	if (free_dentry_slot_in_parent < 0) {
		free(p);
		free(file_name);
		fprintf(stderr, "no free dentry slot in this Directory, failed creation in fisopfs_create\n");
		return -ENOSPC;
	}

	inodo_t *new_inode = &fs->inodos[free_inode];
	set_inode(new_inode, file_name, free_inode, parent_ino, mode, free_blockno);
	set_dentries(free_dentry_slot_in_parent, parent_inode, file_name, free_inode);
	set_i_bitmap(fs, free_inode);
	set_d_bitmap(fs, free_blockno);
	memset(fs->blocks[free_blockno].data, 0, MAX_DATA);

	printf("[debug] fisopfs_create - Blocks: %ld\n", new_inode->blocks);

	parent_inode->mtime = time(NULL);
	parent_inode->atime = time(NULL);
	parent_inode->ctime = time(NULL);

	free(file_name);
	free(p);
	return 0;
}

static int
fisopfs_utimens(const char *path, const struct timespec ts[2])
{
	printf("[debug] fisopfs_utimens - path: %s\n", path);

	char *p = strdup(path);

	if (!p) {
		fprintf(stderr, "strdup failed in fisopfs_utimens\n");
		return -ENOMEM;
	}

	int ino = find_ino_from_path(p, fs);

	if (ino < 0) {
		free(p);
		fprintf(stderr,
		        "find inode from path failed in fisopfs_utimens\n");
		return ino;
	}

	inodo_t *inode = &fs->inodos[ino];

	if (ts) {
		inode->atime = ts[0].tv_sec;
		inode->mtime = ts[1].tv_sec;
		inode->ctime = ts[0].tv_sec;
	} else {
		inode->atime = time(NULL);
		inode->mtime = time(NULL);
		inode->ctime = time(NULL);
	}

	free(p);
	return 0;
}


static int
fisopfs_write(const char *path,
              const char *buf,
              size_t size,
              off_t off,
              struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_write - path: %s, contenido: %s, offset: %ld, "
	       "size: %ld\n",
	       path,
	       buf,
	       off,
	       size);
	char *p = strdup(path);

	if (!p) {
		fprintf(stderr, "strdup failed in fisopfs_write\n");
		return -ENOMEM;
	}

	int ino = find_ino_from_path(p, fs);
	free(p);

	if (ino < 0) {
		fprintf(stderr, "find inode from path failed in fisopfs_write\n");
		return ino;
	}

	inodo_t *inode = &fs->inodos[ino];

	if (S_ISDIR(inode->mode)) {
		fprintf(stderr, "cant write in Directory in fisopfs_write\n");
		return -EISDIR;
	}

	if (inode->mode & (S_ISUID | S_ISGID)) {
		inode->mode &= ~(S_ISUID | S_ISGID);
		inode->mtime = time(NULL);
		inode->ctime = time(NULL);
	}

	size_t file_size = inode->size;
	size_t final_size = file_size + size;

	if (off >= MAX_DATA) {
		fprintf(stderr,
		        "offset greater than max file size in fisopfs_write\n");
		return -EFBIG;
	}

	if (final_size > MAX_DATA) {
		fprintf(stderr, "Not enough space for more content writing in fisopfs_write\n");
		return -ENOMEM;
	}

	size_t max_size_available_to_write = MAX_DATA - off;
	size_t ajusted_size = (size <= max_size_available_to_write)
	                              ? size
	                              : max_size_available_to_write;

	int blockno = inode->blockno;
	if (blockno < 0 || blockno > MAX_BLOCKS) {
		fprintf(stderr, "invalid blockno in fisopfs_write\n");
		return -EIO;
	}


	block_data_t *block = &fs->blocks[blockno];
	memcpy(block->data + off, buf, ajusted_size);

	off_t new_size = file_size;

	if ((off + size) >= file_size) {
		new_size = off + size;
	}

	inode->size = new_size;
	inode->atime = time(NULL);
	inode->mtime = time(NULL);
	inode->ctime = time(NULL);

	update_hierarchy_size(fs, inode, file_size, new_size);

	printf("[debug] NEW PARENT INODE SIZE in WRITE - path: %ln\n",
	       &fs->inodos[ROOT].size);

	return (int) ajusted_size;
}

static int
fisopfs_release(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_release - path: %s\n", path);

	int ino = (int) fi->fh;
	if (ino < 0 || ino >= MAX_INODOS) {
		fprintf(stderr, "ivalid inode number fisopfs_release\n");
		return ino;
	}

	// No hago release de ningún File Descriptor porque en fi->fh solamente está guardado el número de Inodo
	inodo_t *inode = &fs->inodos[ino];
	inode->atime = time(NULL);
	inode->ctime = time(NULL);

	return 0;
}

static int
fisopfs_truncate(const char *path, off_t off)
{
	printf("[debug] fisopfs_truncate - path: %s\n", path);

	char *p = strdup(path);

	if (!p) {
		fprintf(stderr, "strdup failed in fisopfs_truncate\n");
		return -ENOMEM;
	}

	int ino = find_ino_from_path(p, fs);
	free(p);

	if (ino < 0) {
		fprintf(stderr,
		        "find inode from path failed in fisopfs_truncate\n");
		return ino;
	}

	inodo_t *inode = &fs->inodos[ino];

	if (S_ISDIR(inode->mode)) {
		fprintf(stderr, "File is Directory in fisopfs_truncate\n");
		return -EISDIR;
	}

	int blockno = inode->blockno;

	if (blockno < 0 || blockno > MAX_BLOCKS) {
		fprintf(stderr, "invalid blockno in fisopfs_truncate\n");
		return -EIO;
	}

	if (off > MAX_DATA) {
		return -EFBIG;
	}

	update_hierarchy_size(fs, inode, inode->size, off);

	inode->size = off;

	inode->mtime = time(NULL);
	inode->atime = time(NULL);
	inode->ctime = time(NULL);
	return 0;
}

static int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_release - path: %s\n", path);

	FILE *file = fopen(filedisk, "wb");

	if (!file) {
		fprintf(stderr, "fopen failed in fisopfs_flush\n");
		return -ENOENT;
	}

	size_t size_written = fwrite(fs, sizeof(file_system_t), 1, file);

	if (size_written < 1) {
		fclose(file);
		return -EIO;
	}

	if (fflush(file) != 0) {
		fclose(file);
		return -EIO;
	}

	// Verificar si es necesario
	int fd = fileno(file);
	if (fd < 0 || fsync(fd) < 0) {
		fclose(file);
		return -EIO;
	}

	fclose(file);
	return 0;
}

int
fisopsfs_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink - path: %s\n", path);

	char *p = strdup(path);

	if (!p) {
		fprintf(stderr, "strdup failed in fisopfs_unlink\n");
		return -ENOMEM;
	}

	int ino = find_ino_from_path(p, fs);
	free(p);

	if (ino < 0) {
		fprintf(stderr, "find inode from path failed in fisopfs_unlink\n");
		return ino;
	}

	inodo_t *inode = &fs->inodos[ino];

	if (S_ISDIR(inode->mode)) {
		fprintf(stderr, "it isn't type Directory fisopfs_unlink\n");
		return -EISDIR;
	}

	if (inode->blockno < 0 || inode->blockno > MAX_BLOCKS) {
		fprintf(stderr, "invalid blockno in fisopfs_unlink\n");
		return -EIO;
	}

	memset(fs->blocks[inode->blockno].data, 0, MAX_DATA);

	inodo_t *parent_inode = &fs->inodos[inode->parent_ino];
	update_hierarchy_size(fs, inode, inode->size, 0);

	for (int i = 0; i < MAX_INODOS; i++) {
		if (parent_inode->dentries[i].ino == ino) {
			parent_inode->dentries[i].ino = 0;
			parent_inode->dentries[i].name[0] = '\0';
			break;
		}
	}

	parent_inode->mtime = time(NULL);
	parent_inode->atime = time(NULL);
	parent_inode->ctime = time(NULL);
	free_inode(fs, inode);

	return 0;
}

int
fisopsfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_mkdir - path: %s\n", path);

	char *p = strdup(path);

	if (!p) {
		fprintf(stderr, "strdup failed in fisopfs_mkdir\n");
		return -EPERM;
	}

	char *last_segment = strrchr(p, '/');
	if (!last_segment) {
		free(p);
		fprintf(stderr, "invalid path in fisopfs_mkdir\n");

		return -EINVAL;
	}

	// Copio el nombre del dir que viene después del último /
	char *dir_name = strdup(last_segment + 1);
	*last_segment = '\0';
	// Por default Parent Inode es el ROOT
	int parent_ino = 0;

	if (strlen(p) == 0) {
		// Si el Path es del estilo "/prueba"
		// uso el ROOT como Parent Path
		parent_ino = ROOT;
	} else {
		char *parent = p;
		parent_ino = find_ino_from_path(parent, fs);

		if (parent_ino < 0) {
			free(p);
			free(dir_name);
			fprintf(stderr, "inexistent path in fisopfs_mkdir\n");

			return -ENOENT;
		}
	}

	printf("[debug] fisopfs_mkdir - File Name: %s, Parent Inode: %d\n",
	       path,
	       parent_ino);

	// Verifico que el Inodo Parent es del tipo Directorio
	inodo_t *parent_inode = &fs->inodos[parent_ino];

	if (!S_ISDIR(parent_inode->mode)) {
		free(dir_name);
		free(p);
		fprintf(stderr, "it isn't type Directory fisopfs_mkdir\n");

		return -ENOTDIR;
	}

	// Verficar que el nuevo nombre no existe ya en alguno de los dentries del directorio padre
	int inode_same_name_pos =
	        same_dentry_name_in_parent_directory(parent_ino, dir_name, fs);
	if (inode_same_name_pos >= 0) {
		inodo_t *same_name_dir = &fs->inodos[inode_same_name_pos];
		same_name_dir->atime = time(NULL);
		same_name_dir->mtime = time(NULL);
		same_name_dir->ctime = time(NULL);
		free(p);
		free(dir_name);

		return 0;
	}

	int free_inode = find_free_inode(fs);

	if (free_inode < 0) {
		free(p);
		free(dir_name);
		fprintf(stderr, "no space for new dir, failed creation in fisopfs_mkdir\n");

		return -ENOSPC;
	}

	int free_dentry_slot_in_parent = find_free_dentry_slot(parent_inode);

	if (free_dentry_slot_in_parent < 0) {
		free(p);
		free(dir_name);
		fprintf(stderr, "no free dentry slot in this Directory, failed creation in fisopfs_mkdir\n");

		return -ENOSPC;
	}

	inodo_t *new_inode = &fs->inodos[free_inode];
	set_inode(new_inode, dir_name, free_inode, parent_ino, __S_IFDIR, ROOT);
	set_dentries(free_dentry_slot_in_parent, parent_inode, dir_name, free_inode);
	set_i_bitmap(fs, free_inode);

	parent_inode->mtime = time(NULL);
	parent_inode->atime = time(NULL);
	parent_inode->ctime = time(NULL);

	free(dir_name);
	free(p);

	printf("[debug] fisopfs_mkdir - path: %s created succesfully\n", path);


	return 0;
}

int
fisopsfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir - path: %s\n", path);
	char *p = strdup(path);

	if (!p) {
		fprintf(stderr, "strdup failed in fisopfs_rmdir\n");
		return -ENOMEM;
	}

	int ino = find_ino_from_path(p, fs);
	if (ino < 0) {
		free(p);
		fprintf(stderr, "find inode from path failed in fisopfs_rmdir\n");
		return ino;
	}

	inodo_t *inode = &fs->inodos[ino];

	if (!S_ISDIR(inode->mode)) {
		free(p);
		fprintf(stderr, "cant rmdir if its not a dir in fisopfs_rmdir\n");
		return -ENOTDIR;
	}

	if (!is_dentries_empty(inode)) {
		free(p);
		fprintf(stderr, "cant remove not empty dir in fisopfs_rmdir\n");
		return -ENOTEMPTY;
	}

	// Busco el último / del Path
	char *last_segment = strrchr(p, '/');
	if (!last_segment) {
		free(p);
		fprintf(stderr, "invalid path in fisopfs_rmdir\n");
		return -EINVAL;
	}

	char *file_name = strdup(last_segment + 1);
	*last_segment = '\0';

	inodo_t *p_ino = &fs->inodos[inode->parent_ino];

	int i;
	for (i = 0; i < MAX_INODOS; i++) {
		if (strcmp(p_ino->dentries[i].name, file_name) == 0) {
			p_ino->dentries[i].ino = 0;
			strcpy(p_ino->dentries[i].name, "");
			break;
		}
	}

	if (i == MAX_INODOS) {
		// No se encontró ningún dentry con ese nombre
		fprintf(stderr, "dentry not found in parent in fisopfs_rmdir\n");
		free(file_name);
		free(p);
		return -ENOENT;
	}

	update_hierarchy_size(fs, inode, inode->size, 0);
	p_ino->mtime = time(NULL);
	p_ino->atime = time(NULL);
	p_ino->ctime = time(NULL);


	free_inode(fs, inode);
	return 0;
}


static struct fuse_operations operations = {
	.init = fisopfs_init,
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.read = fisopfs_read,
	.destroy = fisopfs_destroy,
	.create = fisopfs_create,
	.utimens = fisopfs_utimens,
	.release = fisopfs_release,
	.truncate = fisopfs_truncate,
	.flush = fisopfs_flush,
	.write = fisopfs_write,
	.open = fisopfs_open,
	.unlink = fisopsfs_unlink,
	.mkdir = fisopsfs_mkdir,
	.rmdir = fisopsfs_rmdir,
};

void
set_abs_path(char *abs_path, char *cwd, char *arg)
{
	strncpy(abs_path, cwd, MAX_PATH - 1);
	abs_path[MAX_PATH - 1] = '\0';
	strncat(abs_path, "/", MAX_PATH - strlen(abs_path) - 1);
	strncat(abs_path, arg, MAX_PATH - strlen(abs_path) - 1);

	filedisk = strdup(abs_path);
	if (!filedisk) {
		fprintf(stderr,
		        "[Error] strdup failed for filedisk in set_abs_path\n");
		exit(EXIT_FAILURE);
	}
}


/* We remove the argument so that fuse doesn't use our
   argument or name as folder.
   Equivalent to a pop.
*/
static void
remove_argument(int *argc, char *argv[], int i, int leap)
{
	for (int j = i; j < *argc - leap; j++) {
		argv[j] = argv[j + leap];
	}
	*argc -= leap;
}


/* Establece a partir del CWD el path absoluto del archivo que se va
   a encargar de persistir el file system
*/
void
set_persistence_file_path(int *argc, char *argv[])
{
	bool file_name_set = false;
	char cwd[MAX_PATH];
	char abs_path[MAX_PATH];

	if (!getcwd(cwd, MAX_PATH)) {
		fprintf(stderr, "[Error] Not CWD available in set_persistence_file_path\n");
		exit(EXIT_FAILURE);
	}

	printf("[debug] CWD OBTENIDO: %s\n", cwd);

	for (int i = 1; i < *argc - 1; i++) {
		if (strcmp(argv[i], "--filedisk") == 0) {
			if (argv[i + 1][0] == '\0') {
				fprintf(stderr, "[Warning] Empty filename provided for --filedisk. Using default\n");
				remove_argument(argc, argv, i, 1);
				break;
			}

			set_abs_path(abs_path, cwd, argv[i + 1]);

			file_name_set = true;
			printf("[debug] Concatenate CWD and filedisk - "
			       "absolute path, from file name: %s\n",
			       filedisk);
			remove_argument(argc, argv, i, 2);
			break;
		}
	}
	if (file_name_set == false) {
		set_abs_path(abs_path, cwd, filedisk);

		printf("[debug] Concatenate CWD and filedisk - absolute path, "
		       "with filedisk default: %s\n",
		       filedisk);
	}
}


int
main(int argc, char *argv[])
{
	set_persistence_file_path(&argc, argv);

	return fuse_main(argc, argv, &operations, NULL);
}
