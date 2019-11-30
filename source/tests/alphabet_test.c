#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../libs/arguments_parse.h"

#define ALPH_SIZE 256
unsigned alphabet_length, wanted_length;
unsigned max_len, min_len;
char* alphabet;
char alphabet_stack[ALPH_SIZE];
unsigned short *thf;

int main(int argc, char* argv[])
{
    if(argc != 3) {
        printf("USAGE!!!\n");
        return 1;
    }

    if((alphabet_length = get_unrolled_alphabet_size(argv[1])) == 0) {
        printf("USAGE!!\n");
        return 1;
    }
    // unsigned int test = (unsigned)pow(2,34);
    // int test2 = (unsigned)pow(2,34);
    unsigned int test = 4294967295;
    int test2 = test;
    printf("signed %d unsigned %u\n", test2, test);
    test = ~(1 << 31);
    test2 = test;
    printf("signed %d unsigned %u\n", test2, test);
    test = 1 << 31;
    test2 = test;
    printf("signed %d unsigned %u\n", test2, test);

    unsigned char uchr = 0;
    printf("unsigned %u signed %c\n", uchr, uchr);

    parse_alphabet_malloc(argv[1], &alphabet, &alphabet_length);
    printf("alphabet malloc = %s, length = %d\n", alphabet, alphabet_length);

    parse_alphabet(argv[1], alphabet_stack, &alphabet_length);
    printf("alphabet stack  = %s, length = %d\n", alphabet, alphabet_length);

    char error = parse_arg_to_unsigned(argv[2],'-',&min_len, &max_len);
    if(error>0) {
        printf("ERROR! Not a number!\n");
        return 1;
    }
    else if (error < 0) {
        printf("ERROR! Lenght of 0!\n");
        return 2;
    }

    printf("min = %u, max = %u\n", min_len, max_len);
    // thf = malloc(sizeof(unsigned short)*alphabet_length);
    // wanted_length = 6;
    // for(int i = 0; i < 513; i++)
    //     check_remainder(i);
    free(alphabet);
    return 0;
}