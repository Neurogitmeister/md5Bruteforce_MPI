#include <string.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFF_SIZE 50 // Максимальная длина обрабатываемой строки

int alphabet_length;
char* alphabet;
int thf[BUFF_SIZE]; // Индексный массив
unsigned char md5_input[MD5_DIGEST_LENGTH]; // Хеш искомой функции
static int count_perm = 0;

// Перевод посимвольно хэш-строки 32х8 бит в хэш-код 32х4 бит
void md5str_to_md5(unsigned char dest_md5[], char* src_str)
{
    if(strlen(src_str) > 31)
        for(int i = 0, j = 0; i < MD5_DIGEST_LENGTH; i++, j+=2)
        {
            if(src_str[j] >= '0' && src_str[j] <= '9')
                dest_md5[i] = (src_str[j] - 48) << 4;
            else 
            if(src_str[j] >= 'a' && src_str[j] <= 'f')
                dest_md5[i] = (src_str[j] - 87) << 4;

            if(src_str[j+1] >= '0' && src_str[j+1] <= '9')
                dest_md5[i] += ((src_str[j+1] - 48) << 4) >> 4;
            else 
            if(src_str[j+1] >= 'a' && src_str[j+1] <= 'f')
                dest_md5[i] += ((src_str[j+1] - 87) << 4) >> 4;
        }
}

// Вывод хеша
void print(unsigned char *str) {
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        printf("%02x", str[i]);
}

// Сравниваем хеши. Нашли или нет
int compare_hash(const unsigned char *a) {
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        if (a[i] != md5_input[i]) return -1;
    return 0;
}

int recursive_permutations(int k, int wanted_len) {
    if (k == wanted_len) {

        
        char current_line[BUFF_SIZE];
        unsigned char cur_key[MD5_DIGEST_LENGTH];
        //Заполнение текущей строки
        int i;
        for (i = 0; i < wanted_len; i++) {
            current_line[i] = alphabet[thf[i]];
        }
        current_line[i] = '\0';

        MD5((const unsigned char *) current_line,
            strlen(current_line),
            (cur_key)
        );

        count_perm++; //счетчик перебранных вариантов

        /*
            printf("%s|%d ", current_line, count_perm);
            printf("[%d] \t(%s)\t", (int)strlen(current_line), current_line);
            print(cur_key);printf(" ");
            printf("\n");
        */

        // сравнение
        if (compare_hash(cur_key) == 0) {
            print(cur_key);
            printf(" <= Password is %s\n", current_line);
            return 1;
        }

    } else {
        for (int j = 0; j < alphabet_length; j++) {
            
            // Генерируем последовательность
            thf[k] = j;
            // Вызываем перестановки для текущей буквы
            if (recursive_permutations(k + 1, wanted_len)) return 1;
        }
    }
    return 0;
}

int main(int argc, char **args) {
    if (argc != 4) {
        printf("Format: \"Alphabet\" \"Wanted\" \"Lenght\"\n");
        return EXIT_FAILURE;
    }

    alphabet = args[1];
    alphabet_length = strlen(alphabet); // Длина алфавита

    md5str_to_md5(md5_input, args[2]); // Нахождение 128 битного хэша по входной 256 битной строке

    int wanted_length = atoi(args[3]); // Длина искомой строки

    printf("Alphabet: %s\nWanted(md5): ", alphabet);
    print(md5_input);
    printf("\n\n");

    int found = 0;
    int curr_length = 1;
    do {
        found = recursive_permutations(0, curr_length);
    } while (!found && wanted_length > curr_length++);

    if (!found) printf("Password not found ._.\n");
    printf("Permutations: %d\n", count_perm);
    return 0;
}