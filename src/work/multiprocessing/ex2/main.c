#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BLOCK_SIZE (1 << 20)  // 1 MiB
#define NUM_BLOCKS 10

int main()
{
    char* blocks[NUM_BLOCKS];
    size_t i;

    for (i = 0; i < NUM_BLOCKS; i++) {
        blocks[i] = (char*)malloc(BLOCK_SIZE);
        if (blocks[i] == NULL) {
            printf("Error: malloc\n");
            break;
        }

        printf("Allocated block %zu\n", i);
        memset(blocks[i], 0, BLOCK_SIZE);
    }

    for (size_t j = 0; j < i; j++) {
        free(blocks[j]);
    }

    return 0;
}
