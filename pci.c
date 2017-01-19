#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>

#define PRINT_ERROR \
	do { \
		fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
		__LINE__, __FILE__, errno, strerror(errno)); exit(1); \
	} while(0)

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

int main(int argc, char **argv) {
	int fd;
	void *map_base, *virt_addr;
	unsigned long read_result, writeval;
	char *filename;
	off_t target;
	int access_type = 'w';

	if(argc < 3) {
		
		fprintf(stderr, "\nUsage:\t%s { sys file } { offset } [ type [ data ] ]\n"
			"\tsys file: sysfs file for the pci resource to act on\n"
			"\toffset  : offset into pci memory region to act upon\n"
			"\ttype    : access operation type : [b]yte, [h]alfword, [w]ord\n"
			"\tdata    : data to be written\n\n",
			argv[0]);
		exit(1);
	}
	filename = argv[1];
	target = strtoul(argv[2], 0, 0);

	if(argc > 3)
		access_type = tolower(argv[3][0]);

    if((fd = open(filename, O_RDWR | O_SYNC)) == -1) PRINT_ERROR;
    printf("%s opened.\n", filename);
    printf("Target offset is 0x%x, page size is %d\n", target, sysconf(_SC_PAGE_SIZE));
    fflush(stdout);

    /* Map one page */
    printf("mmap(%d, %d, 0x%x, 0x%x, %d, 0x%x)\n", 0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target);
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) PRINT_ERROR;
    printf("PCI Memory mapped to address 0x%08x.\n", map_base);
    fflush(stdout);

    virt_addr = map_base + (target & MAP_MASK);
    switch(access_type) {
		case 'b':
			read_result = *((unsigned char *) virt_addr);
			break;
		case 'h':
			read_result = *((unsigned short *) virt_addr);
			break;
		case 'w':
			read_result = *((unsigned long *) virt_addr);
			break;
		default:
			fprintf(stderr, "Illegal data type '%c'.\n", access_type);
			exit(2);
	}
    printf("Value at offset 0x%X (%p): 0x%X\n", target, virt_addr, read_result);
    fflush(stdout);

	if(argc > 4) {
		writeval = strtoul(argv[4], 0, 0);
		switch(access_type) {
			case 'b':
				*((unsigned char *) virt_addr) = writeval;
				read_result = *((unsigned char *) virt_addr);
				break;
			case 'h':
				*((unsigned short *) virt_addr) = writeval;
				read_result = *((unsigned short *) virt_addr);
				break;
			case 'w':
				*((unsigned long *) virt_addr) = writeval;
				read_result = *((unsigned long *) virt_addr);
				break;
		}
		printf("Written 0x%X; readback 0x%X\n", writeval, read_result);
		fflush(stdout);
	}

	if(munmap(map_base, MAP_SIZE) == -1) PRINT_ERROR;
    close(fd);
    return 0;
}
// ./a.out /sys/bus/pci/devices/0000\:00\:1c.4/0000\:09\:00.0/resource0 0x100 w 0x00
// argv[0]      			 [1]                                         [2] [3] [4]