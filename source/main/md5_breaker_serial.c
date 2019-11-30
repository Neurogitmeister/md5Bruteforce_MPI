#include <sys/time.h>

#include "../libs/compile_macros.h"
#include "../libs/wtime.h"

#ifndef PARALLEL

int recursive_permutations(unsigned curr_len) {
    if (curr_len != wanted_length)  {
        for (int j = 0; j < alphabet_length; j++) {
            // Генерируем последовательность
            CLI[curr_len] = j;
            // Вызываем перестановки для текущей буквы
            #ifdef STOP_ON_FIRST
            if (recursive_permutations(curr_len + 1))return 1;
            #else
            recursive_permutations(curr_len + 1);
            #endif
        }
    } else {
        char current_line[wanted_length + 1];
        unsigned char current_key[MD5_DIGEST_LENGTH];
        //Заполнение текущей строки
        int i;
        for (i = 0; i < wanted_length; i++) {
            current_line[i] = alphabet[CLI[i]];
        }
        current_line[i] = '\0';

        MD5((const unsigned char *) current_line,
            strlen(current_line),
            (current_key)
        );

        perm_running++; //счетчик перебранных вариантов

        /*
            printf("%s|%d ", current_line, count_perm);
            printf("[%d] \t(%s)\t", (int)strlen(current_line), current_line);
            print(cur_key);printf(" ");
            printf("\n");
        */

        // Сравнение полученного и исходного хэшей
        if (compare_hash(current_key, md5_wanted) == 0) {
            // Если коллизия:

            
            

            #ifndef BENCHMARK
                
                if(wanted_rank < commsize) {
                    printf("\nCollision with \"%s\", rank = %4d, perm = %llu / %llu\n\n", current_line, rank, perm_running, chunk);
                } else {
                    printf("Collision with \"%s\", rank = %4d, perm = %llu / %llu\n", current_line, rank, perm_running, chunk);
                }

                // Поднятие флага после первой найденой коллизии
                if (collision_found == 0) 
                    collision_found = 1;
            #else
                // Поднятие флага и замер времени
                if (collision_found == 0) {
                    collision_time_stamp = Wtime();
                    collision_found = 1;
                }
            #endif 

            strcpy(collisions[collision_counter++], current_line);

            // Сброс счётчика коллизий, заполнение массива вновь с нулевого элемента
            if(collision_counter == MAX_COLLISIONS) {
                collision_counter = 0;
            }

                
            #ifdef STOP_ON_FIRST
            // Завершение работы при первой найденной коллизии
                return 1;
            #endif
        }
    }
    return 0;
}

