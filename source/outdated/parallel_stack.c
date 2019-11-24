#include <string.h>
#include <openssl/md5.h>
#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFF_SIZE 50 // Максимальная длина обрабатываемой строки
#define ALPH_SIZE 100

#define BENCHMARK
// #define SYNC

int alphabet_length, wanted_length;
char alphabet[ALPH_SIZE];
int thf[BUFF_SIZE]; // Индексный массив
unsigned char md5_wanted[MD5_DIGEST_LENGTH]; // Хеш искомой функции
int perm_running = 0;

#ifndef BENCHMARK
    int wanted_rank;
#endif

#ifdef SYNC
    int sync = 2;
#endif

int chunk;
int key_found_loc = 0;
int key_found_reduce = 0;
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
        if (a[i] != md5_wanted[i]) return -1;
    return 0;
}

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

    alphabet[alphabet_length] = '\0';
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

unsigned short recursive_permutations(unsigned short curr_len, unsigned short leaf_reached) {

    if (curr_len != wanted_length) {
        
        unsigned short i, lb;
        float chunk_resize;
        
        if (leaf_reached)
            lb = 0;
        else {
            chunk_resize = chunk / pow(alphabet_length,  wanted_length - curr_len -1);

            lb = (unsigned short)floor(rank * chunk_resize);
            lb = lb % alphabet_length;
        }

       
        // if (rank == 0)
        //printf("new len = %d, rank = %d, chunk = %d, resize = %f, lb = %d\n", curr_len, rank, chunk, chunk_resize, lb);


        if (rank != commsize - 1)
        {
            for(i = lb; i < alphabet_length && perm_running < chunk; i++)
            {
                thf[curr_len] = i;
                
                if (curr_len + 1 != wanted_length)
                    leaf_reached = recursive_permutations(curr_len + 1, leaf_reached);
                else
                    leaf_reached = recursive_permutations(curr_len + 1, 0);
                
                if (leaf_reached == 2) {
                    return 2;
                }
            }
        } else {
            for(i = lb; i < alphabet_length; i++)
            {
                thf[curr_len] = i;
                
                if (curr_len + 1 != wanted_length)
                    leaf_reached = recursive_permutations(curr_len + 1, leaf_reached);
                else
                    leaf_reached = recursive_permutations(curr_len + 1, 0);
                
                if (leaf_reached == 2) {
                    return 2;
                }
            }
        }

        return leaf_reached;

    } else {

        perm_running++; //счетчик перебранных вариантов

        char current_line[wanted_length + 1];
        unsigned char current_key[MD5_DIGEST_LENGTH];
        //Заполнение текущей строки
        unsigned short i;
        for (i = 0; i < wanted_length; i++) {
            current_line[i] = alphabet[thf[i]];
        }
        current_line[i] = '\0';

        // Вывод паролей одного или нескольких процессов на экран, если не используются замеры времени
        #ifndef BENCHMARK
            if (rank == wanted_rank)
                printf("rank %3d, pwd = %s, perm = %d\n", rank, current_line, perm_running);
            else if (wanted_rank < 0)
                printf("rank %3d, pwd = %s, perm = %d\n", rank, current_line, perm_running);
        #endif

        // Хэширование новой строки символов
        MD5((const unsigned char *) current_line,
            strlen(current_line),
            (current_key)
        );

        // Сравнение полученного и исходного хэшей
        if (compare_hash(current_key) == 0) {
            // Если коллизия:

            #ifndef BENCHMARK
                print(current_key);
                printf(" <= collision with \"%s\", rank = %d, perm = %d / %d\n", current_line, rank, perm_running, chunk);

            #else 
                printf("\"%s\"\n", current_line);
            #endif

            // Поднятие флага после первой найденой коллизии
            key_found_loc = 1;
        }

        #ifdef SYNC
            // Синхронизация потоков по номеру пермутации:
            if ( !(perm_running % sync) && (perm_running < chunk) ){
                MPI_Barrier(MPI_COMM_WORLD);
                MPI_Allreduce(&key_found_loc, &key_found_reduce, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
                // printf("rank = %d, perm = %d, key_found_reduce = %d, key_found_loc = %d\n", rank, perm_running, key_found_reduce, key_found_loc);
                if (key_found_reduce) {
                    printf("Exiting process, rank %d, perm %d\n", rank, perm_running);
                    return 2;
                    }
            }
        #endif

    }
    return 1;
}

int main(int argc, char **argv) {
    
    MPI_Init(NULL,NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);

    if (!rank){
        printf("\nHello, human! Let's break this password! :-)\n\n");
        #ifndef BENCHMARK
            if (argc > 5 || argc < 4) {
                printf("Format: \"Alphabet\" \"MD5 Hash\" \"Word lenght\" \"Output Rank with number N (negative to output all; none to disable)\"\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            } else if (argc == 4){
                wanted_rank = ~(1 << 31);
            } else wanted_rank = atoi(argv[4]);

        #else
            if (argc != 4 ) {
                printf("Format: \"Alphabet\" \"MD5 Hash\" \"Word lenght\" \n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        #endif

        
        if(parse_alphabet(argv[1])) {
            printf("Invalid alphabet sectioning!\
            \nSection should look like this: a-z,\
            \n(multiple sections in a row are allowed, commas aren't included into alphabet)\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }

        md5str_to_md5(md5_wanted, argv[2]); // Нахождение 128 битного хэша по входной 256 битной строке

        wanted_length = atoi(argv[3]); // Длина искомой строки

        for(int i = 1; i < commsize; i++)
        {
            MPI_Send(&alphabet_length, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(alphabet, alphabet_length, MPI_CHAR, i, 0, MPI_COMM_WORLD);
            MPI_Send(&md5_wanted, MD5_DIGEST_LENGTH , MPI_CHAR, i, 0, MPI_COMM_WORLD);
            MPI_Send(&wanted_length, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

            #ifndef BENCHMARK
                MPI_Send(&wanted_rank, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            #endif
        }
    } else {
        MPI_Recv(&alphabet_length, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //printf("\nAlph len = %d", alphabet_length);
        MPI_Recv(alphabet, alphabet_length, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&md5_wanted, MD5_DIGEST_LENGTH, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&wanted_length, 1, MPI_INT, 0, 0 , MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        #ifndef BENCHMARK
            MPI_Recv(&wanted_rank, 1, MPI_INT, 0, 0 , MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        #endif
    }

    int max_wanted_length = wanted_length;

    chunk = pow(alphabet_length, max_wanted_length) / commsize;

    if (!rank) {
        printf("Alphabet: %s, length: %d\n", alphabet, alphabet_length);
        printf("md5 to bruteforce: ");
        print(md5_wanted);
        printf("\n");
        printf("Max word length : %d\n", max_wanted_length);
        printf("N processes : %d\n", commsize);
        printf("Permutations expected:\n");
        int total_perms = 0;
        for (int i = 1; i <= max_wanted_length; i++)
        {
            int n = (int)pow(alphabet_length, i);
            printf("  word length %-2d\n\tall : %d\n\tper process : %d\n",i, n, n / commsize);
            total_perms += n;
        }
        printf("  Total\n\tall : %d\n\tper process : %d\n", total_perms, total_perms / commsize);
        printf("Max chunk: %d\n", chunk);

        #ifdef SYNC
            printf("Sync each %d permutations\n", sync);
        #endif

        printf("\nCollisions : \n\n");
    }

    // Вызываем функцию перебора,
    // увеличивая длину искомой строки на 1
  

    wanted_length = 1;
    int perm_sum = 0;
    int rank_iter = 0; // Итератор для цикла по ранкам процессов для случая пропусков вычислений

    // Работа алгоритма пропусков процессов:
    // хранение номера последнего использованного процесса 'rank_iter' позволяет загружать работой
    // всё новые и новые неиспользованные процессы, равномерно загружая процессы В ПРЕДЕЛАХ алгоритма пропусков.
    // Количество пропущенных итераций не учитывается в других секциях программы для оптимизации загрузки процессов.
    
    #ifdef BENCHMARK
        MPI_Barrier(MPI_COMM_WORLD);
        double time, time_sum;
    #endif

    // Основной цикл программы:
    do {

        #ifdef BENCHMARK
            time = MPI_Wtime();
        #endif

        perm_running = 0;
        chunk = pow(alphabet_length, wanted_length) / commsize;

        if (chunk != 0) {
            recursive_permutations(0,0);
        } else {
            // Случай пропуска вычислений процессами:
            chunk = 1;
            int count = 0;
            int perms_curr = pow(alphabet_length, wanted_length);
            while (count++ < perms_curr){
                if (rank == rank_iter) recursive_permutations(0,0);
                rank_iter++;
                if (rank_iter == commsize) rank_iter = 0;
            }
        }

        #ifdef BENCHMARK
            time = MPI_Wtime() - time;
            time_sum += time;
        #endif

        perm_sum += perm_running; // Суммарное количество пермутаций носит лишь информативный характер 
        //printf("rank = %d, PR = %d, PS = %d\n", rank, perm_running, perm_sum);
    } while

    #ifdef SYNC
        (!key_found_reduce && (wanted_length++ < max_wanted_length));
    #else
        (wanted_length++ < max_wanted_length);
    #endif
    
    #ifdef BENCHMARK
        printf("Execution time on process %d: %lf\n", rank, time_sum);
    #endif

    MPI_Allreduce(&key_found_loc, &key_found_reduce, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
    // printf("rank = %d, perm = %d, key_found_reduce = %d, key_found_loc = %d\n", rank, perm_running, key_found_reduce, key_found_loc);
    if (!rank) {
        if(!key_found_reduce)
            printf("\nCollisions not found\n");
        printf("\nPermutations:\n");
    }

    MPI_Barrier(MPI_COMM_WORLD);
    printf("rank %-3d : %d (last) / %d (all)\n", rank, perm_running, perm_sum );

    MPI_Finalize();
    return 0;
}