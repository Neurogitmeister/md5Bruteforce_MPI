#include <sys/time.h>

#include "../libs/md5_breaker_serial.h"
#include "../libs/wtime.h"


int recursive_permutations(const unsigned curr_len) {
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

        perm_running++; //счетчик перебранных вариантов

        //Заполнение текущей строки
        int i;
        for (i = 0; i < wanted_length; i++) {
			if (current_word[i] != alphabet[CLI[i]])
            current_word[i] = alphabet[CLI[i]];
        }
        current_word[i] = '\0';

        MD5((const unsigned char *) current_word,
            strlen(current_word),
            (current_key)
        );


        /*
            printf("%s|%d ", current_word, count_perm);
            printf("[%d] \t(%s)\t", (int)strlen(current_word), current_word);
            print(cur_key);printf(" ");
            printf("\n");
        */

        // Сравнение полученного и исходного хэшей
        if (compare_hash(current_key, md5_wanted) == 0) {
            // Если коллизия:
            

            #ifndef BENCHMARK
            {
                // Поднятие флага после первой найденой коллизии
                if (collision_found == 0)  {
                    collision_time_stamp = Wtime();
                    collision_found = 1;
                }

                PrintTime(collision_time_stamp - global_time_start);
                printf("perm = %llu, collision with \"%s\"\n\n", perm_running, current_word);
                
            }
            #else
                // Поднятие флага и замер времени
                if (collision_found == 0) {
                    #ifdef BENCHMARK_FIRST_COLLISION
                        collision_time_stamp = Wtime();
                    #endif
                    collision_found = 1;
                }
            #endif 

            // Если переполнение массива коллизий

            if ( (collision_counter) && ( (collision_counter) % MAX_COLLISIONS) == 0 ) {
            #ifdef MALLOC
                // Динамическое расширение и копирование всех предыдущих коллизий
                if (expand_cstring_array(&collisions, max_wanted_length, collision_counter, collision_counter + MAX_COLLISIONS)){
                    printf("ERROR! Couldn't allocate memory for new collisions array size!\n");
                }

            #else
				#ifdef LAST_COLLISIONS
                // Обнуление счётчика для перезаписи первых добавленных до перезаписи коллизий
                collision_counter = 0;
				#endif
                // Оповещение о переполнение массива, означающее что количество будет равно MAX_COLLISIONS
                collision_overflow = 1;
				
			#endif
            }
			#if !defined (LAST_COLLISIONS) && !defined (MALLOC)
			if (!collision_overflow)
			#endif
            strcpy(collisions[collision_counter++], current_word);

                
            #ifdef STOP_ON_FIRST
            // Завершение работы при первой найденной коллизии
                return 1;
            #endif
        }
    }
    return 0;
}

