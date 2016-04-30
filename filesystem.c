/* This file contains the definition of the functions that must be implemented
 * to allow the user access to the file system and the files.
 */

#include "include/filesystem.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/***************************/
/*    Global resources     */
/***************************/

#define FNAME_MAX 64
#define TAG_NAME_MAX 32
#define TAG_MAX_SIZE 30
//#define MAGIC_NUMBER 16797
#define MAX_ITEMS 50 // Número máximo de inodos y bloques de ficheros

#define SIZE_BLOCK 4096
#define SIZE_SUPERBLOCK 28 //en bytes
#define SIZE_ARRAY_ETIQUETAS 990
#define SIZE_ARRAY_ETIQUETAS_INODOS 600
#define SIZE_INODE 70

// ESTO ES EN DISCO (metadatos)

struct superblock {

	unsigned short maxNumItems; /* 50 ficheros*/
	unsigned short numItems;	/* numero de ficheros que el sistema tiene ACTUALMENTE = numBloquesDatos ACTUALMENTE, (max=50)*/

	unsigned short numBloqueSAs; 				/* numero de bloque del Superbloque y las arrays -> 0 */
	unsigned short numBloqueInodos; 			/* numero de bloque de los inodos -> 1 */
	unsigned short numBloquePrimerBloqueDatos; 	/* numero de bloque del primer bloque de datos -> 2*/

	unsigned short ordenSuperbloque;			/* va el primero dentro del bloque 0 */
	unsigned short ordenArrayEtiquetas;			/* va el segundo dentro del bloque 0 */

	unsigned short sizeBloque; 					/* 4096 bytes */
	unsigned short sizeSuperBloque;				/* 24 bytes */  //He ejecutado este programita y me sale 28
	unsigned short sizeArrayEtiquetas_Inodos;	/* 1650 bytes*/
	unsigned short sizeArrayEtiquetas;			/* 990 bytes*/
	unsigned short sizeINode; 					/* 76 bytes */  //Y el inodo 72

	unsigned int tamanyoDispositivo ; /* tamaño del dispositivo dado por el usuario, en bytes*/
} ;

struct inodoFichero{
	char nombre [65]; 				/*nombre fichero*/
	unsigned short tamanyo;      	/* tamaño ACTUAL de un fichero concreto*/
	unsigned short idBloque;		/* bloque donde esta un fichero concreto  (empieza en 2)*/
//	unsigned short indexTag;
	/* indice para encontrar las etiquetas en un inodo (en array_etiquetas_inodos)
	Va por orden de creacion de ficheros , el primero es el 0*/
} ;

	int array_etiquetas_inodos[50][3];					/* una fila por cada fichero*/
	char array_etiquetas[30][33];						/* para que no haya mas de 30 etiquetas*/

// ESTO VA EN MEMORIA

//tabla de ficheros -> inicializar ambos a cero
int openFileDescriptors[MAX_ITEMS][2];
unsigned int openFileDescriptorsIndex;
int array_etiquetas_index;

//char * current[MAX_ITEMS]; //Posicion de fichero
char discoBloques[52][4096]; //Bloques del disco

struct superblock * superbloqueAux;// puntero al superbloque en discoBloques[0]


/***************************/
/* File system management. */
/***************************/

/*
 * Formats a device.
 * Returns 0 if the operation was correct or -1 in case of error.
 */
