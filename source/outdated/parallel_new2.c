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
int perm_running = 0;
int perm_count = 1;

int lb, ub, chunk, chunk_resize;
int key_found_loc = 0;
int key_found_reduce = 0;
int sync = 25;
//int branches_left = 0;
//int depth_curr = 1;
//int *depth_optimal, *min_remain, *min_mult;
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

        perm_running++; //счетчик перебранных вариантов

        if(perm_running < chunk)
        {
            if (!(perm_running % sync)){
                MPI_Barrier(MPI_COMM_WORLD);
                MPI_Allreduce(&key_found_loc, &key_found_reduce, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
                //printf("rank = %d, perm = %d, key_found_reduce = %d, key_found_loc = %d\n", rank, perm_running, key_found_reduce, key_found_loc);
                if (key_found_reduce) return 1;
            }

            // сравнение
            if (compare_hash(cur_key) == 0) {
                print(cur_key);
                printf(" <= Password is \"%s\", rank = %d, perm = %d\n\n", current_line, rank, perm_running);
                key_found_loc = 1;
            }
        } else return 1;

    } else {
        
        
        if(curr_len == wanted_length){
        // Никогда не переходит, если curr_len == wanted_len   
            
            /*
            if(min_remain[wanted_length - 1] != 0)
            {
                int i = 1; // rank compensation
                while(depth_optimal[wanted_length]--){
                    branches_left -= alphabet_length - 1;
                    if(branches_left > 0) {
                        if (rank < commsize - branches_left + 1)
                        {
                            branches_left = 0;
                            thf[curr_len] = (rank + i) % (alphabet_length - 1);
                            if (repeat_permutations(curr_len + 1)) return 1;
                        } else
                            thf[curr_len] = alphabet_length - 1;
                    } else {
                        if (rank < commsize + branches_left)
                        {
                            branches_left = 0;
                            thf[curr_len] = (rank + i) % (alphabet_length - 1);
                            if (repeat_permutations(curr_len + 1)) return 1;
                        } else {
                            for (int k = lb; k < ub; k++) {
                            thf[curr_len] = k;
                            // Уходим в рекурсию для 1 элемента
                            if (repeat_permutations(curr_len + 1)) return 1;
                            }
                        }
                    }
                    curr_len++;
                    i++;
                }
            } else {

                if(curr_len + 1 == wanted_length){
                    for(int j = lb; j < ub; j++)
                    {
                        thf[curr_len] = j;
                        if (repeat_permutations(curr_len + 1)) return 1; 
                    }
                } else {

                    depth_optimal[wanted_length - 1]--;
                    for(int j = 0; j < alphabet_length; j++)
                    {
                        thf[curr_len] = j;
                        if (repeat_permutations(curr_len + 1)) return 1;               
                    }
                }
            }
            */
        } else {
            int i;
            if(commsize > chunk){
                if(rank < chunk) 
                    for(i = 0; i < alphabet_length; i++) {
                        thf[curr_len] = i;
                        if (repeat_permutations(curr_len + 1)) return 1;
                    }
                else 
                    return 0;
            }
            else {
                chunk_resize = chunk;
                for(i = 0; i < wanted_length - curr_len - 1)
                    chunk_resize /= alphabet_length;

                lb = rank * chunk_resize;
                ub = lb + chunk_resize;
                lb += lb % alphabet_length;
                for(i = lb; i < ub; i++)
                {
                    thf[curr_len] = i;
                    if (repeat_permutations(curr_len + 1)) return 1;
                }
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

   

    int max_wanted_length = wanted_length;
   
   /* 
    min_remain = malloc(sizeof(int) * max_wanted_length);
    min_mult = malloc(sizeof(int) * max_wanted_length);
    depth_optimal = malloc(sizeof(int) * max_wanted_length);

    commsize = 64;

    for(int i = 0; i < max_wanted_length; i++)
    {
        min_remain[i] = 404;
        min_mult[i] = 404;
        depth_optimal[i] = 1;
    }

    wanted_length = 1;
    while (wanted_length <= max_wanted_length){
        int perms_curr;
        int remainder = alphabet_length;
        perms_curr = pow(alphabet_length, wanted_length);
        if(commsize > perms_curr){

            min_remain[wanted_length - 1] = -1;
            depth_optimal[wanted_length - 1] = 1;
            min_mult[wanted_length - 1] = -1;
            
            wanted_length++;
            continue;
        }

        for(int j = 1; j <= (alphabet_length + 1) / 2; j++)
            for(int WL = 1; WL < max_wanted_length; WL++) {
            int perms_loop = pow(alphabet_length, WL);
            remainder = (perms_curr * j) % commsize + (perms_loop * (alphabet_length - j)) % commsize ;
            if(remainder <= min_remain[wanted_length - 1]){
                min_remain[wanted_length - 1] = remainder;
                min_mult[wanted_length - 1] = j;
                depth_optimal[wanted_length - 1] = wanted_length;
                depth_optimal_left[wanted_length - 1] = WL;
            }
            if(remainder == 0){
                for(int k = wanted_length; k < max_wanted_length; k++){
                    min_remain[k] = remainder;
                    min_mult[k] = j;
                    depth_optimal[k] = wanted_length;
                    depth_optimal_left[k] = WL;
                }
                break;
            }
        }
        if(remainder == 0) break;
        wanted_length++;
    }
    */

    chunk = pow(alphabet_length, wanted_length) / commsize;

    if(!rank) {
        printf("Alphabet: %s, length: %d\n", alphabet, alphabet_length);
        printf("Wanted(md5): ");
        print(md5_input);
        printf("\n");
        printf("Permutations expected: %d\n", (int)pow(alphabet_length, max_wanted_length));
        printf("Max chunk: %d\n", chunk);
        printf("Sync each %d permutations\n\n", sync);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    printf("chunk = %d, rank = %d, lb = %d, ub = %d\n", ub - lb, rank, lb, ub);
    MPI_Barrier(MPI_COMM_WORLD);
    if(!rank)
        printf("\n");
    MPI_Barrier(MPI_COMM_WORLD);

    if(!rank)
    {
        printf("Optimal:\n");
        for(int i = 0; i < max_wanted_length; i++)
        {
            printf("%d : min_remain=%d; min_mult=%d, depth_optimal=%d\n", i, min_remain[i], min_mult[i], depth_optimal[i]);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // Вызываем функцию перебора,
    // увеличивая длину искомой строки на 1
    wanted_length = 1;
    //depth_curr = 1;
    //branches_left = commsize;

    do {
        int perms_curr = pow(alphabet_length, wanted_length);
        if(commsize > perms_curr) {
            if(rank > perms_curr)
                continue;
        }
        chunk = perms_curr / commsize;
        repeat_permutations(0);
    } while (!key_found_reduce && max_wanted_length > wanted_length++);

    free(alphabet);
    
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Allreduce(&key_found_loc, &key_found_reduce, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
    //printf("rank = %d, perm = %d, key_found_reduce = %d, key_found_loc = %d\n", rank, perm_running, key_found_reduce, key_found_loc);
    if (!key_found_reduce && !rank) printf("Password not found ._.\n\n");
    MPI_Barrier(MPI_COMM_WORLD);
    printf("rank = %d, Permutations: %d\n", rank, perm_running);
    MPI_Finalize();
    return 0;
}