int main(int argc, char **argv) {

    // Переменные внешнего цикла while, способные осущесвлять перебор в прямом и обратном порядке длин слов,
    // в зависимости от входных аргументов :
    int increment = 1; // Инкремент

    #ifdef BENCHMARK
        // Локальные версии переменных
        #ifndef MALLOC
        unsigned max_wanted_length; // Маскимальный размер слова при поиске коллизий,   задаётся аргументом main
        #endif
    #endif

    unsigned min_wanted_length; // Минимальный размер слова при поиске коллизий,    задаётся аргументом main

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
        printf("ERROR! Invalid alphabet sectioning!\
        \nSection should look like this: a-z,\
        \n(multiple sections in a row are allowed, commas aren't included into alphabet)\n");
        return 1;
    }

    if(parse_arg_to_unsigned(argv[3],'-',&min_wanted_length,&max_wanted_length))
    {
        printf("ERROR! Invalid max-min length argument!\n");
        return 1;
    }
    

    printf("Alphabet[%d] : %s | Word length: %d-%d\n", 
        alphabet_length, argv[2], min_wanted_length, max_wanted_length);
    printf("full alphabet : %s\n", alphabet);
    printf("md5 to bruteforce: ");
    md5_print(md5_wanted);
    printf("\n\n");

    #ifdef STOP_ON_FIRST
        printf("USING \"STOP_ON_FIRST\"\n\n");
    #endif

    print_perms_info(alphabet_length,min_wanted_length,max_wanted_length,1);
    unsigned long long geometric;
    if ( (geometric = geometric_series_sum(alphabet_length,min_wanted_length, max_wanted_length)) == 0 )
        geometric = geometric_series_sum(alphabet_length,max_wanted_length, min_wanted_length);
    printf("\n\tGeometric progression sum : %llu\n", geometric);


    #ifndef BENCHMARK
        printf("\n-------------------------------------------\n");
        printf("\nCollisions :\n\n");
    #endif

    //  Переключение на обратный ход внешнего цикла while :
    if (max_wanted_length < min_wanted_length) {
        // Перестановка значений для корректной работы с памятью
        unsigned temp = min_wanted_length;
        min_wanted_length = max_wanted_length;
        max_wanted_length = temp;
        wanted_length = max_wanted_length;
        increment = -1;

    } else {
        increment = 1;
        wanted_length = min_wanted_length;
    }

    #ifdef MALLOC
        current_word = malloc(sizeof(char) * max_wanted_length);
        collisions = malloc(sizeof(char*) * MAX_COLLISIONS + sizeof(char)*(max_wanted_length+1));
        for(int i = 0; i < MAX_COLLISIONS; i++)
            collisions[i] = (char*)malloc(sizeof(char) * (max_wanted_length + 1));
        CLI = malloc(sizeof(unsigned short) * alphabet_length);
    #endif

    // Переменные замера производительности рекурсивной функции для каждого процесса

    #ifdef BENCHMARK
        double time_single; 
        double time_sum = 0; 
    #endif
    #ifdef BENCHMARK_SEPARATE_LENGTHS
        double times_single[max_wanted_length - min_wanted_length + 1];
    #endif
    #ifdef BENCHMARK_TOTAL_PERMS
        double time_sums[max_wanted_length - min_wanted_length + 1];
    #endif
    #ifdef BENCHMARK_FIRST_COLLISION
        double first_collision_time = 0.0, first_collision_time_sum = 0.0;  // Переменная времени нахождения первой коллизии
        unsigned char first_collision_measured = 0;
        double time_single_end;
    #endif
    
    perm_running = 0; // Загружаем в кэш процессора
    global_time_start = Wtime();

    do {

    #ifdef BENCHMARK

        time_single = Wtime();
        recursive_permutations(0);

        #ifdef BENCHMARK_FIRST_COLLISION
            time_single_end = Wtime();
            if (!collision_found) {
                time_single = time_single_end - time_single;
                time_sum += time_single;
                first_collision_time_sum = time_sum;
            } else {
                if (first_collision_measured == 0) {
                    first_collision_time = collision_time_stamp - time_single + first_collision_time_sum;
                    first_collision_measured = 1;
                }
                time_single = time_single_end - time_single;
                time_sum += time_single;
            }
        #else
            time_single = Wtime() - time_single;
            time_sum += time_single;
        #endif

        #ifdef BENCHMARK_SEPARATE_LENGTHS
            times_single[wanted_length - min_wanted_length] = time_single;
        #endif
        #ifdef BENCHMARK_TOTAL_PERMS
            time_sums[wanted_length - min_wanted_length] = time_sum;
        #endif
    #else
        recursive_permutations(0);
    #endif

        wanted_length += increment;
    }
    // Использование доп. условия на выход из цикла для случая синхронизации
    #ifdef STOP_ON_FIRST
        while ((wanted_length <= max_wanted_length && wanted_length >= min_wanted_length) && !collision_counter);

    #else
        while (wanted_length <= max_wanted_length && wanted_length >= min_wanted_length);
    #endif
    

    if(collision_found == 0) {

        #ifdef BENCHMARK
        printf("\n");
        #endif
        printf("-------------------------------------------\n");
        printf("      ;C   no collisions found    >;\n");
        printf("-------------------------------------------\n");
    } else {

        printf("\n");
        printf("-------------------------------------------\n");
        printf(">>>----->  %d collision(s) found  <-----<<<\n", collision_counter);
        printf("-------------------------------------------\n");

        printf("\nCollisions array :\nsize = %d { %s", collision_counter, collisions[0]);
        for(int j = 1; j < collision_counter; j++)
            printf(", %s", collisions[j]);
        printf(" }\n");
    }

    printf("\nPermutations executed : %llu\n\n", perm_running);

    #ifdef BENCHMARK

        printf("Execution times (in seconds) :\n");
        printf("alphabet length = %u, word length (WL) :\n", alphabet_length);
        
        #if defined(BENCHMARK_SEPARATE_LENGTHS) || defined(BENCHMARK_TOTAL_PERMS)

        unsigned curr_length;
        unsigned counter_start, counter;
        if (increment == 1) {
            counter_start = wanted_length - min_wanted_length;
        } else {
            counter_start = max_wanted_length - wanted_length;
        }

        #endif

        #ifdef BENCHMARK_SEPARATE_LENGTHS
        counter = counter_start;
        if (increment == 1) {
            curr_length = min_wanted_length;
        } else {
            curr_length = max_wanted_length;
        }

         // Проходим по массиву замеров для разных длин слова
        while (counter--) {
            printf("WL %3u%5s\t%.*lf\n", curr_length," ",
            6, times_single[curr_length - min_wanted_length]);
            curr_length += increment;
        }
        printf("\n");
        #endif

        #ifdef BENCHMARK_TOTAL_PERMS
        unsigned first_length;
        counter = counter_start;
        if (increment == 1) {
            curr_length = min_wanted_length;
            first_length = min_wanted_length;
        } else {
            curr_length = max_wanted_length;
            first_length = max_wanted_length;
        }
        while (counter--) {
            printf("WL %3u-%-3u\t%.*lf\n", first_length, curr_length,
            6, time_sums[curr_length - min_wanted_length]);

            curr_length += increment;
        }
        #ifdef LOGFILE
        unsigned curr_length_start;
        if (increment == 1) {
            curr_length_start = min_wanted_length;
            first_length = min_wanted_length;
        } else {
            curr_length_start = max_wanted_length;
            first_length = max_wanted_length;
        }
        counter = counter_start;
        curr_length = curr_length_start;
        printf("WL ");
        while (counter--) {
            printf("%u-%u ", first_length, curr_length);
            curr_length += increment;
        }
        counter = counter_start;
        curr_length = curr_length_start;
        printf("\nSerial ");
        while (counter--) {
            printf("%.*lf ", 6, time_sums[curr_length - min_wanted_length]);
            curr_length += increment;
        }
        #endif
        #else
        printf("WL %3u-%-3u\t%.*lf\n", min_wanted_length, max_wanted_length, 6, time_sum);
        #endif

    printf("\nProgram finished after : ");
    
    printf("%.*lf sec.\n", 6, time_sum);

    #endif

    #ifdef BENCHMARK_FIRST_COLLISION

    if (collision_counter == 0)
        printf("\nFirst collision isn't found.\n");
    else
        printf("\n");
    if (first_collision_measured) {
        printf("First collision found after %lf sec.\n\n", first_collision_time);
    }

    #endif



    /*  DO SOMETHING WITH RESULTING ARRAY  */



    #ifdef MALLOC
    
        for(int i = 0; i < (collision_counter / MAX_COLLISIONS) + 1; i++)
        {
            free(collisions[i]);
        }
        free(collisions);
        free(CLI);
        free(alphabet);

    #endif

    return 0;
}