int mkFS(int maxNumFiles, long deviceSize) {
	int bwritten, i;
	// Comprobaciones iniciales de parámetros
	if (maxNumFiles != MAX_ITEMS){
		return -1;
	}
	if (deviceSize < 320*1024 || deviceSize > 500*1024){
		return -1;
	}
	// Inicializamos el superbloque a ser escrito en disco
	struct superblock superbloque;
	superbloque.maxNumItems = maxNumFiles;
	superbloque.numItems = 0;
	superbloque.numBloqueSAs = 0;
	superbloque.numBloqueInodos = 1;
	superbloque.numBloquePrimerBloqueDatos = 2;
	superbloque.ordenSuperbloque = 0;
	superbloque.ordenArrayEtiquetas = 1;
	superbloque.sizeBloque = SIZE_BLOCK;
	superbloque.sizeSuperBloque = SIZE_SUPERBLOCK;
	superbloque.sizeArrayEtiquetas_Inodos = SIZE_ARRAY_ETIQUETAS_INODOS;
	superbloque.sizeArrayEtiquetas = SIZE_ARRAY_ETIQUETAS;
	superbloque.sizeINode = SIZE_INODE;
	superbloque.tamanyoDispositivo = deviceSize;
	// Inicializamos el array de etiquetas y de etiquetas_inodos a 0
	memset(array_etiquetas, '\0', 33*MAX_ITEMS);
	memset(array_etiquetas_inodos, -1, MAX_ITEMS*3*sizeof(int));
	// Inicializamos el inodo vacío
	struct inodoFichero inodo;
	memset(inodo.nombre, '\0', 65);
	inodo.tamanyo = 0;
	inodo.idBloque = 2;
	// Creamos el buffer para la escritura en disco
	char * buffer = malloc(SIZE_SUPERBLOCK+SIZE_ARRAY_ETIQUETAS+SIZE_ARRAY_ETIQUETAS_INODOS);
	memcpy(buffer, &superbloque, SIZE_SUPERBLOCK);
	memcpy(buffer+SIZE_SUPERBLOCK, array_etiquetas, SIZE_ARRAY_ETIQUETAS);
	memcpy(buffer+SIZE_SUPERBLOCK+SIZE_ARRAY_ETIQUETAS, array_etiquetas_inodos, SIZE_ARRAY_ETIQUETAS_INODOS);
	bwritten = bwrite("disk.dat", 0, buffer);
	if (bwritten == -1){
		return -1;
	}
	free(buffer);
	buffer = malloc(SIZE_INODE*50);
	inodo.idBloque = 1;
	for(i = 0; i<50; i++){
		memcpy(buffer+(SIZE_INODE*i), &inodo, SIZE_INODE);
		inodo.idBloque++;
	}
	bwritten = bwrite("disk.dat", 1, buffer);
	if (bwritten == -1){
		return -1;
	}
	free(buffer);
	return 0;
}

/*
 * Mounts a file system from the device deviceName.
 * Returns 0 if the operation was correct or -1 in case of error.
 */
int mountFS() {
	int i;
	for(i=0;i<MAX_ITEMS+2;i++){
		memset(discoBloques[i], '\0', 4096);
	}
	int bytesRead;
	//leer bloque 0 y 1 de disco a discoBloques[0] y discoBloques[1] que contienen los metadatos
	i = 0;
	while(i<2){
		bytesRead = bread("disk.dat", i, discoBloques[i]);
		if(bytesRead){
			return -1;
		}
		i++;
	}
	memset(openFileDescriptors, 0, MAX_ITEMS*2*sizeof(int));
	// La línea de arriba está haciendo lo mismo que el for que has puesto abajo, lo pongo en comment ^^
	/*
	int i;
	for(i=0; i<MAX_ITEMS; i++){
		openFileDescriptors[i]=0;
	}*/
	openFileDescriptorsIndex=0;
	// Apuntamos al superbloque en memoria con el puntero global
	superbloqueAux = (struct superblock *) discoBloques[0];

	return 0;
}

/*
 * Unmount file system.
 * Returns 0 if the operation was correct or -1 in case of error.
 */
int umountFS() {
	int bytesWritten, i;
	//escribir los metadatos en disco (bloques 0 y 1)
	i=0;
	while(i<2){
		bytesWritten = bwrite("disk.dat", i, discoBloques[i]);
		if(bytesWritten){
			return -1;
		}
		i++;
	}
	return 0;
}

/*******************/
/* File read/write */
/*******************/

/*
 * Creates a new file, if it doesn't exist.
 * Returns 0 if a new file is created, 1 if the file already exists or -1 in
 * case of error.
 */