int main(int argc, char **argv) {

    unsigned max_wanted_length; // Маскимальный размер слова при поиске коллизий,   задаётся аргументом main
    unsigned min_wanted_length; // Минимальный размер слова при поиске коллизий,   задаётся аргументом main
    
    if (argc != 4 ) {
        printf("Format: \"MD5 Hash\" \"Alphabet\" \"Word lenght\" \n");
        return 1;
    }


    md5str_to_md5(md5_wanted, argv[1]); // Нахождение 128 битного хэша по входной 256 битной строке

     #ifdef MALLOC
        // Динамическое аллоцирование алфавита подсчитанной функцией длины
        parse_alphabet_malloc(argv[2], &alphabet, &alphabet_length);
    #else
        parse_alphabet(argv[2], alphabet, &alphabet_length);
    #endif



    if(alphabet_length == 0) {
        printf("Invalid alphabet sectioning!\
        \nSection should look like this: a-z,\
        \n(multiple sections in a row are allowed, commas aren't included into alphabet)\n");
        return 1;
    }

    if(parse_arg_to_unsigned(argv[3],'-',&min_wanted_length,&max_wanted_length))
    {
        printf("Invalid max-min length argument!\n");
        return 1;
    }
    

    
    #ifdef MALLOC
        collisions = malloc(sizeof(char*) * MAX_COLLISIONS + sizeof(char)*(max_wanted_length+1));
        for(int i = 0; i < MAX_COLLISIONS; i++)
            collisions[i] = (char*)malloc(sizeof(char) * (max_wanted_length + 1));
    #endif


    #ifdef MALLOC
        CLI = malloc(sizeof(unsigned short) * alphabet_length);
    #endif

    printf("Alphabet[%d] : %s | Word length: %d-%d\n", 
    alphabet_length, argv[2], min_wanted_length, max_wanted_length);
    printf("md5 to bruteforce: ");
    md5_print(md5_wanted);
    printf("\n\n");
    print_perms_info(alphabet_length,min_wanted_length,max_wanted_length,1);

    #ifdef STOP_ON_FIRST
        printf("\nUSING \"STOP_ON_FIRST\"\n", SYNC_CONST);
        #ifndef BENCHMARK
            printf("\n--------------------------------\n");
        #endif
    #endif

    #ifndef BENCHMARK
        printf("\nCollisions :\n\n");
    #endif

    wanted_length = min_wanted_length;

    #ifdef BENCHMARK_SEPARATE_LENGTHS
        double times_single[max_wanted_length - min_wanted_length + 1];
    #endif
    #ifdef BENCHMARK_TOTAL_PERMS
        double time_sums[max_wanted_length - min_wanted_length + 1];
    #endif
    #ifdef BENCHMARK
        double time_single, time_sum; // Переменные замера времени рекурсивной функции для каждого процесса

        double time_complete = Wtime(); // Переменная замера времени до завершения обхода дерева всеми процессами
    #endif
    do {
    #ifdef BENCHMARK

        time_single = Wtime();
        
        recursive_permutations(0);

        time_single = Wtime() - time_single;
        time_sum += time_single;
        #ifdef BENCHMARK_SEPARATE_LENGTHS
            times_single[wanted_length - min_wanted_length] = time_single;
        #endif
        #ifdef BENCHMARK_TOTAL_PERMS
            time_sums[wanted_length - min_wanted_length] = time_sum;
        #endif
    #else
        recursive_permutations(0);
    #endif

    } while
    #ifdef STOP_ON_FIRST
        (!collision_found && max_wanted_length > wanted_length++);
    #else
        (max_wanted_length > wanted_length++);
    #endif

    time_complete = Wtime() - time_complete;

    if(collision_found) {

        printf("!!! Collisions found !!!\n\n");
        printf("Collisions array : { %s",collisions[0]);
        for(int j = 1; j < collision_counter; j++)
            printf(", %s", collisions[j]);
        printf(" }\n");
    } else {
        printf("!!! Collisions not found !!!\n");
    }

    printf("\nPermutations executed : %llu\n\n", perm_running);

    #ifdef BENCHMARK

        printf("Execution times for alphabet length %u, word length (WL) :\n", alphabet_length);
       
        #ifdef BENCHMARK_SEPARATE_LENGTHS
        for( wanted_length = min_wanted_length; wanted_length <= max_wanted_length; wanted_length++) {
                printf("WL %3u%5s\t%.*lf sec.\n", wanted_length," ", 6, times_single[wanted_length - min_wanted_length]);
        }
        printf("\n");
        #endif
        #ifdef BENCHMARK_TOTAL_PERMS
        for( wanted_length = min_wanted_length; wanted_length <= max_wanted_length; wanted_length++) {
                printf("WL %3u-%-3u\t%.*lf sec.\n", min_wanted_length, max_wanted_length, 6, time_sums[wanted_length-min_wanted_length]);
        }
        #else
        printf("WL %3u-%-3u\t%.*lf sec.\n", min_wanted_length, max_wanted_length, 6, time_sum);
        #endif
    #endif
    printf("\nProgram counted total collisions after : ");
    
    printf("%.*lf sec.\n", 6, time_complete);
    
    return 0;
}

#else

int main(int argc, char* argv[])
{
    printf("\n *** Compiles only without PARALLEL defined in compile_macros.h ! ***\n\n");
    return 1;
}

#endif