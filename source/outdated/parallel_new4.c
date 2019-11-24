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

int wanted_rank;

int chunk;
int key_found_loc = 0;
int key_found_reduce = 0;
int sync = 25;
int commsize, rank;

// Перевод посимвольно хэш-строки 32х8 бит в хэш-код 32х4 бит
void md5str_to_md5(unsigned char dest_md5[], char* src_str)
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

int repeat_permutations(int curr_len, int leaves_visited) {

    if (curr_len != wanted_length) {
        
        int i, lb, ub;
        float chunk_resize;
        
        chunk_resize = chunk / pow(alphabet_length,  wanted_length - curr_len -1);

        ub = (int)floor((rank + 1) * chunk_resize);
        lb = (int)floor(rank * chunk_resize);

        if (curr_len == 0) {
            
            if (ub / alphabet_length - lb / alphabet_length > 0)
                ub = alphabet_length - 1;
            else
                ub = ub % alphabet_length;

            lb = lb % alphabet_length;

        } else {

            int node_number = leaves_visited / pow(alphabet_length, wanted_length - curr_len - 1) + 1;
            printf("    len = %d, rank = %d, node_number = %d / %f + 1 = %d, lb = %d, ub = %d\n", curr_len, rank, leaves_visited, pow(alphabet_length, wanted_length - curr_len - 1), node_number, lb, ub);
       
            if (leaves_visited != 0 && lb + node_number < ub) {

                lb = 0;
                ub = alphabet_length - 1;

            } else if ( leaves_visited == 0) { // first node

                lb = lb % alphabet_length;
                if (ub - lb > chunk_resize)
                    ub = alphabet_length - 1;
                else
                    ub = ub % alphabet_length;

            } else { // last node
            printf("reached last node on rank = %d, len = %d!\n", rank, curr_len);
                lb = 0;
                ub = ub % alphabet_length;
            }

            //printf("    len = %d, rank = %d, node_number = %d / %f + 1 = %d, lb = %d, ub = %d\n", curr_len, rank, leaves_visited, pow(alphabet_length, wanted_length - curr_len - 1), node_number, lb, ub);
        }

        if (rank == commsize - 1) ub = alphabet_length - 1;

        // if (rank == 0)
        printf("new len = %d, rank = %d, chunk = %d, resize = %f, lb = %d, ub = %d\n", curr_len, rank, chunk, chunk_resize, lb, ub);
        if (curr_len + 1 != wanted_length)

            for(i = lb; i <= ub ; i++)
            {
                thf[curr_len] = i;
                leaves_visited += repeat_permutations(curr_len + 1, leaves_visited);
                if (leaves_visited == -1) return -1;
            }

        else

            for(i = lb; i <= ub ; i++)
            {
                thf[curr_len] = i;
                leaves_visited += repeat_permutations(curr_len + 1, 0);
                if (leaves_visited == -1) return -1;
            }

        return leaves_visited;
//        }
    } else {

        char current_line[BUFF_SIZE];
        unsigned char cur_key[MD5_DIGEST_LENGTH];
        //Заполнение текущей строки
        int i;
        for (i = 0; i < wanted_length; i++) {
            current_line[i] = alphabet[thf[i]];
        }
        current_line[i] = '\0';
        if (rank == wanted_rank)
            // printf("rank = %d, pwd = %s\n", rank, current_line);

        MD5((const unsigned char *) current_line,
            strlen(current_line),
            (cur_key)
        );

        perm_running++; //счетчик перебранных вариантов

        if (perm_running < chunk)
        {
            if (!(perm_running % sync)){
                MPI_Barrier(MPI_COMM_WORLD);
                MPI_Allreduce(&key_found_loc, &key_found_reduce, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
                // printf("rank = %d, perm = %d, key_found_reduce = %d, key_found_loc = %d\n", rank, perm_running, key_found_reduce, key_found_loc);
                if (key_found_reduce) return -1;
            }

            // сравнение
            if (compare_hash(cur_key) == 0) {
                print(cur_key);
                printf(" <= Password is \"%s\", rank = %d, perm = %d\n\n", current_line, rank, perm_running);
                key_found_loc = 1;
            }
        }
        return 1;
    }

}

int main(int argc, char **args) {
    if (argc != 5) {
        printf("Format: \"Alphabet\" \"Wanted\" \"Lenght\"\n");
        return EXIT_FAILURE;
    }
    MPI_Init(NULL,NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);
    if (!rank){

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

   
    wanted_rank = atoi(args[4]);
    
    int max_wanted_length = wanted_length;

    chunk = pow(alphabet_length, wanted_length) / commsize;

    if (!rank) {
        printf("Alphabet: %s, length: %d\n", alphabet, alphabet_length);
        printf("Wanted(md5): ");
        print(md5_input);
        printf("\n");
        printf("Permutations expected: %d\n", (int)pow(alphabet_length, max_wanted_length));
        printf("Max chunk: %d\n", chunk);
        printf("Sync each %d permutations\n\n", sync);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if (!rank)
        printf("\n");
    MPI_Barrier(MPI_COMM_WORLD);


    // Вызываем функцию перебора,
    // увеличивая длину искомой строки на 1

    //wanted_length = 3;

    // do {
        int perms_curr = pow(alphabet_length, wanted_length);
        // if (commsize > perms_curr) {
            // if (rank > perms_curr)
                // continue;
        // }
        chunk = perms_curr / commsize;
        if (chunk == 0) chunk = 1;
        repeat_permutations(0, 0);
    // } while (!key_found_reduce && max_wanted_length > wanted_length++);

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