#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/filesystem.h"

#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"

int main() {
	int ret, i, fd;
	char buffer1[BLOCK_SIZE];
	char buffer2[BLOCK_SIZE];
	char buffer3[BLOCK_SIZE];

	fprintf(stdout, "%s", "TEST mkFS\n");

	ret = mkFS(50, 327680); //204800   327680
	if(ret != 0) {
		fprintf(stdout, "%s%s%s%s", "TEST mkFS ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s", "TEST mkFS ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);
	fprintf(stdout, "%s", "TEST mountFS\n");

	ret = mountFS();
	if(ret != 0) {
		fprintf(stdout, "%s%s%s%s", "TEST mountFS ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}

	fprintf(stdout, "%s%s%s%s", "TEST mountFS ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);
	fprintf(stdout, "%s", "TEST creatFS\n");

	ret = creatFS("test.txt");
	if(ret != 0) {
		fprintf(stdout, "%s%s%s%s", "TEST creatFS ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}

	fprintf(stdout, "%s%s%s%s", "TEST creatFS ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);
	fprintf(stdout, "%s", "TEST openFS + closeFS\n");

	fd = openFS("test.txt");
	if(fd < 0) {
		fprintf(stdout, "%s%s%s%s%s", "TEST openFS + closeFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at openFS\n");
		return -1;
	}
	ret = closeFS(fd);
	if(ret != 0) {
		fprintf(stdout, "%s%s%s%s%s", "TEST openFS + closeFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at closeFS\n");
		return -1;
	}

	fprintf(stdout, "%s%s%s%s", "TEST openFS + closeFS ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);
	fprintf(stdout, "%s", "TEST openFS + readFS + writeFS + lseekFS + closeFS\n");

	fd = openFS("test.txt");
	if(fd < 0) {
		fprintf(stdout, "%s%s%s%s%s", "TEST openFS + readFS + writeFS + lseekFS + closeFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at openFS\n");
		return -1;
	}
	ret = readFS(fd, buffer1, BLOCK_SIZE);
	if(ret != 0) {
		fprintf(stdout, "%s%s%s%s%s", "TEST openFS + readFS + writeFS + lseekFS + closeFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at readFS\n");
		return -1;
	}
	memset(buffer2, 't', BLOCK_SIZE);
	ret = writeFS(fd, buffer2, BLOCK_SIZE);
	if(ret != BLOCK_SIZE) {
		fprintf(stdout, "%s%s%s%s%s", "TEST openFS + readFS + writeFS + lseekFS + closeFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at writeFS\n");
		return -1;
	}
	ret = lseekFS(fd, 0, FS_SEEK_BEGIN);	// EL ORIGINAL ES FS_SEEK_SET, PERO ESE RESULTADO DARÁ MAL SIEMPRE, SEGÚN NUESTRA IMPLEMENTACION
	if(ret != 0) {
		fprintf(stdout, "%s%s%s%s%s", "TEST openFS + readFS + writeFS + lseekFS + closeFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at lseekFS\n");
		return -1;
	}
	ret = readFS(fd, buffer3, BLOCK_SIZE);
	if(ret != BLOCK_SIZE) {
		fprintf(stdout, "%s%s%s%s%s", "TEST openFS + readFS + writeFS + lseekFS + closeFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at readFS #2\n");
		return -1;
	}
	for(i = 0; i < BLOCK_SIZE; ++i) {
		if(buffer3[i] != 't') {
			fprintf(stdout, "%s%s%s%s%s", "TEST openFS + readFS + writeFS + lseekFS + closeFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at readFS #2\n");
			return -1;
		}
	}
	ret = closeFS(fd);
	if(ret != 0) {
		fprintf(stdout, "%s%s%s%s%s", "TEST openFS + readFS + writeFS + lseekFS + closeFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at closeFS\n");
		return -1;
	}

	fprintf(stdout, "%s%s%s%s", "TEST openFS + readFS + writeFS + lseekFS + closeFS ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);
	fprintf(stdout, "%s", "TEST tagFS + listFS + untagFS\n");

	ret = tagFS("test.txt", "sample");
	if(ret != 0) {
		fprintf(stdout, "%s%s%s%s%s", "TEST tagFS + listFS + untagFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at tagFS\n");
		return -1;
	}
	char **files = (char**) malloc(50*sizeof(char*));
	for(i = 0; i < 50; ++i) {
		files[i] = (char*) malloc(64*sizeof(char));
	}
	ret = listFS("sample", files);
	if(ret != 1 || strcmp(files[0], "test.txt") != 0) {
		fprintf(stdout, "%s%s%s%s%s", "TEST tagFS + listFS + untagFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at listFS\n");
		return -1;
	}
	ret = untagFS("test.txt", "sample");
	if(ret != 0) {
		fprintf(stdout, "%s%s%s%s%s", "TEST tagFS + listFS + untagFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at untagFS\n");
		return -1;
	}
	ret = listFS("sample", files);
	if(ret != -1) {
		fprintf(stdout, "%s%s%s%s%s", "TEST tagFS + listFS + untagFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at listFS #2\n");
		return -1;
	}
	fprintf(stdout, "%s%s%s%s", "TEST tagFS + listFS + untagFS ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);
	fprintf(stdout, "%s", "TEST umountFS\n");


	ret = umountFS();
	if(ret != 0) {
		fprintf(stdout, "%s%s%s%s", "TEST umountFS ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}

	fprintf(stdout, "%s%s%s%s", "TEST umountFS ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	/* TESTS CREADOS POR PABLO LEON PACHECO - 100316797 Y CARMEN LOPEZ JURADO 100315115 */
	// Crea un sistema de ficheros de 400KB, lo monta, crea 2 archivos, escribe 'd' en la totalidad del primero, reposiciona el puntero al principio del fichero, lo lee,
	// escribe el contenido leído en el segundo fichero, cierra ambos ficheros y desmonta el FS.
	int fd1, fd2;
	char buffer4[BLOCK_SIZE];
	fprintf(stdout, "%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS\n");

	ret = mkFS(50, 400*1024);
	if(ret != 0){
		fprintf(stdout, "%s%s%s%s%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at mkFS\n");
		return -1;
	}
	ret = mountFS();
	if(ret != 0){
		fprintf(stdout, "%s%s%s%s%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at mountFS\n");
		return -1;
	}
	ret = creatFS("testfile1.txt");
	if(ret != 0){
		fprintf(stdout, "%s%s%s%s%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at creatFS #1\n");
		return -1;
	}
	ret = creatFS("testfile2.txt");
	if(ret != 0){
		fprintf(stdout, "%s%s%s%s%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at creatFS #2\n");
		return -1;
	}
	fd1 = openFS("testfile1.txt");
	if(fd1 != 0){
		fprintf(stdout, "%s%s%s%s%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at openFS #1\n");
		return -1;
	}
	fd2 = openFS("testfile2.txt");
	if (fd2 != 1){
		fprintf(stdout, "%s%s%s%s%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at openFS #2\n");
		return -1;
	}
	memset(buffer4, 'd', BLOCK_SIZE);
	ret = writeFS(fd1, buffer4, BLOCK_SIZE);
	if (ret != BLOCK_SIZE){
		fprintf(stdout, "%s%s%s%s%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at writeFS\n");
		return -1;
	}
	ret = lseekFS(fd1, 0, FS_SEEK_BEGIN);
	if (ret != 0){
		fprintf(stdout, "%s%s%s%s%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at lseekFS\n");
		return -1;
	}
	ret = readFS(fd1, buffer4, BLOCK_SIZE);
	if (ret != BLOCK_SIZE){
		fprintf(stdout, "%s%s%s%s%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at readFS\n");
		return -1;
	}
	ret = writeFS(fd2, buffer4, BLOCK_SIZE);
	if (ret != BLOCK_SIZE){
		fprintf(stdout, "%s%s%s%s%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at writeFS\n");
		return -1;
	}
	ret = closeFS(fd1);
	if (ret != 0){
		fprintf(stdout, "%s%s%s%s%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at closeFS #1\n");
		return -1;
	}
	ret = closeFS(fd2);
	if (ret != 0){
		fprintf(stdout, "%s%s%s%s%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at closeFS #2\n");
		return -1;
	}
	ret = umountFS();
	if (ret != 0){
		fprintf(stdout, "%s%s%s%s%s", "TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at unmountFS\n");
		return -1;
	}

	fprintf(stdout, "%s%s%s%s","TEST mkFS + mountFS + creatFS + openFS + writeFS + lseekFS + readFS + writeFS + closeFS + unmountFS ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	//Intenta crear un FS de 300KB < 320KB de mínimo, asi que "falla"
	fprintf(stdout, "%s", "TEST mkFS 300KB\n");
	ret = mkFS(50, 300*1024);
	if (ret != -1){
		fprintf(stdout, "%s%s%s%s", "TEST mkFS 300KB ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s", "TEST mkFS 300KB ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	//Intenta crear un FS de 30 ficheros maximos, siendo 50 el unico numero permitido, asi que "falla"
	fprintf(stdout, "%s", "TEST mkFS 30 files\n");
	ret = mkFS(30, 350*1024);
	if (ret != -1){
		fprintf(stdout, "%s%s%s%s", "TEST mkFS 30 files ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s", "TEST mkFS 30 files ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	//Monta un fichero e intenta crear un fichero con nombre demasiado largo, y otro normal
	fprintf(stdout, "%s", "TEST mountFS + creatFS bad + creatFS good\n");
	ret = mountFS();
	if (ret != 0){
		fprintf(stderr, "%s%s%s%s%s", "TEST mountFS + creatFS bad + creatFS good ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at mountFS\n");
		return -1;
	}
	ret = creatFS("esteficherotieneunnombredamsiadolargoyportantonodeberiasercreado_encasodesercreadodeberafallar.txt");
	if (ret != -1){
		fprintf(stderr, "%s%s%s%s%s", "TEST mountFS + creatFS bad + creatFS good ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at creatFS bad\n");
		return -1;
	}
	ret = creatFS("ficheroBueno.txt");
	if (ret != 0){
		fprintf(stderr, "%s%s%s%s%s", "TEST mountFS + creatFS bad + creatFS good ", ANSI_COLOR_RED, "FAILED ", ANSI_COLOR_RESET, "at creatFS good\n");
		return -1;
	}
	fprintf(stdout, "%s%s%s%s", "TEST mountFS + creatFS bad + creatFS good ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	//Intenta crear un fichero con un nombre ya existente y "falla"
	fprintf(stdout, "%s", "TEST creatFS repetido\n");
	ret = creatFS("ficheroBueno.txt");
	if (ret != 1){
		fprintf(stdout, "%s%s%s%s", "TEST creatFS repetido ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
	}
	fprintf(stdout, "%s%s%s%s", "TEST creatFS repetido ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	return 0;
}
