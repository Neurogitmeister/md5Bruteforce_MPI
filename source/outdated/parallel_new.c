#include <string.h>
#include <openssl/md5.h>
#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFF_SIZE 50 // Максимальная длина обрабатываемой строки

int alphabet_length, wanted_length;
char* alphabet;
int thf[BUFF_SIZE]; // Индексный массив
unsigned char md5_input[MD5_DIGEST_LENGTH]; // Хеш искомой функции
int count_perm = 0;

int lb, ub, chunk;
int sync = 25;
int key_found_loc = 0;
int key_found_reduce = 0;
int limit_perm = 0;
int commsize, rank;

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

int get_perm_per_proc(int commsize)
{
    int chunk = alphabet_length / commsize;
    int perms = 0;
    for(int l = 1; l <= wanted_length; l++)
    {  
        perms += pow(alphabet_length, l-1)*chunk;
    }
    return perms;
}

int repeat_permutations(int curr_len) {
    
    if (curr_len == wanted_length) {
        char current_line[BUFF_SIZE];
        unsigned char cur_key[MD5_DIGEST_LENGTH];
        //Заполнение текущей строки
        int i;
        for (i = 0; i < wanted_length; i++) {
            current_line[i] = alphabet[thf[i]];
        }
        current_line[i] = '\0';

        MD5((const unsigned char *) current_line,
            strlen(current_line),
            (cur_key)
        );

        count_perm++; //счетчик перебранных вариантов

        if (!(count_perm % sync) && (limit_perm > count_perm - 2) ){
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Allreduce(&key_found_loc, &key_found_reduce, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
            //printf("rank = %d, perm = %d, key_found_reduce = %d, key_found_loc = %d\n", rank, count_perm, key_found_reduce, key_found_loc);
            if (key_found_reduce) return 1;
        }

        // сравнение
        if (compare_hash(cur_key) == 0) {
            print(cur_key);
            printf(" <= Password is \"%s\", rank = %d, perm = %d\n\n", current_line, rank, count_perm);
            key_found_loc = 1;
        }

    } else {
        // Making a tree cut on the last branching
        if(curr_len + 1 == wanted_length){
            for (int k = lb; k < ub; k++) {
                thf[curr_len] = k;
                // Уходим в рекурсию для 1 элемента
                if (repeat_permutations(curr_len + 1)) return 1;
            }
        } else {
            for(int j = 0; j < alphabet_length; j++)
            {
                // Генерируем последовательность до вышины дерева WL - 1
                thf[curr_len] = j;
                // Уходим в рекурсию для 1 элемента
                if (repeat_permutations(curr_len + 1)) return 1;
            }
        }
       
    }
    return 0;
}

int main(int argc, char **args) {
    if (argc != 4) {
        printf("Format: \"Alphabet\" \"Wanted\" \"Lenght\"\n");
        return EXIT_FAILURE;
    }
    MPI_Init(NULL,NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);
    if(!rank){

        alphabet_length = strlen(args[1]); // Длина алфавита
        alphabet = (char*)malloc(sizeof(char) * ( alphabet_length + 1 ));
        sprintf(alphabet, "%s", args[1]);
        md5str_to_md5(md5_input, args[2]); // Нахождение 128 битного хэша по входной 256 битной строке

        wanted_length = atoi(args[3]); // Длина искомой строки

        for(int i = 1; i < commsize; i++)
        {
            MPI_Send(&alphabet_length, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(alphabet, alphabet_length, MPI_CHAR, i, 0, MPI_COMM_WORLD);
            MPI_Send(&md5_input, MD5_DIGEST_LENGTH , MPI_CHAR, i, 0, MPI_COMM_WORLD);
            MPI_Send(&wanted_length, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(&alphabet_length, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //printf("\nAlph len = %d", alphabet_length);
        alphabet = (char*)malloc(sizeof(char)* (alphabet_length + 1));
        MPI_Recv(alphabet, alphabet_length, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&md5_input, MD5_DIGEST_LENGTH, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&wanted_length, 1, MPI_INT, 0, 0 , MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    chunk = alphabet_length / commsize;
    
    if(chunk == 0)
    {
        // Процессы, с рангом больше мощности алфавита простаивают...
        if(alphabet_length > rank){
            lb = rank;
            ub = rank + 1;
        } else {
            ub = lb = 0;
        }
    } else {
        /* Пространство итераций распределено равномерно между commsize - 1 потоками, 
        *  последнему плюсуется остаток от деления на commsize */
        chunk = alphabet_length / commsize;
        lb = rank * chunk;
        ub = (rank != commsize - 1) ? (rank + 1) * chunk : alphabet_length;
    }
    
    limit_perm = get_perm_per_proc(commsize);

    if(!rank) {
        printf("Alphabet: %s\nWanted(md5): ", alphabet);
        print(md5_input);
        printf("\n");
        printf("Permutations expected: %d\n", (int)pow(alphabet_length, wanted_length));
        printf("Permutations per proccess: %d\n", limit_perm);
        printf("Sync each %d permutations\n\n", sync);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    printf("chunk = %d, rank = %d, lb = %d, ub = %d\n", ub - lb, rank, lb, ub);
    MPI_Barrier(MPI_COMM_WORLD);
    if(!rank)
        printf("\n");
    MPI_Barrier(MPI_COMM_WORLD);
    // Вызываем функцию перебора,
    // увеличивая длину искомой строки на 1
    int max_wanted_length = wanted_length;
    wanted_length = 1;
    do {
        repeat_permutations(0);
    } while (!key_found_reduce && max_wanted_length > wanted_length++);

    free(alphabet);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Allreduce(&key_found_loc, &key_found_reduce, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
    //printf("rank = %d, perm = %d, key_found_reduce = %d, key_found_loc = %d\n", rank, count_perm, key_found_reduce, key_found_loc);
    if (!key_found_reduce && !rank) printf("Password not found ._.\n\n");
    MPI_Barrier(MPI_COMM_WORLD);
    printf("rank = %d, Permutations: %d\n", rank, count_perm);
    MPI_Finalize();
    return 0;
}