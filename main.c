#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "./board.h"

int parse_arguments(char response) {
    if(response == 'y' || response == 'Y') {
        return 1;
    } else if(response == 'n' || response == 'N') {
        return 2;
    }   
    return 0;
} 

int main(int argc, char **argv) {
    Board *b = NULL;
    if(argc < 2) {
        char response = 0;
        int result = 0;
        fprintf(stderr, "No ROM file loaded!\nSystem will proceed with the default reset.bin ROM\n");
        fprintf(stderr, "Continue? (Y/n)");
        while((result = parse_arguments(response)) == 0) {
            response = getchar();
        }
        if(result == 1) {
            b = board_init(NULL);            
        } else if (result == 2){
            return 1;
        } 
    } else {
        b = board_init(argv[1]);
    }
    
    if (b == NULL) {
        printf("failed to init board\n");
        return 2;
    }

    bool power = true;
    while (power) {
        __run(b);
        debug_print_CPU(b->c);
        system("clear");
    }

    board_shutdown(b);
    
    return 0;
}
