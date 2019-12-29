#include <string.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFF_SIZE 50 // Максимальная длина обрабатываемой строки

int main(int argc, char* argv[])
{
    char str[256] = {0};
    if (argc == 2)
        strncpy(str, (strlen(argv[1]) > 255) ? 255 : strlen(argv[1]) + 1, argv[1]);
    else{
        printf("Usage: %s <word>(char[])\n", argv[0]);
        return 1;
    }
    unsigned char hash[MD5_DIGEST_LENGTH+1];
    hash[16] = 0;
   

    MD5((unsigned char*)str, strlen(str), hash);

    char hash32[MD5_DIGEST_LENGTH * 2 + 1];
    printf("STRING=%s\n", str);
    for (int i = 0, j = 0; i < 16; i++, j+=2)
    {
        sprintf(hash32+j, "%02x", hash[i]);
    }
    hash32[32] = '\0';
    printf ("MD5_32 = %s\n", hash32);

    return 0;
}