int creatFS(char *fileName) {
	/* 	- Si fileName.size > FNAME_MAX 64
			- return -1
		- else if numItems == maxNumItems
			- reutrn -1
		- else: (continuar)
			- Si numBLoquePrimerBloqueDatos = 2 -> primer fichero que se crea
				- numBLoquePrimerBloqueDatos = 2
				- inicializar newInode:
					- nombre = fileName
					- tamanyo = 0 bytes
					- idBLoque =  numItems + numBloquePrimerBloqueDatos
				- numItems++
				- meter newInode al final del bloque 1
				- reescribir con bwrite el bloque 1 en disco
				- reescribir con bwrite el superbloque (numItems)
				-return 0

			- else:
				- llevarse a memoria el bloque e inodos(1) con bread
				- Si nombre del inodo == fileName (for con numItems)
					- return -1
				- Si hemos conseguido salir del bucle -> es un nombre nuevo
					- inicializar newInode:
					- nombre = fileName
					- tamanyo = 0 bytes
					- idBLoque =  numItems + numBloquePrimerBloqueDatos
				- numItems++
				- meter newInode al final del bloque 1 (indice = numItems * sizeInodos)
				- reescribir con bwrite el bloque 1 en disco
				- reescribir con bwrite el superbloque (numItems)
				-return 0
	*/

	struct inodoFichero nuevoInodo;
	//int bwritten, i;
	int i;
	if(strlen(fileName)> FNAME_MAX){
		return -1;
	}
	if(superbloqueAux->numItems == superbloqueAux->maxNumItems){
		return -1;
	}else if(superbloqueAux->numItems == 0){
		memcpy(nuevoInodo.nombre, fileName, strlen(fileName)+1);
		nuevoInodo.tamanyo = 0;
		nuevoInodo.idBloque = superbloqueAux->numBloquePrimerBloqueDatos;

		superbloqueAux->numItems++;
		memcpy(discoBloques[0], superbloqueAux, superbloqueAux->sizeSuperBloque);

		memcpy(discoBloques[1], &nuevoInodo, SIZE_INODE);
		return 0;
	} else {
		struct inodoFichero * inodoAux;
		for(i=0;i<superbloqueAux->maxNumItems;i++){
			inodoAux = (struct inodoFichero *) &discoBloques[1][i *superbloqueAux->sizeINode];
			if(strcmp(inodoAux->nombre, fileName)==0){
				return 1;
			}
		}
		memcpy(nuevoInodo.nombre, fileName, strlen(fileName)+1);
		nuevoInodo.tamanyo = 0;
		nuevoInodo.idBloque = superbloqueAux->numBloquePrimerBloqueDatos+superbloqueAux->numItems;

		superbloqueAux->numItems++;
		memcpy(discoBloques[0], superbloqueAux, superbloqueAux->sizeSuperBloque);

		memcpy(&discoBloques[1][(nuevoInodo.idBloque-superbloqueAux->numBloquePrimerBloqueDatos)*superbloqueAux->sizeINode], &nuevoInodo, superbloqueAux->sizeINode);
		return 0;
	}
}

/*
 * Opens an existing file.
 * Returns file descriptor if possible, -1 if file does not exist or -1 in case
 * of error.
 */
int openFS(char *fileName) {
	/* 	- Si fileName.size > FNAME_MAX 64
			- return -1
		- else if no hay mas hueco libre en openFileDescriptorsIndex -> todos los ficheros estan abiertos
			- return -1
		- else:
			- Si ya esta abierto, devolver -1
			- else: Recorrer todos los i-nodos:
				- Si fileName == inodo.nombre:
					- Coger de ese inodo su idBloque
					- meter el idBloque en openFileDescriptor (1a posicion con idBLoque 0)
					- return indice
				- Pero si hemos pasado todos los inodos es que no existe
					- return -1 */
	if(strlen(fileName)> FNAME_MAX +1){ // +1 por el caracter null
		return -1;
	}
	else if (openFileDescriptorsIndex == superbloqueAux->maxNumItems){
		return -1;
	}

	else {

		struct inodoFichero * nuevoInodo;
		int i,j;
		//intenta localizar el bloque por el criterio fileName
		for (i=0; i<superbloqueAux->numItems; i++){
			nuevoInodo = (struct inodoFichero *) &discoBloques[1][i*superbloqueAux->sizeINode];
			if(strcmp(nuevoInodo->nombre, fileName)== 0){ //el fichero existe
				for (j=0; j<superbloqueAux->numItems;j++){// mira si ya esta abierto
					if(openFileDescriptors[i][0] == nuevoInodo->idBloque){
						return -1;
					}
				}
				//el fichero existe pero NO esta abierto
				openFileDescriptors[openFileDescriptorsIndex][0] = nuevoInodo->idBloque;
				openFileDescriptors[openFileDescriptorsIndex][1] = 0;
				int fd = openFileDescriptorsIndex;
				//buscar lo del puntero de escritura
				//actualizacion de openFileDescriptorsIndex
				j=0;
				while(j<superbloqueAux->numItems && openFileDescriptors[j][0]!=0){//idBLoque minimo es dos
					j++;
				}
				openFileDescriptorsIndex=j;

				return fd;//openFileDescriptorsIndex sera como max = maxNumItems (50)

			}
		}
		return -1; //el fichero no existe
	}
}

