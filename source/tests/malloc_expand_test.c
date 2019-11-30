#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../libs/expand_cstring_array.h"

#define MAX_COLLISIONS 5
#define MAX_TEST 25

char** collisions = NULL;
unsigned char collision_counter = 0;// Счетчик найденных коллизий
unsigned max_wanted_length = 5;

int main(int argc, char* argv[])
{

    collisions = malloc(sizeof(char*) * MAX_COLLISIONS);
    for(int i = 0; i < MAX_COLLISIONS; i++)
        collisions[i] = (char*)malloc(sizeof(char) * (max_wanted_length + 1));
    
    for(int i = 0; i < MAX_COLLISIONS; i++)
    {
        strcpy(collisions[collision_counter++], "aaaaa");
    }
    printf("Filling collisions array :\n\n");
    for(int i = 0; i < MAX_COLLISIONS; i++)
    {
        printf("%s\n", collisions[i]);
    }
    char str[max_wanted_length + 1];
    strcpy(str, "a aaa");
    printf("\nExpanding array %d times by %d words : \n\n",MAX_TEST / MAX_COLLISIONS, MAX_COLLISIONS);
    for(collision_counter = 0; collision_counter < MAX_TEST;)
    {
        if( (collision_counter) && ( (collision_counter) % MAX_COLLISIONS) == 0 ) {
            expand_cstring_array(&collisions, max_wanted_length, collision_counter, collision_counter + MAX_COLLISIONS);
            
            strcpy(collisions[collision_counter], str);
            printf("added new to realloc'd = %s\n\n", collisions[collision_counter]);
            collision_counter++;
        } else {
            strcpy(collisions[collision_counter], str);
            printf("added new = %s %d\n", collisions[collision_counter], collision_counter);
            collision_counter++;
        }
        str[0] += 1;
    }
    printf("\n");
    for(int i = 0; i < collision_counter; i++)
    {
        printf("%s\n", collisions[i]);
    }

    return 0;
}

        