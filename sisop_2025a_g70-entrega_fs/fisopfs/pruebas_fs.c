#include "pruebas_fs.h"
#include "file_system.h"

#define MAX_PATH 50

void probar_busqueda_inodo()
{
    const int ino_1 = 1;
    const int ino_2 = 2;
    const int ino_3 = 3;
    const int ino_4 = 4;
    const int ino_5 = 5;
    
    file_system_t *fs; 				//Estructura file system global


    fs = crear_file_system();
	if (!fs)
		fprintf(stderr, "[Error] No es posible generar file system\n");
///----------------------------HARCODEO DENTRIES Y DATOS EN ARCHIVOS, TESTING--------------------------
///HEADERS :
//set_inode(inodo_t *inode, char *name, int ino, int type, int blockno)
//set_i_bitmap(file_system_t *fs, int idx)
//set_d_bitmap(file_system_t *fs, int idx)
//set_dentries(int dentry_no, inodo_t *inode, char *name, int ino)


//setting de dentries de root
    set_dentries(0, &fs->inodos[ROOT], "file_1.txt", 1);
    set_dentries(1, &fs->inodos[ROOT], "dir_1", 2);

    ///inicialización inodos de "dir_2":

///inicialización inodo de "file_1.txt":
    inodo_t *inode_1 = &fs->inodos[ino_1];
    int blockno_1 = 0;
    set_inode(inode_1, "file_1.txt", ino_1, __S_IFREG, blockno_1);
    set_d_bitmap(fs, blockno_1);
    set_i_bitmap(fs, ino_1);

	///inicialización data block de "file_1.txt":
    size_t sz_block_0 = sizeof("Este es el contenido del archivo file_1.txt!!\n");
	block_data_t *buffer_0 = &fs->blocks[inode_1->blockno];
	memcpy(buffer_0, "Este es el contenido del archivo file_1.txt!!\n", sz_block_0);

///inicialización inodo de "dir_1" (inodo N° 2):
    inodo_t *inode_2 = &fs->inodos[ino_2];
    int blockno_2 = N_A_BLOCK;
    set_inode(inode_2, "dir_1", ino_2, __S_IFDIR, blockno_2);
    set_i_bitmap(fs, ino_2);

	///inicialización dentries de "dir_1":
    set_dentries(0, &fs->inodos[ino_2], "dir_2", ino_3);
    
///------------------------------------
///inicialización inodo de "dir_2" (inodo N° 3):
    inodo_t *inode_3 = &fs->inodos[ino_3];
    int blockno_3 = N_A_BLOCK;
    set_inode(inode_3, "dir_2", ino_3, __S_IFDIR, blockno_3);
    set_i_bitmap(fs, ino_3);

	///inicialización dentries de "dir_1":
    set_dentries(0, &fs->inodos[ino_3], "file 2.yml", ino_4);
    set_dentries(1, &fs->inodos[ino_3], "dir_3", ino_5);

///------------------------------------
///inicialización inodo de "file 2.yml":
    inodo_t *inode_4 = &fs->inodos[ino_4];
    int blockno_4 = 1;
    set_inode(inode_4, "file 2.yml", ino_4, __S_IFREG, blockno_4);
    set_d_bitmap(fs, blockno_4);
    set_i_bitmap(fs, ino_4);

	///inicialización data block de "file_1.txt":
    size_t sz_block_1 = sizeof("Este es el contenido del archivo file 2.yml!!\n");
	block_data_t *buffer_1 = &fs->blocks[inode_4->blockno];
	memcpy(buffer_1, "Este es el contenido del archivo file 2.yml!!\n", sz_block_1);

///inicialización inodo de "dir_3":
    inodo_t *inode_5 = &fs->inodos[ino_5];
    int blockno_5 = N_A_BLOCK;
    set_inode(inode_5, "dir_3", ino_5, __S_IFDIR, blockno_5);
    set_i_bitmap(fs, ino_5);


    char path_1[MAX_PATH] = "/file_1.txt";
    int ino1 = find_ino_from_path(path_1, fs);
    int esperado_1 = 1;
    printf("Valor ino1: %i | Valor esperado_1: %i \n", ino1, esperado_1);
    pfs_afirmar(ino1 == esperado_1, "Busqueda de número de inodo de tipo regular, nivel de anidación 1\n");

    char path_2[MAX_PATH] = "/dir_1/dir_2";
    int ino2 = find_ino_from_path(path_2, fs);
    int esperado_2 = 3;
    printf("Valor ino2: %i | Valor esperado_2: %i \n", ino2, esperado_2);
    pfs_afirmar(ino2 == esperado_2, "Busqueda de número de inodo de tipo directorio, nivel de anidación 2\n");

    char path_3[MAX_PATH] = "/dir_1/dir_2/file 2.yml";
    int ino3 = find_ino_from_path(path_3, fs);
    int esperado_3 = 4;
    printf("Valor ino3: %i | Valor esperado_3: %i \n", ino3, esperado_3);
    pfs_afirmar(ino3 == esperado_3, "Busqueda de número de inodo de tipo regular, nivel de anidación 3\n");

    char path_4[MAX_PATH] = "/dir_1/dir_2/dir_3";
    int ino4 = find_ino_from_path(path_4, fs);
    int esperado_4 = 5;
    printf("Valor ino4: %i | Valor esperado_4: %i \n", ino4, esperado_4);
    pfs_afirmar(ino4 == esperado_4, "Busqueda de número de inodo de tipo directorio, nivel de anidación 3\n");

    char path_error[MAX_PATH] = "/dir_1/dir_2/dir_err";
    int ino_err = find_ino_from_path(path_error, fs);
    int esperado_err = -2;
    printf("Valor ino_err: %i | Valor esperado_err: %i \n", ino_err, esperado_err);
    pfs_afirmar(ino_err == esperado_err, "Devuelve error (-2) en caso de recibir un path incorrecto\n");



    free(fs);
}


int main()
{
	pfs_nuevo_grupo(
		"\n======================== PRUEBAS GENERALES ========================");
    
    pfs_nuevo_grupo(
        "\n-------------------- Pruebas busqueda de inodo ---------------------");
	probar_busqueda_inodo();

	

	return pfs_mostrar_reporte();
}