/*
 * Closes a file.
 * Returns 0 if the operation was correct or -1 in case of error.
 */
int closeFS(int fileDescriptor) {
	/*
		- if fileDescriptor >= maxNumItems  -> fd no valido
			- return -1
		- else:
			- llegar en openFileDescriptors hasta la posicion indicada en fileDescriptor
				- Si la posicions indicada  es igual a cero: -> el fichero no estaba abierto
					- return -1
				- else: ->
					- poner dicha posicion a cero
					- actualizar openFileDescriptorsIndex
					- return 0*/

	if(fileDescriptor >= superbloqueAux->maxNumItems){
		return -1;
	}
	else{
		if(openFileDescriptors[fileDescriptor][0] == 0){
			return -1;
		}
		else{
			openFileDescriptors[fileDescriptor][0] = 0;
			openFileDescriptors[fileDescriptor][1] = 0;
			int i;
			for(i=0; i<superbloqueAux->maxNumItems; i++){
				if(openFileDescriptors[i][0]==0){
					openFileDescriptorsIndex--;
					break;
				}
			}
			// si pasa este for, openFileDescriptorsIndex == 50-> todos los ficheros estan abiertos

			return 0;
		}
	}
}

/*
 * Reads a number of bytes from a file and stores them in a buffer.
 * Returns the number of bytes read or -1 in case of error.
 */
int readFS(int fileDescriptor, void *buffer, int numBytes) {
	/* Iterar dentro de la array openFileDescriptors hasta encontrar fileDescriptor

	 - Si no lo encuentro , retornar -1
	 - Si lo encuentro:
	 	- Irme al nodo correspondiente (con idBlock)
	 	- Si numBytes > tamanyo
	 		- retornar -1
	 	else:
	 	- situar mi puntero current al inicio del bloque correspondiente
	 	- contador a cero
	 	- leer los numbytes requeridos y copiarlos en el buffer, contador ++
	 	- repetir hasta que contador > numbytes		Esto lo he modificado un poco en el código real, ya cambiaré el pseudocódigo
	*/
	if(openFileDescriptorsIndex == 0){
		return -1;
	}
	if(numBytes>SIZE_BLOCK){
		return -1;
	}
	if(openFileDescriptors[fileDescriptor][0] == 0){
		return -1;
	}
	int i;
	struct inodoFichero * inodoAux;
	for(i=0;i<superbloqueAux->numItems;i++){
		if(i == fileDescriptor && openFileDescriptors[i][0] != 0){
			int idBlock = openFileDescriptors[i][0];
			int current = openFileDescriptors[i][1];
			inodoAux = (struct inodoFichero *) &discoBloques[1][idBlock*SIZE_INODE];
			int fileSize = inodoAux->tamanyo;
			int remainBytes = fileSize-current;
			if(numBytes>remainBytes){
				memcpy(buffer, &discoBloques[idBlock][current], remainBytes);
				openFileDescriptors[i][1]=fileSize;
				return remainBytes;
			} else if(remainBytes==0){
				return 0;
			} else {
				memcpy(buffer, &discoBloques[idBlock][current], numBytes);
				openFileDescriptors[i][1]=current+numBytes;
				return numBytes;
			}
		}
	}
	return -1;
}

/*
 * Reads number of bytes from a buffer and writes them in a file.
 * Returns the number of bytes written, 0 in case of end of file or -1 in case
 * of error.
 */
