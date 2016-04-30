/* This file contains the definition of the functions that must be used in
 * order to read or read to and from the device.
 */

#include "ufs.h"

/****************/
/* Disk access. */
/****************/

/*
 * Reads a block from the device and stores it in a buffer.
 * Returns the number of bytes read or -1 in case of error, including short
 * read.
 */
int bread(char *deviceName, int blockNumber, char *buffer) {
	int fd = open(deviceName, O_RDONLY);
	
	if(fd < 0){
		/* fprintf(stderr, "ERROR: UNABLE TO OPEN DISK FILE %s \n", deviceName); */
		return -1;
	}

	int len = lseek(fd, 0, SEEK_END) + 1;
	if((4096*blockNumber+4096) > len) {
		close(fd);
		return -1;
	}

	lseek(fd, 4096*blockNumber, SEEK_SET);
	
	int total_read, read_result;

	total_read = 0;
	do{
		read_result = read(fd, buffer+total_read, 4096-total_read);
		total_read = total_read + read_result;
	} while(total_read < 4096 && read_result >= 0);

	close(fd);
	
	return 0;
}

/*
 * Writes a block from a buffer to the device.
 * Returns the number of bytes written or -1 in case of error.
 */
int bwrite(char *deviceName, int blockNumber, char*buffer) {
	int fd = open(deviceName, O_WRONLY);
	
	if(fd < 0){
		 fprintf(stderr, "ERROR: UNABLE TO OPEN DISK FILE %s \n", deviceName); 
		return -1;
	}

	int len = lseek(fd, 0, SEEK_END) + 1;
	if((4096*blockNumber+4096) > len) {
		close(fd);
		return -1;
	}

	lseek(fd, 4096*blockNumber, SEEK_SET);
	
	int total_write, write_result;

	total_write = 0;
	do{
		write_result = write(fd, buffer+total_write, 4096-total_write);
		total_write = total_write + write_result;
	} while(total_write < 4096 && write_result >= 0);

	close(fd);
	
	return 0;
}
