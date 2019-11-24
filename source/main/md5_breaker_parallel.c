#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../libs/compile_macros.h"
#include "../libs/alphabet.h"
#include "../libs/print_calc_perms.h"
#include "../libs/md5_userlib.h"
#include "../libs/mpi_userlib.h"

#ifndef BENCHMARK
    int wanted_rank; // Ранк, варианты перестановок которого требуется отобразить,      задаётся аргументом main
    unsigned int max_wanted_length; // Маскимальный размер слова при поиске коллизий,   задаётся аргументом main
#endif

#ifdef SYNC
    int sync = SYNC_CONST; // Число проверенных перестановок до проверки на наличие коллизий в остальных процессах (синхронизации)
#endif

#ifdef MALLOC
    unsigned int* CLI; // (Current Letter Index) Массив индексов букв для текущей перестановки букв
    char* alphabet; // Алфавит,                                                         задаётся аргументом main и обрабатывается
    char** collisions = NULL; // Массив для хранения MAX_COLLISIONS последних коллизий
#else
    char alphabet[ALPH_SIZE];
    unsigned int CLI[LINE_SIZE];
    char collisions[LINE_SIZE][MAX_COLLISIONS];
#endif

unsigned char md5_wanted[MD5_DIGEST_LENGTH]; // Хеш искомой функции,                    задаётся аргументом main
unsigned char alphabet_length; // Мощность полученного алфавита
unsigned char wanted_length; // Текущая длина искомой строки
unsigned int perm_running = 0; // Счетчик пермутаций (перестановок)
unsigned int chunk;         // Длина блока вычислений для процесса

unsigned char collision_counter = 0; // Счетчик найденных коллизий
unsigned char collision_memory_overflow = 0; // Флаг переполнения памяти массива коллизий
char key_found_loc = 0;     // Локальный флаг найденной коллизии
char key_found_reduce = 0;  // Сумма флагов коллизий всех процессов

int commsize;               // Число процессов 
int rank;                   // Номер данного процесса


// Отображение информации о введенных аргументах запуска
void print_launch_info(int commsize, int max_wanted_length) {
    printf("Alphabet: %s, length: %d\n", alphabet, alphabet_length);
    printf("md5 to bruteforce: ");
    md5_print(md5_wanted);
    printf("\n");
    printf("Max current_line length : %d\n", max_wanted_length);
    printf("N processes : %d\n", commsize);
    printf("Max chunk: %d\n", (int)pow(alphabet_length, max_wanted_length)/commsize);
}

