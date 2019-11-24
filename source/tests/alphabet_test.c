#include "stdio.h"
#include "string.h"
#include "stdlib.h"

int alphabet_length, wanted_length;
char* alphabet;
unsigned short *thf;

short int parse_alphabet(char* str)
{
    int len = strlen(str);

    alphabet_length = 0;
    for(int i = 0; i < len; i++)
    {
        if( str[i] == '-' && i && i - len + 1)
        {
            if(str[i+2] == ',' || str[i+2] == '\0' ) {
                int temp = str[i+1] - str[i-1];
                if (temp < 0) temp *= -1;
                alphabet_length += temp;
                i+=2;
            }
            else 
                return 1;
        } else {
            alphabet_length++;
        }
    }

    alphabet = (char*)malloc(sizeof(char)* (alphabet_length + 1));

    int counter = 0;
    for(int i = 0; i < len; i++)
    {
        if( str[i] == '-' && i && i - len + 1)
        {
            if(str[i+2] == ',' || str[i+2] == '\0' ) {
                // a-z,A-Z
                char first = str[i-1];
                char last = str[i+1];
                while(first < last) {
                    alphabet[counter++] = ++first;
                }
                while(first > last) {
                    alphabet[counter++] = --first;
                }
                i+=2;
            }
        } else {
            alphabet[counter++] = str[i];
        }
    }

    return 0;
}

void check_remainder(int rank){

    char* word = malloc(sizeof(char)*(wanted_length + 1));
    word[wanted_length] = '\0';

    unsigned set = rank;
    unsigned i = wanted_length - 1;
    do
    {
        word[i] = alphabet[alphabet_length - 1 - set % alphabet_length];
        set /= alphabet_length;
        i--;
    } while(set > 0);

    i++;
    for(int j = 0; j < i; j++) {
        word[j] = alphabet[alphabet_length - 1];
    }

    
    printf("%s\n",word);
    
    return;
}

int main(int argc, char* argv[])
{
    if(argc != 2) {
        printf("USAGE!!!\n");
        return 1;
    }

    if(parse_alphabet(argv[1])) {
        printf("USAGE!!\n");
        return 1;
    } 
    printf("alphabet = %s, length = %d\n", alphabet, alphabet_length);

    thf = malloc(sizeof(unsigned short)*alphabet_length);
    wanted_length = 6;
    for(int i = 0; i < 513; i++)
        check_remainder(i);
    free(alphabet);
    return 0;
}