#include "./rom.h"

rom *rom_load(const char *path) {
    FILE *f = fopen(path, "r+b");
    if (f == NULL) return NULL;
    rom *r = (rom *)malloc(sizeof(rom));
    if(r == NULL) return NULL;

    fseek(f, 0, SEEK_END);
    r->size = ftell(f);
    fseek(f, 0, SEEK_SET);
    r->bytes = malloc(r->size);
    if (r->bytes == NULL) return NULL;

    if (fread(r->bytes, 1, r->size, f) != r->size) {
	    // Handle read error or incomplete read
	    fclose(f);
	    free(r->bytes);
	    free(r);
	    return NULL;
    }

    fclose(f);
    return r;
}

void rom_unload(rom *r) {
    free(r->bytes);
    free(r);
}

void rom_dump(rom *r) {
    for(size_t i = 0; i < r->size; i++) {
        // Print address at the start of each line
        if(i % 16 == 0) {
            if(i != 0) {
                printf(" |");
                for(size_t j = i - 16; j < i; j++) {
                    // Check if the byte is printable
                    if(r->bytes[j] >= 0x20 && r->bytes[j] <= 0x7E) {
                        printf("%c", r->bytes[j]);
                    } else {
                        printf(".");
                    }
                }
                printf("|\n");
            }
            printf("%04zx: ", i);
        }
        
        // Print hex value
        printf("%02x ", r->bytes[i]);
        
        // Add extra spacing in the middle of the hex dump
        if(i % 16 == 7) {
            printf(" ");
        }
    }
    
    // Print the final ASCII representation for the last line if it's not complete
    size_t remaining = r->size % 16;
    if(remaining) {
        // Padding for incomplete line
        for(size_t i = 0; i < 16 - remaining; i++) {
            printf("   ");
            if(i + remaining == 7) printf(" "); // Extra space in the middle
        }
        
        printf(" |");
        for(size_t i = r->size - remaining; i < r->size; i++) {
            if(r->bytes[i] >= 0x20 && r->bytes[i] <= 0x7E) {
                printf("%c", r->bytes[i]);
            } else {
                printf(".");
            }
        }
        printf("|\n");
    }
}