int writeFS(int fileDescriptor, void *buffer, int numBytes) {
	if(numBytes>SIZE_BLOCK){
		return -1;
	}
	if(openFileDescriptorsIndex==0){
		return -1;
	}
	if(openFileDescriptors[fileDescriptor][0] == 0){
		return -1;
	}
	int i;
	struct inodoFichero * inodoAux;
	for(i=0;i<superbloqueAux->numItems;i++){
		if(i == fileDescriptor && openFileDescriptors[i][0] != 0){
			int idBlock = openFileDescriptors[i][0];
			int current = openFileDescriptors[i][1];
			int remainBytes = SIZE_BLOCK-current;
			if(numBytes>remainBytes){
				return -1;
			} else if (remainBytes==0){
				return 0;
			} else {
				memcpy(&discoBloques[idBlock][current], buffer, numBytes);
				openFileDescriptors[i][1]=current+numBytes;
				inodoAux = (struct inodoFichero *) &discoBloques[1][idBlock*superbloqueAux->sizeINode];
				inodoAux->tamanyo = numBytes;
				memcpy(&discoBloques[1][idBlock*superbloqueAux->sizeINode], inodoAux, superbloqueAux->sizeINode);
				int bytesWritten = bwrite("disk.dat", idBlock, discoBloques[idBlock]);
				if(bytesWritten){
					return -1;
				}
				return numBytes;
			}
		}
	}
	return -1;
}


/*
 * Repositions the pointer of a file. A greater offset than the current size, or
 * an offset lower than 0 are considered errors.
 * Returns new position or -1 in case of error.
 */
int lseekFS(int fileDescriptor, long offset, int whence) {
	if(openFileDescriptorsIndex==0){
		return -1;
	}
	if(openFileDescriptors[fileDescriptor][0] == 0){
		return -1;
	}
	int i;
	struct inodoFichero * inodoAux;
	for(i=0;i<superbloqueAux->numItems;i++){
		if(i == fileDescriptor && openFileDescriptors[i][0] != 0){
			int idBlock = openFileDescriptors[i][0];
			int current = openFileDescriptors[i][1];
			inodoAux = (struct inodoFichero *) &discoBloques[1][idBlock*SIZE_INODE];
			int fileSize = inodoAux->tamanyo;
			int movement = offset+current;
			if(whence == FS_SEEK_SET){
				if(movement>fileSize || movement<fileSize){
					return -1;
				} else {
					openFileDescriptors[i][1] = movement;
					return movement;
				}
			} else if (whence == FS_SEEK_BEGIN){
				openFileDescriptors[i][1] = 0;
				return 0;
			} else if (whence == FS_SEEK_END){
				openFileDescriptors[i][1] = fileSize;
				return fileSize;
			}

		}
	}
	return -1;
}

/**********************/
/* Version management */
/**********************/

/*
 * Tags a file with the given tag name. Returns 0 if the operation is
 * successful, 1 if the file already had that tag or -1 in case of error.
 */