// Рекурсивная функция осуществления перестановок букв алфавита до искомой длины
// и проверки хэшей полученных слов на соответствие искомому
/*
 Использует глобальные переменные:

    int rank
    unsigned chunk
    unsigned char 
        collision_counter
        wanted_length
        alphabet_length
    char* alphabet || char alphabet[ALPH_SIZE]
    int* CLI       || char CLI[LINE_SIZE]
    char* collisions[8]


    Место в памяти на 1 узле :
        (ppn - числов процессов на 1 узле; в максимуме равно числу ядер)

        4 + 3*1 + (alphabet_length || ALPH_SIZE) + 4 * (max_wanted_length || LINE_SIZE)) * ppn + 8 * (max_wanted_length || LINE_SIZE) =
        (stack)  = 7 + 100 + 200 * ppn + 400 = 507 + 200 * ppn [Байт]
        (64 ядра) = 13207 [Байт] = 12,90 [КБайт],
        (8 ядер)  = 2107 [Байт]  = 2,06 [КБайт]

        (malloc) = 8 + alphabet_length + (4 * ppn + 8) * max_wanted_length [Байт] 
        min (8 ядер)  = 8 + 1 + (4 * 8 + 8) * 1 = 49 [Байт]
        min (64 ядер) = 8 + 1 + (4 * 64 + 8) * 1 = 273 [Байт]
        max (8 ядер)  = 8 + 255 + (4 * 8 + 8) * 255 = 10463 [Байт] = 10,21 [КБайт]
        max (64 ядер) = 8 + 255 + (4 * 64 + 8) * 255 = 67583 [Байт] = 66 [КБайт]

 Передаёт рекурсивно в следующие итерации :

    unsigned char  
        leaf_reached - флаг достижения первого слова для процесса
        curr_len     - текущая длина составленной функцией строки
    При первом вызове curr_len и leaf_reached должны быть равны 0!

    Место в памяти на 1 узле:
        (ppn - числов процессов на 1 узле; в максимуме равно числу ядер)
        (1 + 1) * max_wanted_length * ppn ;
        min = 2 * ppn [Байт]
        (8 ядер)  = 16 [Байт]
        (64 ядер) = 128 [Байт]
        max = 2 * max_wanted_length * ppn = 2 * 255 * ppn [Байт]
        (8 ядер)  = 4080 [Байт]  = 3,98 [КБайт]
        (64 ядра) = 32640 [Байт] = 31,87 [КБайт],

        Справедливо, так как одновременное существование вызовов
        на одной глубине дерева (в одном for цикле) в одном процессе не возможно

 Вычисляет начальные позиции циклов итераций в количестве раз, равном вышине дерева итераций (текущей длине слова)
    затем остальные chunk - wanted_length раз использует 0 для стартовых позиций в последующих вызовах
    и останавливает работу при достижении количества итераций, равном блоку итераций, приходящихся на процесс
    Сложность О(L) + О((M^L)/commsize), где L - текущая искомая длина слова, M - мощность (длина) алфавита.

    (M^L)/commsize - число итераций на процесс, целая часть от частного.
    Остатки от деления проверяются в отдельной функции check_remaninder(int rank);
    
*/
unsigned char recursive_permutations(unsigned char curr_len, unsigned char leaf_reached) {

    if (curr_len != wanted_length) {
        // Часть, задающая буквы для текущей глубины curr_len, растущей с 0 до wanted_length в каждом рекурсивном вызове
        unsigned short i, lb; // Переменные индексов циклов for
        float chunk_resize;   // Пересчитанный под количество итераций на глубине curr_len дробный chunk,
        // домножаемый на rank для получения индекса в текущем множестве итераций,
        // чтобы затем определить по нему индекс в цикле for от 0 до alphabet_length - 1;
        
        if (leaf_reached)
            lb = 0;
        else {
            chunk_resize = chunk / pow(alphabet_length,  wanted_length - curr_len - 1);
            lb = (unsigned)(rank * chunk_resize) % alphabet_length;
        }

       
        // if (rank == 0)
        //printf("new len = %d, rank = %d, chunk = %d, resize = %f, lb = %d\n", curr_len, rank, chunk, chunk_resize, lb);

        // Проверка достижения последней итерации для процесса и проход по циклу с рекурсивным вызовом
        for(i = lb; i < alphabet_length && perm_running < chunk; i++) {

            CLI[curr_len] = i;
            leaf_reached = recursive_permutations(curr_len + 1, leaf_reached);

            #ifdef SYNC
            if (leaf_reached == 2) {
                return 2;
            }
            #endif
        }

        return leaf_reached;

    } else {

        perm_running++; //счетчик перебранных вариантов

        char current_line[wanted_length + 1];
        unsigned char current_key[MD5_DIGEST_LENGTH];

        //Заполнение текущей строки
        unsigned short i;
        for (i = 0; i < wanted_length; i++) {
            current_line[i] = alphabet[CLI[i]];
        }
        current_line[i] = '\0';

        #ifndef BENCHMARK
        // Вывод паролей одного или нескольких процессов на экран, если не используются замеры времени
            if ((perm_running == 1 || perm_running == chunk)) {
                if(rank == wanted_rank) {
                    printf("rank %3d, pwd = %*s, perm = %d\n", rank, max_wanted_length, current_line, perm_running);
                } else if (wanted_rank < 0) {
                    printf("rank %3d, pwd = %*s, perm = %d\n", rank, max_wanted_length, current_line, perm_running);
                }
            }
        #endif

        // Хэширование новой строки символов
        MD5((const unsigned char *) current_line,
            strlen(current_line),
            (current_key)
        );

        // Сравнение полученного и исходного хэшей
        if (compare_hash(current_key, md5_wanted) == 0) {
            // Если коллизия:

            #ifndef BENCHMARK
                if(wanted_rank < commsize) {
                    printf("\nCollision with \"%s\", rank = %d, perm = %d / %d\n\n", current_line, rank, perm_running, chunk);
                } else {
                    printf("Collision with \"%s\", rank = %d, perm = %d / %d\n", current_line, rank, perm_running, chunk);
                }
            #endif 

            strcpy(collisions[collision_counter++], current_line);

            // Сброс счётчика коллизий, заполнение массива вновь с нулевого элемента
            if(collision_counter == MAX_COLLISIONS && collision_memory_overflow == 0) {
                collision_memory_overflow = 1;
                collision_counter = 0;
            }

            // Поднятие флага после первой найденой коллизии
            key_found_loc = 1;
        }

        #ifdef SYNC
            // Синхронизация потоков по номеру пермутации:
            if ( !(perm_running % sync) && (perm_running < chunk) ) {
                MPI_Allreduce(&key_found_loc, &key_found_reduce, 1, MPI_CHAR, MPI_SUM, MPI_COMM_WORLD);
                // printf("rank = %d, perm = %d, key_found_reduce = %d, key_found_loc = %d\n", rank, perm_running, key_found_reduce, key_found_loc);
                if (key_found_reduce) {
                    // printf("Exiting process, rank %d, perm %d\n", rank, perm_running);
                    return 2;
                    }
            }
        #endif

    }
    return 1;
}


