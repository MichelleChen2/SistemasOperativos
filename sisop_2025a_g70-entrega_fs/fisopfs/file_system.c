#include "file_system.h"


void
set_i_bitmap(file_system_t *fs, int ino){
    fs->i_bitmap[ino] = 1;
}

void
set_d_bitmap(file_system_t *fs, int idx){
    fs->d_bitmap[idx] = 1;
}

void
set_dentries(int dentry_no, inodo_t *inode, char *name, int ino){
    strcpy(inode->dentries[dentry_no].name, name);
	inode->dentries[dentry_no].ino = ino;
}


void
set_inode(inodo_t *inode, char *name, __ino_t ino, __ino_t parent_ino, int type, int blockno)
{
	strcpy (inode->name, name);
    inode->ino = ino;
    inode->parent_ino = parent_ino;
    inode->uid = geteuid();			
    inode->gid = getegid();			
    inode->size = 0;			
    inode->blksize = MAX_DATA;
    inode->blocks = 0;
    inode->atime = time(NULL);
    inode->mtime = time(NULL);
    inode->ctime = time(NULL);

    if (S_ISDIR(type)){
        inode->mode =  __S_IFDIR | 0755; 
        inode->nlink = 2;
        inode->blockno = N_A_BLOCK;
    }else{
        inode->mode =  __S_IFREG | 0644; 
        inode->nlink = 1;
        inode->blockno = blockno;
    }
}


file_system_t *
crear_file_system()
{
	file_system_t *fs_init = calloc( 1, sizeof (file_system_t));

	if (!fs_init)
		return NULL;
	//root_init
	set_inode(&fs_init->inodos[ROOT], ROOT_PATH, ROOT, ROOT,__S_IFDIR, ROOT); 
    set_i_bitmap(fs_init, ROOT);
    set_d_bitmap(fs_init, ROOT);

	return fs_init;
}


bool 
verify_dentries(int parent_ino, int child_ino, file_system_t *fs) {
    inodo_t *parent = &fs->inodos[parent_ino];
    for (int i = 0; i < MAX_INODOS; i++) {
        if (parent->dentries[i].ino == child_ino)
            return true;
    }
    return false;
}

int 
find_ino(inodo_t inodes[MAX_INODOS], int i_bitmap[MAX_INODOS], const char *name) {
    for (int i = 0; i < MAX_INODOS; i++) {
        if (!i_bitmap[i]) continue;           // Salto de inodos libres
        if (strcmp(inodes[i].name, name) == 0)
            return i;
    }
    return -1;
}

bool 
has_next_segment(char **path, char *segment) {
    char *p = *path;
    if (*p == '/') p++;              // salto slash inicial

	int i = 0;
	while (p[i] == ' ')	
 		i++;

	p = p + i;

    if (*p == '\0') return false;    // no hay más segmentos

    char *start = p;
    while (*p != '/' && *p != '\0') p++;
    size_t len = (size_t) (p - start);
    if (len >= MAX_NOMBRE) return false;
	strncpy(segment, start, len);
	segment[len] = '\0';

    *path = p;                       // para la siguiente iteración
    return true;
}

int 
find_ino_from_path(char *path, file_system_t *fs) {
    // siempre arranco en el inodo raíz
    int cur_ino = ROOT;  // p. ej. 0
    char segment[MAX_NOMBRE];

    // iterar por cada componente
    while (has_next_segment(&path, segment)) {
        int child_ino = find_ino(fs->inodos, fs->i_bitmap, segment);
        if (child_ino < 0) return -ENOENT;

        // si el inodo actual no es un dir, se devuelve error
        if (!(fs->inodos[cur_ino].mode & __S_IFDIR))
            return -ENOTDIR;

        // verificar que este child_ino está en las dentries de cur_ino
        if (!verify_dentries(cur_ino, child_ino, fs))
            return -ENOENT;

        cur_ino = child_ino;
    }
    return cur_ino;
}

/* Recibe un superbloque y devuelve el número de inodo desocupado en caso de haber
   sino devuelve error de falta de estapsio 
*/
int 
find_free_inode(file_system_t *fs)
{
    // Empiezo desde el slot después del ROOT
    int cur_ino = 1; 

    for(; cur_ino < MAX_INODOS; cur_ino++) {
        if (fs->i_bitmap[cur_ino] == 0) {
            return cur_ino; 
        }
    }
    // Error Falta de Espacio en el Dispositivo
    printf("Error: %d in FIND FREE INODE\n", cur_ino); 
    return -ENOSPC; 
}

int
find_free_blockno(file_system_t *fs) 
{
    int cur_block = 0; 

    for (; cur_block < MAX_BLOCKS; cur_block++) {
        if (fs->d_bitmap[cur_block] == 0) {
            return cur_block; 
        }
    }
    printf("Error: %d in FIND FREE BLOCKNO\n", cur_block); 
    return -ENOSPC; 
}

int
find_free_dentry_slot(inodo_t *parent_inode) 
{
    int cur_entry_slot = 0; 

    for (; cur_entry_slot < MAX_INODOS; cur_entry_slot++) {
        if(parent_inode->dentries[cur_entry_slot].ino == 0) {
            return cur_entry_slot; 
        }
    }

    printf("Error: %d in FIND FREE DENTRY SLOT\n", cur_entry_slot); 
    return -ENOSPC; 
}

void 
free_inode(file_system_t *fs, inodo_t *inode){
    fs->i_bitmap[inode->ino] = 0;

    if (!S_ISDIR(inode->mode)){
        fs->d_bitmap[inode->blockno] = 0;
    }

    inodo_t empty_inode = {0};
    fs->inodos[inode->ino] = empty_inode;
}

int
same_dentry_name_in_parent_directory(int ino_dir_parent, char *name_new_file, file_system_t *fs) 
{
    inodo_t *parent_inode = &fs->inodos[ino_dir_parent]; 

    for (int i = 0; i < MAX_INODOS; i++) {
        if ((parent_inode->dentries[i].ino >= 0) && 
        (strcmp(parent_inode->dentries[i].name, name_new_file) == 0)) {
            return parent_inode->dentries[i].ino; 
        }
    }
    return -1; 
}

int 
find_parent_ino(file_system_t *fs, char *p ){
    // Por default Parent Inode es el ROOT
	int parent_ino = 0; 

	if (strlen(p) == 0) {
       // Si el Path es del estilo "/prueba.txt"
	   // uso el ROOT como Parent Path
	   parent_ino = ROOT; 
	} else {
		// En caso que el Parent no es ROOT, busco el número de Inodo correspondiente
		char *parent = p; 
		parent_ino = find_ino_from_path(parent, fs); 

		if (parent_ino < 0) {
			return -ENOENT; 
		}
	}

    return parent_ino;
}

int 
is_dentries_empty(inodo_t *inode) {
    for (int i = 0; i < MAX_INODOS; i++) {
        if (inode->dentries[i].name[0] != '\0') {
            return 0; 
        }
    }
    return 1;
}

void
update_hierarchy_size(file_system_t *fs, inodo_t *inode, size_t old_size, size_t new_size ){
    inodo_t *parent_inode = &fs->inodos[inode->parent_ino];  

    while (parent_inode->ino != 0) {
		parent_inode->size = parent_inode->size - old_size + new_size; 
		parent_inode = &fs->inodos[parent_inode->parent_ino]; 
	}
 
	parent_inode->size = parent_inode->size - old_size + new_size;
}
