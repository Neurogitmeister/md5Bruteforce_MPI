#include <stdio.h>
#include <openssl/md5.h>
#include <string.h>

// Перевод посимвольно хэш-строки 32х8 бит в хэш-код 32х4 бит
void md5str_to_md5(unsigned char dest_md5[MD5_DIGEST_LENGTH], char* src_str)
{
    if (strlen(src_str) > 31)
        for(int i = 0, j = 0; i < MD5_DIGEST_LENGTH; i++, j+=2)
        {
            if (src_str[j] >= '0' && src_str[j] <= '9')
                dest_md5[i] = (src_str[j] - 48) << 4;
            else 
            if (src_str[j] >= 'a' && src_str[j] <= 'f')
                dest_md5[i] = (src_str[j] - 87) << 4;

            if (src_str[j+1] >= '0' && src_str[j+1] <= '9')
                dest_md5[i] += ((src_str[j+1] - 48) << 4) >> 4;
            else 
            if (src_str[j+1] >= 'a' && src_str[j+1] <= 'f')
                dest_md5[i] += ((src_str[j+1] - 87) << 4) >> 4;
        }
}

// Вывод хеша
void md5_print(unsigned char *str) {
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        printf("%02x", str[i]);
}

// Сравнение исходного хэша и хэша из аргумента compare_hash
int compare_hash(unsigned char* md5_input, unsigned char* md5_reference) {
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        if (md5_input[i] != md5_reference[i]) return -1;
    return 0;
}