void check_remainder(int num) {

    perm_running++;

    #ifndef BENCHMARK
        unsigned rank = num;
    #endif

    char current_line[wanted_length + 1];
    current_line[wanted_length] = '\0';

    unsigned i = wanted_length - 1;
    do
    {
        current_line[i] = alphabet[alphabet_length - 1 - num % alphabet_length];
        num /= alphabet_length;
        i--;
    } while(num > 0);

    i++;
    for(int j = 0; j < i; j++) {
        current_line[j] = alphabet[alphabet_length - 1];
    }

    #ifndef BENCHMARK   
        if(rank == wanted_rank) {
            printf("rank %3d, pwd = %*s, perm = %d <== remainder\n", rank, max_wanted_length, current_line, perm_running);
        } else if (wanted_rank < 0) {
            printf("rank %3d, pwd = %*s, perm = %d <== remainder\n", rank, max_wanted_length, current_line, perm_running);
        }
    #endif

    unsigned char current_key[MD5_DIGEST_LENGTH];

    // Хэширование новой строки символов
    MD5((const unsigned char *) current_line,
        strlen(current_line),
        (current_key)
    );

    // Сравнение полученного и исходного хэшей
    if (compare_hash(current_key, md5_wanted) == 0) {
        // Если коллизия, просто выводим её на экран:

        #ifndef BENCHMARK
            printf("Collision with \"%s\", rank = %d, perm = %d / %d\n", current_line, rank, perm_running, chunk);
        #endif

        strcpy(collisions[collision_counter++], current_line);
        // Сброс счётчика коллизий, заполнение массива вновь с нулевого элемента
        if(collision_counter == MAX_COLLISIONS && collision_memory_overflow == 0) {
            collision_memory_overflow = 1;
            collision_counter = 0;
        }

        // Поднятие флага после первой найденой коллизии
        key_found_loc = 1;
    }
    return;
}

