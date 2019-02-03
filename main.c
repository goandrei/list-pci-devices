#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/io.h>

#define MAX_BUSES 8
#define MAX_DEVICES 32
#define MAX_FUNCTIONS 8

#define COMMAND_PORT 0xCF8
#define DATA_PORT 0XCFC

#define PRIVILEGE_LEVEL 3

#define ERR -1

/*
*
Reads 32 bits from a given port
*
*/
unsigned int read_32bits(unsigned short port){
    return inl(port);
}

/*
*
Writes 32 bits from a given port
*
*/
void write_32bits(unsigned short port, unsigned int data){
    outl(data, port);
}

/*
*
Send an input to the command port to let the port know we need some data about the device on that bus
The input has the following structure:

| 31 |  30 - 24  |  23-16    |    15 - 11    |    10 -8        |   7 - 2        |  1 - 0 |
|  1 |  reserved | bus number| device number | function number | register number|   00   |
*
*/

unsigned int read(unsigned short bus, unsigned short device, unsigned short function, unsigned int register_offset){

    unsigned int input = (0x1 << 31) | 
                ((bus & 0xFF) << 16) | 
                ((device & 0x1F) << 11) |
                ((function & 0x07) << 8) |
                (register_offset & 0xFC);

    write_32bits(COMMAND_PORT, input);
    int output = read_32bits(DATA_PORT);

    return output >> (8 * (register_offset % 4));
}

bool device_has_functions(unsigned short bus, unsigned short device){
    return read(bus, device, 0, 0x0E) & (1 << 7); 
}

void show_description(unsigned short bus, unsigned short device, unsigned short function){

    unsigned short result16bits;
    unsigned char result8bits;

    //VENDOR_ID
    result16bits = read(bus, device, function, 0x00);

    //no vendor has such an id so it`s useless to continue
    if(result16bits == 0x0000 || result16bits == 0xFFFF){
        return;
    }

    printf("BUS %3x DEVICE %2x FUNCTION %x ", bus, device, function);
    printf("VENDOR_ID = %04x\n", result16bits);

    //DEVICE_ID
    result16bits = read(bus, device, function, 0x02);
    printf("DEVICE_ID = %04x\n", result16bits);

    //CLASS_ID
    result8bits = read(bus, device, function, 0x0B);
    printf("CLASS_ID = 0x%04x\n", result8bits);

    //SUBCLASS_ID
    result8bits = read(bus, device, function, 0x0A);
    printf("SUBCLASS_ID = 0x%04x\n", result8bits);

    //INTERFACE_ID
    result8bits = read(bus, device, function, 0x09);
    printf("INTERFACE_ID = 0x%04x\n", result8bits);

    //REVISION_ID
    result8bits = read(bus, device, function, 0x08);
    printf("PREVISION_ID = %02x\n", result8bits);

    printf("\n");
}

void list_pci(){

    unsigned char bus, device, function;

    for(bus = 0;bus < MAX_BUSES; ++bus){
        for(device = 0;device < MAX_DEVICES; ++device){
            short FUNCTIONS = device_has_functions(bus, device) ? MAX_FUNCTIONS : 1;
            for(function = 0;function < FUNCTIONS; ++function){
                show_description(bus, device, function);
            }
        }
    }
}

unsigned char increase_privileges(unsigned char level)
{
    if (iopl(level)) {
        perror("Error(try using sudo) ");
        return ERR;
    }

    return EXIT_SUCCESS;
}


int main(){

    if(increase_privileges(PRIVILEGE_LEVEL) == ERR){
        return -1;
    }
    list_pci();
    
    return 0;
}