int tagFS(char *fileName, char *tagName) {
/* 	- Si fileName.len>  o tagName >
		- return -1
	- Si array_etiquetas_index >= 30 -> ya no se pueden poner mas etiquetas
		- return -1
	- else:
		- ver si el file existe y localizar el numBloque
		- Si el tag ya existe:
			- quedarse con la posicion que ocuapa el tag
			- anhadir esa posicion en la posicion numBLoque - 2 en la primera celda libre
		- Si el tag no existe (fuera del for)
			- anhadir el tag en la  primera posicion libre
			- quedarse con la posicion que ocuapa el tag
			- anhadir esa posicion en la posicion numBLoque - 2 en la primera celda libre  */

	if(strlen(fileName)> FNAME_MAX+1 || strlen(tagName)> TAG_NAME_MAX+1){
		return -1;
	}
	else{
		int i;
		struct inodoFichero * nuevoInodo;
		for(i=0; i<superbloqueAux->numItems;i++){//Compruebo que el fichero existe
			nuevoInodo = (struct inodoFichero *) &discoBloques[1][i*superbloqueAux->sizeINode];
			if(strcmp(nuevoInodo->nombre, fileName)== 0){// si entra en el if, el fichero existe
				//Comprobar si el tag YA existe
				char etiquetas[30][33];
				int etiquetas_inodos[50][3];
				memcpy(&etiquetas, &discoBloques[0][superbloqueAux->sizeSuperBloque], superbloqueAux->sizeArrayEtiquetas);
				memcpy(&etiquetas_inodos,&discoBloques[0][superbloqueAux->sizeSuperBloque+superbloqueAux->sizeArrayEtiquetas], superbloqueAux->sizeArrayEtiquetas_Inodos);

				int j;
				for(j=0; j<TAG_MAX_SIZE; j++){
					if(strcmp(etiquetas[j], tagName)==0){//el tag ya existia de antes
						int k; //veamos si el fichero contiene ese tag que si existe
						for(k=0;k<3;k++){
							if(etiquetas_inodos[nuevoInodo->idBloque - superbloqueAux->numBloquePrimerBloqueDatos][k]==j){
								return 1;	//el fichero ya contiene ese tag
							}
						}
						//Si el flujo llega hasta aquí, el fichero no contiene al tag-> hay que anhadirselo
						for(k=0;k<3;k++){
							if(etiquetas_inodos[nuevoInodo->idBloque - superbloqueAux->numBloquePrimerBloqueDatos][k]==-1){
								etiquetas_inodos[nuevoInodo->idBloque - superbloqueAux->numBloquePrimerBloqueDatos][k]=j;
								memcpy(&discoBloques[0][superbloqueAux->sizeSuperBloque+superbloqueAux->sizeArrayEtiquetas], &etiquetas_inodos, superbloqueAux->sizeArrayEtiquetas_Inodos);
								return 0;//el tag existe y ha sido "linkado" al fihcero
							}
						}
						return -1;//el fichero ya tiene tres tags
					}
				}
				//el tag no existe -> no puede haber estado asociado al fichero
				int t;
				for(t=0; t<TAG_MAX_SIZE; t++){
					if(etiquetas[t][0] =='\0'){//hay espacio para una etiqueta mas -> relleno etiquetas i_nodos
						int k;
						for(k=0;k<3;k++){
							if(etiquetas_inodos[nuevoInodo->idBloque - superbloqueAux->numBloquePrimerBloqueDatos][k]==-1){ // == -1
								memcpy(etiquetas[array_etiquetas_index],tagName,strlen(tagName));
								memcpy(&discoBloques[0][superbloqueAux->sizeSuperBloque], &etiquetas, superbloqueAux->sizeArrayEtiquetas);
								etiquetas_inodos[nuevoInodo->idBloque - superbloqueAux->numBloquePrimerBloqueDatos][k]=array_etiquetas_index;
								memcpy(&discoBloques[0][superbloqueAux->sizeSuperBloque+superbloqueAux->sizeArrayEtiquetas], &etiquetas_inodos, superbloqueAux->sizeArrayEtiquetas_Inodos);
								//update del array_etiquetas_index
								int u;
								for(u=0; u<TAG_MAX_SIZE;u++){
									if(etiquetas[u][0] =='\0'){//[0] porque rellenamos de izquierda a derecha. No tendria sentido rellenar a partir de la segunda posicion por ejemplo
										array_etiquetas_index =	u; //como mucho es 30
										break;
									}
								}
								return 0; //el tag no existe pero se anhadio y "linko" al fichero
							}
						}
						//Si ha salido de este for es que ya tenia tres etiquetas
						return -1;
					}

				}// Si llega aqui, ya habia 30 etiquetas
				return -1;
			}
		}
		return -1;//el fichero no existe
	}

}

/*
 * Removes a tag from a file. Returns 0 if the operation is successful, 1 if
 * the tag wasn't associated to the file or -1 in case of error.
 */
