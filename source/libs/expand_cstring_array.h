#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int expand_cstring_array(char*** array, unsigned words_length, unsigned size_old, unsigned size_new){
    char** temp = NULL; 
    temp = malloc(sizeof(char*) * size_old);
    for(int i = 0; i < size_old; i++) {
        temp[i] = (char*)malloc(sizeof(char) * (words_length + 1));
        strcpy(temp[i], *(*array + i));
        free(*(*array + i));
    }

    free(*array);
    if ( (*array = malloc(sizeof(char*) * (size_new)) ) == NULL) {
        return 1;
    }

    for(int i = 0; i < size_new; i++) {
        *(*array + i) = (char*)malloc(sizeof(char) * (words_length + 1));
        if (i < size_old) {
            strcpy(*(*array + i), temp[i]);
            free(temp[i]);
        }
    }
    free(temp);

    return 0;
}

int expand_cstring_array_debug(char*** array, unsigned words_length, unsigned size_old, unsigned size_new){
    char** temp = NULL; 
    temp = malloc(sizeof(char*) * size_old);
    for(int i = 0; i < size_old; i++) {
        temp[i] = (char*)malloc(sizeof(char) * (words_length + 1));
        strcpy(temp[i], *(*array + i));
        free(*(*array + i));
    }
    printf("\nallocated and freed + copied %d\n", size_old);

   free(*array);
    if ( (*array = malloc(sizeof(char*) * (size_new)) ) == NULL) {
        return 1;
    }

   for(int i = 0; i < size_new; i++) {
        *(*array + i) = (char*)malloc(sizeof(char) * (words_length + 1));
    } 

    printf("realloc'd to %d\n", size_new);

     for(int i = 0; i < size_old; i++) {

        strcpy(*(*array + i), temp[i]);
        free(temp[i]);
    }
    free(temp);
    printf("free'd temp\n");
    return 0;
}