int main(int argc, char **argv) {
    
    MPI_Init(NULL,NULL); 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);


    // Обработка входных аргументов в нулевом процессе по стандарту MPI
    if (!rank) {
        #ifndef BENCHMARK
            if (argc > 5 || argc < 4) {
                printf("Format: \"Alphabet\" \"MD5 Hash\" \"Word lenght\" \"Output Rank with number N (negative to output all; none to disable)\"\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            } else if (argc == 4) {
                wanted_rank = ~(1 << 31);
            } else wanted_rank = atoi(argv[4]);

        #else
            if (argc != 4 ) {
                printf("Format: \"Alphabet\" \"MD5 Hash\" \"Word lenght\" \n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        #endif

        alphabet_length = get_alphabet_size(argv[1]);

        if(alphabet_length == 0) {
            printf("Invalid alphabet sectioning!\
            \nSection should look like this: a-z,\
            \n(multiple sections in a row are allowed, commas aren't included into alphabet)\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }

        #ifdef MALLOC
        // Динамическое аллоцирование алфавита полученной длины
            alphabet = (char*)malloc(sizeof(char)* alphabet_length);
        #endif

        parse_alphabet(argv[1], alphabet);

        md5str_to_md5(md5_wanted, argv[2]); // Нахождение 128 битного хэша по входной 256 битной строке

        wanted_length = atoi(argv[3]); // Длина искомой строки    

    }

    // Рассылка аргументов всем остальным потокам  
    MPI_Bcast(&alphabet_length, 1, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    #ifdef MALLOC
        if(rank)
            alphabet = malloc(sizeof(char) * alphabet_length + 1);
    #endif

    MPI_Bcast(alphabet, alphabet_length + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Bcast(&md5_wanted, MD5_DIGEST_LENGTH , MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Bcast(&wanted_length, 1, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    
    // if(rank == 0)
    //     printf("sent alphabet :     %s\n", alphabet);
    // if(rank == 1)
    //     printf("recieved alphabet : %s\n", alphabet);
    // MPI_Barrier(MPI_COMM_WORLD);

    #ifndef BENCHMARK
        MPI_Bcast(&wanted_rank, 1, MPI_INT, 0, MPI_COMM_WORLD);
        max_wanted_length = wanted_length;
    #else
        int max_wanted_length = wanted_length;
    #endif

    #ifdef MALLOC
        collisions = malloc(sizeof(char*) * 8 + sizeof(char)*(max_wanted_length+1));
        for(int i = 0; i < 8; i++)
            collisions[i] = (char*)malloc(sizeof(char) * (max_wanted_length + 1));
    #endif

    // Вывод информации перед запуском:
    // Здесь выводятся входные параметры и предсказывается количество пермутаций для каждой итерации главного цикла по длине слов и по процессам
    if (!rank) {

        #ifndef MEASURE_ONLY
            printf("\nHello, human! Let's break this password! :-)\n\n");    
            print_launch_info(commsize, max_wanted_length);
            printf("\n--------------------------------\n\n");
           
            print_perms_info(alphabet_length, max_wanted_length, commsize);
            printf("\n--------------------------------\n");
        #else
            printf("N processes = %d, Max word length: %d,  Alphabet : %s\n", commsize, max_wanted_length, argv[1]);
        #endif
            
        #ifdef SYNC
            printf("\nSync each %d permutations\n", sync);
            #ifndef MEASURE_ONLY
                printf("\n--------------------------------\n");
            #endif
        #endif

        #ifndef BENCHMARK
            printf("\nCollisions :\n\n");
        #endif
    }
    
    #ifdef MALLOC
        CLI = malloc(sizeof(unsigned short) * alphabet_length);
    #endif

    /*
    *
    *
    * * * * * * * * * * * * * * * *
    *   Основной цикл программы   *
    * * * * * * * * * * * * * * * *
    *
    *
    */

    int perm_sum  = 0; // Счётчик общего количества переборов
    int rank_iter = 0; // Итератор для цикла по ранкам процессов для случая пропусков вычислений
    wanted_length = 1; // Начинаем с длины слова в 1 букву

    // Работа алгоритма пропусков процессов:
    // хранение номера последнего использованного процесса 'rank_iter' позволяет загружать работой
    // всё новые и новые неиспользованные процессы, равномерно загружая процессы В ПРЕДЕЛАХ алгоритма пропусков.
    // Количество пропущенных итераций не учитывается в других секциях программы для оптимизации загрузки процессов.
    

    MPI_Barrier(MPI_COMM_WORLD);

    #ifdef BENCHMARK
        double time, time_sum; // Переменные замера времени рекурсивной функции для каждого процесса
        double time_complete = MPI_Wtime(); // Переменная замера времени до завершения обхода дерева всеми процессами
    #endif


    do {

        #ifdef BENCHMARK
            time = MPI_Wtime();
        #endif

        perm_running = 0;

        int perms_curr = pow(alphabet_length, wanted_length);
        chunk = perms_curr / commsize;

        if (chunk != 0) {
            recursive_permutations(0,0);
            if(perms_curr % commsize > rank)
                check_remainder(rank);
        } else {
            // Случай пропуска вычислений процессами:
            chunk = 1;
            int count = 0;
            while (count++ < perms_curr) {
                if (rank == rank_iter) {
                    recursive_permutations(0,0);
                }
                rank_iter++;
                if (rank_iter == commsize) rank_iter = 0;
            }
        }

        #ifdef BENCHMARK
            time = MPI_Wtime() - time;
            time_sum += time;
        #endif

        perm_sum += perm_running; // Суммарное количество пермутаций носит лишь информативный характер, исключено из замеров времени,
                                  // как и операции сравнения в цикле

        //printf("rank = %d, PR = %d, PS = %d\n", rank, perm_running, perm_sum);
    } while
    // Использование доп. условия на выход из цикла для случая синхронизации
    #ifdef SYNC
        (!key_found_reduce && (wanted_length++ < max_wanted_length));
    #else
        (wanted_length++ < max_wanted_length);
    #endif

    // Определение общего числа коллизий из всех процессов в key_found_reduce
    MPI_Allreduce(&key_found_loc, &key_found_reduce, 1, MPI_CHAR, MPI_SUM, MPI_COMM_WORLD);
    // printf("rank = %d, perm = %d, key_found_reduce = %d, key_found_loc = %d\n", rank, perm_running, key_found_reduce, key_found_loc);

    #ifdef BENCHMARK
        // Программа завершила работу после того, как определила общее число коллизий совершив все возможные варианты перебора
        time_complete = MPI_Wtime() - time_complete;
    #endif

    if(!rank && key_found_reduce) {
        #ifndef MEASURE_ONLY
            printf("\n");
        #endif
        printf("Collisions arrays :\n");
    }

    MPI_Recv(NULL, 0, MPI_INT, (rank > 0) ? rank - 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  
    if(key_found_loc) {

        printf("rank %d : { %s",rank,collisions[0]);
        for(int j = 1; j < collision_counter; j++)
            printf(", %s", collisions[j]);
        printf(" }\n");
    }
    MPI_Ssend(NULL, 0, MPI_INT, rank != commsize - 1 ? rank + 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD);


    MPI_Barrier(MPI_COMM_WORLD);

    if (!rank) {
        if(!key_found_reduce)
            printf("\nCollisions not found\n");
        else 
            printf("\n");
        
        #ifndef MEASURE_ONLY
            printf("--------------------------------\n\n");
        #endif
    
    }

    // Вывод результатов работы:
    char* message = malloc(sizeof(char)*100);

    if(!rank) {
        printf("Permutations executed:\n");
        #ifndef BENCHMARK
            printf("\n");
        #endif
    }
    sprintf(message,"%d (last) / %d (all)", perm_running, perm_sum);
    MPI_Print_in_rank_order_unique(commsize, rank, message);

    #ifdef BENCHMARK
    {
        double time_min, time_max, time_avg;
        // double time_compare;
        long long int time_a, time_b;


        #ifdef TIME_CHECK
            if (commsize < N_CHECK) {
                MPI_Barrier(MPI_COMM_WORLD);
                sprintf(message,"Execution time on process %d: %lf\n", rank, time_sum);
                MPI_Print_in_rank_order(commsize, rank, message);
            }
        #endif

        MPI_Allreduce(&time, &time_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);

        //double // time_compare = time - time * 0.00001;
        // if\(time_min > time_compare) {
        
        time_a = time_min * 1000000;
        time_b = time * 1000000;
        if(!rank)
            printf("Execution time's mean on %d processes\n", commsize);
        if(time_a == time_b) {
            printf("\tminimal : %lf seconds on rank %d;\n", time_min, rank);
        }

        MPI_Allreduce(&time, &time_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

        // time_compare = time + time * 0.00001;
        // if\(time_max < time_compare) {

        time_a = time_max * 1000000;
        time_b = time * 1000000;
        if(time_a == time_b) {
            printf("\tmaximal : %lf seconds on rank %d;\n", time_max, rank);
        }

        MPI_Allreduce(&time, &time_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        if(!rank) {
            time_avg /= commsize;
            printf("\tmean :    %lf seconds.\n", time_avg);
            printf("Program finished last permutation after\n");
        }

        MPI_Allreduce(&time_complete, &time_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);

        // time_compare = time_complete - time_complete * 0.00001;
        // if\(time_min > time_compare) {

        time_a = time_min * 1000000;
        time_b = time_complete * 1000000;
        if(time_a == time_b) {
            printf("\tminimal : %lf seconds on rank %d;\n", time_min, rank);
        }

        MPI_Allreduce(&time_complete, &time_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

        // time_compare = time_complete + time_complete * 0.00001;
        // if\(time_max < time_compare) {

        time_a = time_max * 10000;
        time_b = time_complete * 10000;
        if(time_a == time_b) {
            printf("\tmaximal : %lf seconds on rank %d;\n", time_max, rank);
        }

        MPI_Allreduce(&time_complete, &time_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        if(!rank) {
            time_avg /= commsize;
            printf("\tmean :    %lf seconds.\n", time_avg);
        }
    }
    #endif
    
    #ifdef MALLOC
        free(CLI);
        free(alphabet);
        free(collisions);
    #endif

    MPI_Finalize();
    return 0;
}