int untagFS(char *fileName, char *tagName) {
/* 	- Si fileName.len>  o tagName >
		- return -1
	- else: -> comporbar que tagName esta de antes
		- ver si el file existe y localizar el numBloque
		- Si el file existe:
			- comprobar que dicho file esta asociado al tagName:
				- Conseguir el numero de bloque del inodo en cuestion
				- me dirijo a array_etiquetas_inodos a la posicion que marca ese numbloque
				- con un for desde 0 hasta dos compruebo si algun int de dicha array
				apunta a tagName
				- Si hay un match -> return 0
				- Si no hay ningun match -> return -1
		- Si no existe return -1
		*/
	if(strlen(fileName)> FNAME_MAX+1 || strlen(tagName)> TAG_NAME_MAX+1){ //Creo que el +1 no es necesario, pero se puede mirar
		return -1;
	}
	else{
		int i;
		struct inodoFichero * nuevoInodo;
		for(i=0; i<superbloqueAux->numItems;i++){//Compruebo que el fichero existe
			nuevoInodo = (struct inodoFichero *) &discoBloques[1][i*superbloqueAux->sizeINode];
			if(strcmp(nuevoInodo->nombre, fileName)== 0){//el fichero existe
				//comprobar si tagName existe
				//comprobamos si dicho fichero contiene a tagName
				char etiquetas[30][33];
				int etiquetas_inodos[50][3];
				memcpy(&etiquetas, &discoBloques[0][superbloqueAux->sizeSuperBloque], superbloqueAux->sizeArrayEtiquetas);
				memcpy(&etiquetas_inodos,&discoBloques[0][superbloqueAux->sizeSuperBloque+superbloqueAux->sizeArrayEtiquetas], superbloqueAux->sizeArrayEtiquetas_Inodos);

				int k;
				for(k=0; k<3;k++){//indexAux se refiere a la posicion de fila
					int indexAux = etiquetas_inodos[nuevoInodo->idBloque - superbloqueAux->numBloquePrimerBloqueDatos][k];
					if(strcmp(etiquetas[indexAux], tagName)== 0){ //Si la contiene->eliminarla
						//"deslinkar" etiqueta del fichero
						etiquetas_inodos[nuevoInodo->idBloque - superbloqueAux->numBloquePrimerBloqueDatos][k] = -1;
						memcpy(&discoBloques[0][superbloqueAux->sizeSuperBloque+superbloqueAux->sizeArrayEtiquetas], &etiquetas_inodos, superbloqueAux->sizeArrayEtiquetas_Inodos);
						//Eliminar etiqueta si es necesario y actualizar array_etiquetas_index
						//chequeamos si es nesario
						int u,t;
						for(u=0; u<superbloqueAux->maxNumItems;u++){
							for(t=0; t<3; t++){
								if(etiquetas_inodos[u][t] == indexAux){
									//aun no es necesario
									return 0;
								}
							}
						}
						//si llega aqui, es necesario
						//borrar el tag
						memset(etiquetas[indexAux], '\0', TAG_NAME_MAX);
						memcpy(&discoBloques[0][superbloqueAux->sizeSuperBloque], &etiquetas, superbloqueAux->sizeArrayEtiquetas);
						/*for(u=0; u<TAG_NAME_MAX + 1 ; u++){
							etiquetas[indexAux][u] = '\0';
						}*/
						//actualizar array_etiquetas_index
						for(u=0; u<TAG_MAX_SIZE; u++){
							if(etiquetas[indexAux][u] == '\0'){
								array_etiquetas_index = u;
								break;
							}
						}

						return 0;
					}
				}

				//Si ha llegado hasta aqui es que no lo contiene
				return 1;
			}
		}
		return -1; //porque el file no existe
	}

}

/*
 * Looks for all files tagged with the tag tagName, and stores them in the list
 * passed as a parameter. Returns the number of tagged files found or -1 in
 * case of error.
 */
int listFS(char *tagName, char **files) {
	if(strlen(tagName)>TAG_NAME_MAX){
		return -1;
	}
	int i, j, k;
	int result = -1;
	struct inodoFichero * inodoAux;
	char etiquetas[30][33];
	int etiquetas_inodos[50][3];	//Inicializamos los arrays auxiliares de etiquetas y etiquetas_inodos
	memcpy(&etiquetas, &discoBloques[0][superbloqueAux->sizeSuperBloque], superbloqueAux->sizeArrayEtiquetas);
	memcpy(&etiquetas_inodos,&discoBloques[0][superbloqueAux->sizeSuperBloque+superbloqueAux->sizeArrayEtiquetas], superbloqueAux->sizeArrayEtiquetas_Inodos);
	for(i=0;i<TAG_MAX_SIZE;i++){		// Iteramos por todas las etiquetas posibles, esté lleno o vacío el array
		if(strcmp(etiquetas[i], tagName)==0){		//Comprobamos si la etiqueta de la posición actual coincide con la buscada
			result = 0;
			for(j=0;j<superbloqueAux->numItems;j++){	//Iteramos a través de tantas posiciones como ficheros se hayan creado
				for(k=0;k<3;k++){
					if(etiquetas_inodos[j][k]==i){		//Si el índice de la etiqueta coincide con el correspondiente a alguna de las etiquetas del fichero
						int idBlock = j;		// Tomamos su idBloque e inicializamos el inodo para dicho fichero, adjuntándolo a la lista de ficheros etiquetados
						inodoAux = (struct inodoFichero *) &discoBloques[1][superbloqueAux->sizeINode*idBlock];
						strcat(*files, inodoAux->nombre);
						result++;
					}
				}
			}
		}
	}

	return result;
}
