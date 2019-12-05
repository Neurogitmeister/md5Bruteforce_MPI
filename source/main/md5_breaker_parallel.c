#include "../libs/md5_breaker_parallel.h"


#include <mpi.h>
#include "../libs/mpi_userlib.h"
#include "../libs/wtime.h"

// Рекурсивная функция осуществления перестановок букв алфавита до искомой длины
// и проверки хэшей полученных слов на соответствие искомому
unsigned char recursive_permutations(unsigned curr_len, unsigned char leaf_reached) {

/*      
    Принцип работы:

    Вычисляет начальные позиции циклов итераций в количестве раз, равном вышине дерева итераций (текущей длине слова)
    затем остальные chunk - wanted_length раз использует 0 для стартовых позиций в последующих вызовах
    и останавливает работу при достижении количества итераций, равном блоку итераций, приходящихся на процесс
    Сложность О(L) + О((M^L)/commsize), где L - текущая искомая длина слова, M - мощность (длина) алфавита.

    (M^L)/commsize - число итераций на процесс, целая часть от частного.
    Остатки от деления проверяются в отдельной функции check_remaninder(int rank);
*/

/*
 Использует глобальные переменные:

    int rank
    unsigned long long chunk           0 - 2^64-1
    unsigned long long perm_running    0 - 2^64-1
    unsigned char collision_counter    0 - 2^8-1
    unsigned int wanted_length         0 - 2^32-1
    unsigned int alphabet_length       0 - 2^32-1
    unsigned char md5_wanted[MD5_DIGEST_LENGTH];
    --------------------------------------------------------------------------------
    |  defined(MALLOC)                |  !defined(MALLOC)                          |
    |---------------------------------|--------------------------------------------|
    |char* alphabet                   | char alphabet[ALPH_SIZE]                   |
    |unsigned* CLI                    | unsigned CLI[LINE_SIZE]                    |
    |char* collisions[MAX_COLLISIONS] | char collisions[LINE_SIZE][MAX_COLLISIONS] |
    --------------------------------------------------------------------------------
    |   !defined(BENCHMARK)                                                   |
    |-------------------------------------------------------------------------|
    | int commsize,                                                           |
    | unsigned max_wanted_length,                                             |
    | unsigned min_wanted_length ***                                          |
    | *** Версия с выводом на экран крайних элементов потоков использует      |
    | обращение к этим переменным при нахождении коллизии, что происходит     |
    | редко, следовательно ими можем принебречь в расчётах (12 Байт)          |
    ---------------------------------------------------------------------------


    * * * Расчёт занимаемой памяти на 1 вычислительном узле * * *

    (здесь "ppn (processes per node)" - числов процессов на 1 узле; в максимуме равно числу ядер)

 Место для глобальных переменных в памяти на 1 узле :

        (sizeof(type1)[Байт] * N1 + sizeof(type2)[Байт] * N2 + ... )

        8*2 + 1 + 4*1 + 1*(alphabet_length || ALPH_SIZE) + 4 * (max_wanted_length || LINE_SIZE)) * ppn + 8 * (max_wanted_length || LINE_SIZE) =

        defined(MALLOC):

        (heap) = 21 + alphabet_length + (4 * ppn + 8) * max_wanted_length [Байт] 
        min(1) (8 ядер)  = 21 + 1 + (4 * 8 + 8)  * 1 = 62 [Байт]
        min(1) (64 ядер) = 21 + 1 + (4 * 64 + 8) * 1 = 283 [Байт]
        mid(255) (8 ядер)  = 21 + 255 + (4 * 8 + 8)  * 255 = 10476 [Байт] = 10,23 [КБайт]
        mid(255) (64 ядер) = 21 + 255 + (4 * 64 + 8) * 255 = 67596 [Байт] = 66,01 [КБайт]
        max(2^32-1) (8 ядер)  = 21 + 4 294 967 295 + (4 * 8 + 8)  * 4 294 967 295 = 164 [ГБайт]
        max(2^32-1) (64 ядер) = 21 + 4 294 967 295 + (4 * 64 + 8) * 4 294 967 295 = 1060 [ГБайт]

        !defined(MALLOC):

        (stack)  = 21 + 100 + 200 * ppn + 400 = 521 + 200 * ppn [Байт]
        (64 ядра) = 13221 [Байт] = 12,91 [КБайт],
        (8 ядер)  = 2121 [Байт]  = 2,07 [КБайт]

 Передаёт рекурсивно в следующие итерации :

    unsigned char  
        leaf_reached - флаг достижения первого слова для процесса
        curr_len     - текущая длина составленной функцией строки
    (при первом вызове curr_len и leaf_reached должны быть равны 0)

 Место для локальных переменных в памяти на 1 узле:

        !!! одновременное существование двух или нескоьких вызовов функции на одной глубине дерева
        !!! (в одном for цикле) в одном процессе не возможно, поэтому кол-во рекурсивно создаваемых
        !!! в памяти переменных ограничится максимальной высотой дерева т.е. максимальной длиной слова

        (1 + 1) * max_wanted_length * ppn ;

        min(1) : 2 * ppn [Байт]
        (8 ядер)  = 16 [Байт]
        (64 ядер) = 128 [Байт]

        mid(255) : 2 * max_wanted_length * ppn = 2 * 255 * ppn [Байт]
        (8 ядер)  = 4080 [Байт]  = 3,98 [КБайт]
        (64 ядра) = 32640 [Байт] = 31,87 [КБайт]

        max(2^32-1) : 2 * max_wanted_length * ppn = 2 * 4 294 967 295 * ppn [Байт]
        (8 ядер)  = 4080 [Байт]  = 64 [ГБайт]
        (64 ядра) = 32640 [Байт] = 512 [ГБайт]
    
*/


    if (curr_len != wanted_length) {
        // Часть, задающая буквы для текущей глубины curr_len, растущей с 0 до wanted_length в каждом рекурсивном вызове
        unsigned short i, lb; // Переменные индексов циклов for

        /* Пересчитанный под количество итераций на глубине curr_len дробный chunk,
        домножаемый на rank для получения индекса в текущем множестве итераций,
        чтобы затем определить по нему индекс в цикле for от 0 до alphabet_length - 1;*/
        double chunk_resize; 
        
        if (leaf_reached)
            lb = 0;
        else {
            chunk_resize = chunk / pow(alphabet_length,  wanted_length - curr_len - 1);
            lb = (unsigned)(rank * chunk_resize) % alphabet_length;
        }

       
        // if (rank == root)
        //printf("new len = %u, rank = %4d, chunk = %llu, resize = %lf, lb = %llu\n", curr_len, rank, chunk, chunk_resize, lb);

        // Проверка достижения последней итерации для процесса и проход по циклу с рекурсивным вызовом
        for (i = lb; i < alphabet_length && perm_running < chunk; i++) {

            CLI[curr_len] = i;
            leaf_reached = recursive_permutations(curr_len + 1, leaf_reached);

            #ifdef STOP_ON_FIRST
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
        // Вывод первого и последнего вариантов слова одного или нескольких процессов на экран, если не используются замеры производительности
            if ((perm_running == 1 || perm_running == chunk)) {
                if (rank == wanted_rank) {
                    printf("rank %4d, pwd = %*s, perm = %llu\n", rank, max_wanted_length, current_line, perm_running);
                } else if (wanted_rank == -1) {
                    printf("rank %4d, pwd = %*s, perm = %llu\n", rank, max_wanted_length, current_line, perm_running);
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
            {
                // Поднятие флага после первой найденой коллизии
                if (collision_found == 0)  {
                    collision_time_stamp = MPI_Wtime();
                    collision_found = 1;
                }
                if (wanted_rank < commsize) {
                    printf("\n");
                    PrintTime(collision_time_stamp - global_time_start);
                    printf(" rank = %-4d, perm = %llu / %llu , collision with \"%s\"\n\n", rank, perm_running, chunk, current_line);
                } else {
                    PrintTime(collision_time_stamp - global_time_start);
                    printf(" rank = %-4d, perm = %llu / %llu , Collision with \"%s\"\n",   rank, perm_running, chunk, current_line);
                }
                
            }
            #else
                // Поднятие флага и замер времени
                if (collision_found == 0) {
                    #ifdef BENCHMARK_FIRST_COLLISION
                        collision_time_stamp = MPI_Wtime();
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
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }

            #else
                // Обнуление счётчика для перезаписи первых добавленных коллизий
                collision_counter = 0;
                // Оповещение о переполнение массива, означающее что количество будет равно MAX_COLLISIONS
                collision_overflow = 1;
            #endif
            }

            strcpy(collisions[collision_counter++], current_line);

        }

        #ifdef STOP_ON_FIRST
            // Синхронизация потоков по номеру пермутации, если она кратна константе и не больше последней:
            if ( !(perm_running % SYNC_CONST) && (perm_running < chunk) ) {
                MPI_Allreduce(&collision_found, &collision_counter_reduce, 1, MPI_UNSIGNED_CHAR, MPI_SUM, MPI_COMM_WORLD);
                /* printf("rank = %4d, perm = %llu, collision_counter_reduce = %d, collision_found = %d\n",
                    rank, perm_running, collision_counter_reduce, collision_found); */
                if (collision_counter_reduce) {
                    // printf("Exiting process, rank %4d, perm %llu\n", rank, perm_running);
                    return 2;
                    }
            }
        #endif

    }
    return 1; // Отправляет в leaf_reached предыдущего вызова функции 1,
    // оповещая о достижении листа дерева итераций (нахождении первого слова требуемой длины)
}

// Проверка слова с конца списка на коллизию хэша по порядковому номеру с нулевого.
// Выполняется по одному разу на процесс, поэтому не требует оптимизации по нескольким переборам
// Перебор Всего perms_curr % commsize раз.

/* Использует глобальные значения :
    unsigned alphabet_length
    unsigned wanted_length
    unsigned char md5wanted[MD5_DIGEST_LENGTH]
    char* collisions[MAX_COLLISIONS][LINE_ZIZE] || char** collisiions
 */
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
    } while (num > 0);

    i++;
    for (int j = 0; j < i; j++) {
        current_line[j] = alphabet[alphabet_length - 1];
    }

    #ifndef BENCHMARK   
        if (rank == wanted_rank) {
            printf("rank %4d, pwd = %*s, perm = %llu <== remainder\n", rank, max_wanted_length, current_line, perm_running);
        } else if (wanted_rank < 0) {
            printf("rank %4d, pwd = %*s, perm = %llu <== remainder\n", rank, max_wanted_length, current_line, perm_running);
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
            printf("Collision with \"%s\", rank = %4d, perm = %llu / %llu\n", current_line, rank, perm_running, chunk);
        #else
            // Замер времени нахождения первой коллизии
            if (collision_found == 0) {
                collision_time_stamp = MPI_Wtime();
            }
        #endif 

        // Если переполнение массива коллизий

        if ( (collision_counter) && ( (collision_counter) % MAX_COLLISIONS) == 0 ) {
        #ifdef MALLOC
            // Динамическое расширение и копирование всех предыдущих коллизий
            if (expand_cstring_array(&collisions, max_wanted_length, collision_counter, collision_counter + MAX_COLLISIONS)){
                printf("ERROR! Couldn't allocate memory for new collisions array size!\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

        #else
            // Обнуление счётчика для перезаписи первых добавленных коллизий
            collision_counter = 0;
            // Оповещение о переполнение массива, означающее что количество будет равно MAX_COLLISIONS
            collision_overflow = 1;
        #endif
        }

        strcpy(collisions[collision_counter++], current_line);

        // Поднятие флага после найденой коллизии
        collision_found = 1;
    }
    return;
}

/*
*
*
* * * * * * * * * * * * *
*   Основная программа  *
* * * * * * * * * * * * *
*
*
*/

int main(int argc, char **argv) {
    
    MPI_Init(NULL,NULL); 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);

    if (commsize == 1) {
        printf("Error, trying to execute single process version of MPI program!\nUse binary for serial version instead!\n\n");
        MPI_Finalize();
        return 1;
    }

    // Указатель на массив всех найденных на процессах коллизий - результат работы функции
    char** result_collisions_array = NULL;
    int root = 1;

    // Переменные внешнего цикла while, способные осущесвлять перебор в прямом и обратном порядке длин слов,
    // в зависимости от входных аргументов :
    int increment = 1; // Инкремент

    #ifdef BENCHMARK
        // Локальные версии переменных
        #ifndef MALLOC
        unsigned max_wanted_length; // Маскимальный размер слова при поиске коллизий,   задаётся аргументом main
        #endif
        unsigned min_wanted_length; // Минимальный размер слова при поиске коллизий,    задаётся аргументом main
    #endif

    // Обработка входных аргументов в нулевом процессе по стандарту MPI
    if (rank == root) {
        #ifndef BENCHMARK
            if (argc > 5 || argc < 4) {
                printf("Format: \"MD5 Hash\" \"Alphabet\" \"Word lenght\" \"Output Rank with number N (negative to output all; none to disable)\"\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            } else if (argc == 4) {
                wanted_rank = ~(1 << 31);
            } else wanted_rank = strtol(argv[4], NULL, 10); //atoi(argv[4]);
    
        #else
            if (argc != 4 ) {
                printf("Format: \"MD5 Hash\" \"Alphabet\" \"Word lenght\" \n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        #endif

        md5str_to_md5(md5_wanted, argv[1]); // Нахождение 128 битного хэша по входной 256 битной строке

        #ifdef MALLOC
        // Динамическое аллоцирование алфавита подсчитанной функцией длины
        parse_alphabet_malloc(argv[2], &alphabet, &alphabet_length);
        #else
        parse_alphabet(argv[2], alphabet, &alphabet_length);
        #endif

        if (alphabet_length == 0) {
            printf("ERROR! Invalid alphabet sectioning!\
            \nSection should look like :\ta-z,\
            \n(multiple sections in a row are allowed, commas aren't included into alphabet)\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }

        if (parse_arg_to_unsigned(argv[3],'-',&min_wanted_length,&max_wanted_length))
        {
            printf("ERROR! Invalid max-min length argument!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
    }

    /*
    *
    *
    * * * * * * * * * * * * * * * * * * *
    *    Рассылка аргументов запуска    *
    * * * * * * * * * * * * * * * * * * *
    *
    *
    */
   
    MPI_Bcast(&alphabet_length, 1, MPI_UNSIGNED_CHAR, root, MPI_COMM_WORLD);

    #ifdef MALLOC
        if (rank != root)
            alphabet = malloc(sizeof(char) * alphabet_length + 1);
    #endif

    MPI_Bcast(alphabet, alphabet_length + 1, MPI_CHAR, root, MPI_COMM_WORLD);
    MPI_Bcast(&md5_wanted, MD5_DIGEST_LENGTH , MPI_CHAR, root, MPI_COMM_WORLD);
    MPI_Bcast(&max_wanted_length, 1, MPI_UNSIGNED, root, MPI_COMM_WORLD);
    MPI_Bcast(&min_wanted_length, 1, MPI_UNSIGNED, root, MPI_COMM_WORLD);
    


    
    #ifndef BENCHMARK
        MPI_Bcast(&wanted_rank, 1, MPI_INT, root, MPI_COMM_WORLD);
    #endif


    // Вывод информации перед запуском:
    // Здесь выводятся входные параметры и предсказывается количество пермутаций для каждой итерации главного цикла по длине слов и по процессам
    #ifndef JUST_DO_IT
    if (rank == root) {

        printf("Alphabet[%u] : %s | Word length: %u-%u | N processes = %d\n", 
        alphabet_length, argv[2], min_wanted_length, max_wanted_length, commsize);
        printf("full alphabet : %s\n", alphabet);
        printf("md5 to bruteforce: ");
        md5_print(md5_wanted);
        printf("\n\n");
        print_perms_info(alphabet_length, min_wanted_length, max_wanted_length, commsize);
        unsigned long long geometric;
        if ( (geometric = geometric_series_sum(alphabet_length,min_wanted_length, max_wanted_length)) == 0 )
            geometric = geometric_series_sum(alphabet_length,max_wanted_length, min_wanted_length);
        printf("\n\tGeometric progression sum : %llu\n", geometric);


        #ifdef STOP_ON_FIRST
            printf("\nUSING \"STOP_ON_FIRST\" (each %d permutations)\n", SYNC_CONST);
            #ifndef BENCHMARK_SEPARATE_LENGTHS
                printf("\n--------------------------------\n");
            #endif
        #endif

        #ifndef BENCHMARK
            printf("\nCollisions :\n\n");
        #endif
    }
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

        CLI = malloc(sizeof(unsigned) * max_wanted_length);
        collisions = malloc(sizeof(char*) * MAX_COLLISIONS);
        for (int i = 0; i < MAX_COLLISIONS; i++)
            collisions[i] = (char*)malloc(sizeof(char) * (max_wanted_length + 1));
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

    unsigned long long perm_sum  = 0; // Счётчик общего количества переборов
    unsigned rank_iter = 0; // Итератор для цикла по ранкам процессов для случая пропусков вычислений

    // Работа алгоритма пропусков вычислений:
    // хранение номера последнего использованного процесса 'rank_iter' позволяет загружать работой
    // всё новые и новые неиспользованные процессы, равномерно загружая их В ПРЕДЕЛАХ алгоритма пропусков.
    // Количество пропущенных итераций не учитывается в других секциях программы для оптимизации загрузки процессов.
    

    MPI_Barrier(MPI_COMM_WORLD);

    // Переменные замера производительности рекурсивной функции для каждого процесса

    #ifdef BENCHMARK
        double time_sum = 0.0; 
        double time_single;
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
        global_time_start = MPI_Wtime();
    #endif

    do {

        #ifdef BENCHMARK
            time_single = MPI_Wtime();
        #endif

        perm_running = 0;

        int perms_curr = pow(alphabet_length, wanted_length);
        chunk = perms_curr / commsize;
        if (chunk != 0) {
            recursive_permutations(0,0);
            if (perms_curr % commsize > rank)
                check_remainder(rank);
        } else {
            // Случай пропуска вычислений процессами:
            chunk = 1;
            if (rank >= rank_iter && rank < perms_curr + rank_iter) {
                recursive_permutations(0, 0);
            }
            rank_iter = (perms_curr + rank_iter) % commsize;
        }

        #ifdef BENCHMARK

            #ifdef BENCHMARK_FIRST_COLLISION
                time_single_end = MPI_Wtime();
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
                time_single = MPI_Wtime() - time_single;
                time_sum += time_single;
            #endif

        #endif
        #ifdef BENCHMARK_SEPARATE_LENGTHS
            times_single[wanted_length - min_wanted_length] = time_single;
        #endif
        #ifdef BENCHMARK_TOTAL_PERMS
            time_sums[wanted_length - min_wanted_length] = time_sum;
        #endif

        perm_sum += perm_running;   // Суммарное количество пермутаций носит лишь информативный характер, исключено из замеров времени,

        wanted_length += increment; // как и операции сравнения во внешнем цикле

        //printf("rank = %4d, PR = %llu, PS = %llu\n", rank, perm_running, perm_sum);
    }
    // Использование доп. условия на выход из цикла для случая синхронизации
    #ifdef STOP_ON_FIRST
        while ((wanted_length <= max_wanted_length && wanted_length >= min_wanted_length) && !collision_counter_reduce);

    #else
        while (wanted_length <= max_wanted_length && wanted_length >= min_wanted_length);
    #endif



    /*
    *
    *
    * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    * Рассылка всех собранных коллизий в массив коллизий корневого процесса root  *
    * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    *
    *
    */

    #ifdef BENCHMARK
    double time_gather = MPI_Wtime(); // Переменная замера времени до завершения обхода дерева всеми процессами
    #endif

    // Определение общего числа коллизий из всех процессов в collision_counter_reduce
    MPI_Allreduce(&collision_counter, &collision_counter_reduce, 1, MPI_UNSIGNED_CHAR, MPI_SUM, MPI_COMM_WORLD);

    // printf("rank = %4d, perm = %llu, collision_counter_reduce = %d, collision_found = %d\n", rank, perm_running, collision_counter_reduce, collision_found);
    

    // P.S. При включенной синхронизации STOP_ON_FIRST после нахождения первой коллизии остальные процессы
    // могут успеть обнаружить коллизии до следующей кратной SYNC_CONST итерации, следовательно все проверки актуальны

    if (collision_counter_reduce)
    {

        unsigned char initiate_exchange = 1;
        if( rank == root && collision_counter == collision_counter_reduce) {
            initiate_exchange = 0;
        }
        MPI_Bcast(&initiate_exchange, 1, MPI_UNSIGNED_CHAR, root, MPI_COMM_WORLD);
        if (initiate_exchange == 0) {
            result_collisions_array = collisions;
        } else {

            // Приступить к передаче
            if (rank == root) printf("sending collisions to root...\n");

            unsigned char* counts = (unsigned char *)malloc(commsize*sizeof(unsigned char));
            int* recvcounts = (int *)malloc(commsize*sizeof(int));
            int* displs = (int *)malloc(commsize*sizeof(int));
            for (int i = 0; i < commsize; i++)
            {
                recvcounts[i] = 1;
                displs[i] = i;
            }
        
            MPI_Gatherv(&collision_counter, 1, MPI_UNSIGNED_CHAR, 
            counts, recvcounts, displs, MPI_UNSIGNED_CHAR,
            root, MPI_COMM_WORLD);
            free(recvcounts);
            free(displs);

            /*
            printf("rank %d count %u\n", rank, collision_counter);
            MPI_Barrier(MPI_COMM_WORLD);
            if (rank == root)
            {
                for (int i = 0; i < commsize; i++){
                    printf("%u", counts[i]);
                }
                printf("\n");
            }
            */

            if (rank == root) {

            #ifdef MALLOC

                /*   -------- свободное место корневого процесса --------    -------- коллизии других процессов ---------  */    
                if ( MAX_COLLISIONS - (collision_counter % MAX_COLLISIONS) < collision_counter_reduce - collision_counter ) {
                    
                    result_collisions_array = malloc(sizeof(char*) * collision_counter_reduce);

                    if (result_collisions_array == NULL) {
                        printf("\nERROR! Couldn't allocate pointer to resulting collisions array!\n");
                        MPI_Abort(MPI_COMM_WORLD, 1);
                    }
                    for (int i = 0; i < collision_counter_reduce; i++) {

                        result_collisions_array[i] = (char*)malloc(sizeof(char) * (max_wanted_length + 1));
                        if (result_collisions_array[i] == NULL) {
                            printf("\nERROR! Couldn't allocate char array for resulting collisions array!\n");
                            MPI_Abort(MPI_COMM_WORLD, 1);
                        }

                        if (i < collision_counter)
                            strcpy(result_collisions_array[i], collisions[i]);
                    }
                } else {
                    result_collisions_array = collisions;
                }

            #else
                result_collisions_array = malloc(sizeof(char*) * collision_counter_reduce);
                if (result_collisions_array == NULL) {
                    printf("\nERROR! Couldn't allocate pointer to resulting collisions array!\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
                for (int i = 0; i < collision_counter_reduce; i++) {
                    result_collisions_array[i] = (char*)malloc(sizeof(char) * (max_wanted_length + 1));
                    if (result_collisions_array[i] == NULL) {
                        printf("\nERROR! Couldn't allocate char array for resulting collisions array!\n");
                        MPI_Abort(MPI_COMM_WORLD, 1);
                    }

                    if (i < collision_counter)
                        strcpy(result_collisions_array[i], collisions[i]);
                }
            #endif

                // Цикл приёма коллизий от процессов их содержащих
                for (int i = 0; i < commsize; i++){
                    if (counts[i] && i != root) {
                        for (int j = 0; j < counts[i]; j++) {
                            MPI_Recv(result_collisions_array[collision_counter + j], max_wanted_length + 1, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        }
                    }
                }
                // printf("gathered result_collisions_array: ");
                // for (int i = 0; i < collision_counter_reduce; i++)
                //     printf("%s ", result_collisions_array[i]);
                // printf("\n");

            } else {
                // Отправка ряда коллизий процесса в массив коллизий корневого
                for (int i = 0; i < collision_counter; i++) {
                    MPI_Send(collisions[i], max_wanted_length + 1, MPI_CHAR, root, 0, MPI_COMM_WORLD);
                }
            }
            free(counts);
        } 
    }

    #ifdef BENCHMARK
        // Программа завершила работу после того, как определила общее число коллизий совершив все возможные варианты перебора
        time_gather = MPI_Wtime() - time_gather;
        double time_complete = time_gather + time_sum;
        // printf ("gather time: %lf\n", time_complete);
    #endif

    /*
    *
    *
    * * * * * * * * * * * * * * * * *
    *   Вывод результатов поисков   *
    * * * * * * * * * * * * * * * * *
    *
    *
    */

    #ifndef JUST_DO_IT

    MPI_Barrier(MPI_COMM_WORLD);

    if (collision_counter_reduce == 0) {

        if (rank == root) {
            #ifdef BENCHMARK
            printf("\n");
            #endif
            printf("-------------------------------------------\n");
            printf("      ;C   no collisions found    >;\n");
            printf("-------------------------------------------\n");
        }
    } else {
    
        if (rank == root) {
            printf("\n");
            printf("-------------------------------------------\n");
            printf(">>>----->  %d collision(s) found  <-----<<<\n", collision_counter_reduce);
            printf("-------------------------------------------\n");

            if(collision_counter_reduce != collision_counter)
                printf("\nCollisions arrays : \n");
        }

        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Recv(NULL, 0, MPI_INT, (rank > 0) ? rank - 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  
        if (collision_found && rank != root) {
            printf("rank %-4d size %3d  { %s", rank, collision_counter, collisions[0]);
            for (int j = 1; j < collision_counter; j++)
                printf(", %s", collisions[j]);
            printf(" }\n");
        }
        MPI_Send(NULL, 0, MPI_INT, rank != commsize - 1 ? rank + 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD);

        MPI_Barrier(MPI_COMM_WORLD);

        if (rank == root) {
            printf("\nCollisions gathered in :\nroot %-4d size %3d  { %s", rank, collision_counter_reduce, result_collisions_array[0]);
            for (int j = 1; j < collision_counter; j++)
                printf(", %s", result_collisions_array[j]);
            printf(" }\n");
        }

    }
    #endif

    MPI_Barrier(MPI_COMM_WORLD);
    fflush(stdout);
    if (rank == 0) {
        printf("\nPermutations executed:\n");
    }
    //printf("rank %d\n", rank);
    char* message = malloc(sizeof(char)*100);
    sprintf(message,"%llu (last) / %llu (total)", perm_running, perm_sum);
    // printf("rank = %d, %s\n", rank, message);
    MPI_Print_in_rank_order_unique(commsize, rank, message);
    free(message);

    /*
    *
    *
    * * * * * * * * * * * * * * * *
    *   Обработка времен запуска  *
    * * * * * * * * * * * * * * * *
    *
    *
    */

    #ifdef BENCHMARK
    {

        double reduced_time = 0, time_min, time_max, time_avg;
        if (max_wanted_length == 1)
            MPI_Allreduce(&reduced_time, &time_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        #ifdef TIME_CHECK

            MPI_Barrier(MPI_COMM_WORLD);
            char* msg = malloc(sizeof(char)*100);
            if (rank > FIRST_RANK_TO_CHECK && rank < FIRST_RANK_TO_CHECK + N_CHECK) {
                sprintf(msg,"Execution time on process %d: %lf\n", rank, time_sum);
            }
            free(msg);

        #endif


        unsigned curr_length;
        unsigned counter_start, counter;
        if (increment == 1) {
            counter_start = wanted_length - min_wanted_length;
        } else {
            counter_start = max_wanted_length - wanted_length;
        }


        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == root){
            printf("Execution times statistics (in seconds)\n");
            printf("processes = %d, alphabet length = %u, for word lengths (WL) :\n", commsize, alphabet_length);
            printf("\t\tmin\t\tmax\t\tmean\n");
        }
        

        #ifdef BENCHMARK_SEPARATE_LENGTHS
        counter = counter_start;
        if (increment == 1) {
            curr_length = min_wanted_length;
        } else {
            curr_length = max_wanted_length;
        }
        // Проходим по массиву замеров для разных длин слова
        while (counter--) {
            reduced_time = times_single[curr_length - min_wanted_length];
        
            MPI_Allreduce(&reduced_time, &time_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
            MPI_Allreduce(&reduced_time, &time_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
            MPI_Allreduce(&reduced_time, &time_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
            
            if (rank == root) {
               
                printf("WL %3u%5s", curr_length," ");
                printf("\t%.*lf", 6, time_min);
                printf("\t%.*lf", 6, time_max);
                time_avg /= commsize;
                printf("\t%.*lf\n", 6, time_avg);
            }

            curr_length += increment;
        } // Конец for
        if (rank == root) printf("\n");
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
            reduced_time = time_sums[curr_length - min_wanted_length];
            MPI_Allreduce(&reduced_time, &time_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
            MPI_Allreduce(&reduced_time, &time_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
            MPI_Allreduce(&reduced_time, &time_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
            if (rank == root) {
                printf("WL %3u-%-3u", first_length, curr_length);
                printf("\t%.*lf", 6, time_min);
                printf("\t%.*lf", 6, time_max);
                time_avg /= commsize;
                printf("\t%.*lf\n", 6, time_avg);
            }
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
        if (rank == root) printf("\nWL ");
        while (counter--) {
            reduced_time = time_sums[curr_length - min_wanted_length];
            if (rank == root) {
                printf("%u-%u ", first_length, curr_length);
            }
            curr_length += increment;
        }
        counter = counter_start;
        curr_length = curr_length_start;
        if (rank == root) printf("\nP%dmin ", commsize);
        while (counter--) {
            reduced_time = time_sums[curr_length - min_wanted_length];
            MPI_Allreduce(&reduced_time, &time_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
            if (rank == root) {
                printf("%.*lf ", 6, time_min);
            }
            curr_length += increment;
        }
        counter = counter_start;
        curr_length = curr_length_start;     
        if (rank == root) printf("\nP%dmax ", commsize);   
        while (counter--) {
            reduced_time = time_sums[curr_length - min_wanted_length];
            MPI_Allreduce(&reduced_time, &time_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
            if (rank == root) {
                printf("%.*lf ", 6, time_max);
            }
            curr_length += increment;
        }
        counter = counter_start;
        curr_length = curr_length_start;     
        if (rank == root) printf("\nP%davg ", commsize);  
        while (counter--) {
            reduced_time = time_sums[curr_length - min_wanted_length];
            MPI_Allreduce(&reduced_time, &time_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
            if (rank == root) {
                time_avg /= commsize;
                printf("%.*lf ", 6, time_avg);
            }
            curr_length += increment;
        }
        #endif

        #else
        reduced_time = time_sum;
        MPI_Allreduce(&reduced_time, &time_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
        MPI_Allreduce(&reduced_time, &time_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        MPI_Allreduce(&reduced_time, &time_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

        if (rank == root) {
            printf("WL %3u-%-3u", min_wanted_length, max_wanted_length);
            printf("\t%.*lf", 6, time_min);
            printf("\t%.*lf", 6, time_max);
            time_avg /= commsize;
            printf("\t%.*lf\n", 6, time_avg);
        }
        #endif
        if (rank == root) {
            printf("\nProgram finished after (seconds)\n");
            printf("\t\tmin\t\tmax\t\tmean\n");
        }
        MPI_Allreduce(&time_sum, &time_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
        MPI_Allreduce(&time_sum, &time_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(&time_sum, &time_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        if (rank == root) {
            printf("Search    \t%.*lf", 6, time_min);
            printf("\t%.*lf", 6, time_max);
            time_avg /= commsize;
            printf("\t%.*lf\n", 6, time_avg);
        }
        MPI_Allreduce(&time_gather, &time_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
        MPI_Allreduce(&time_gather, &time_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(&time_gather, &time_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        if (rank == root) {
            printf("Gatherv  \t%.*lf", 6, time_min);
            printf("\t%.*lf", 6, time_max);
            time_avg /= commsize;
            printf("\t%.*lf\n", 6, time_avg);
        }
        MPI_Allreduce(&time_complete, &time_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
        MPI_Allreduce(&time_complete, &time_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(&time_complete, &time_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        if (rank == root) {
            printf("Total    \t%.*lf", 6, time_min);
            printf("\t%.*lf", 6, time_max);
            time_avg /= commsize;
            printf("\t%.*lf\n", 6, time_avg);
        }
    }

        #ifdef BENCHMARK_FIRST_COLLISION

        if (rank == root) {
            if (collision_counter_reduce == 0)
                printf("\nFirst collision isn't found.\n");
            else
                printf("\n");
        }

        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Recv(NULL, 0, MPI_INT, (rank > 0) ? rank - 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  
        if (first_collision_measured) {
                
            printf("First collision on process %d found after %lf sec.\n", rank, first_collision_time);
        }
        MPI_Ssend(NULL, 0, MPI_INT, rank != commsize - 1 ? rank + 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD);
        if (rank == root)
            printf("\n");

        #endif

    #endif

    if (rank != root) {

        #ifdef MALLOC
        
            for(int i = 0; i < (collision_counter / MAX_COLLISIONS) + 1; i++)
            {
                free(collisions[i]);
            }
            free(collisions);
            free(CLI);
            free(alphabet);

        #endif

        free(result_collisions_array);
    }


    /*  DO SOMETHING WITH RESULTING ARRAY ON ROOT  */


    #ifdef MALLOC

        if (rank == root) { 
            free(CLI);
            free(alphabet);
        }

    #endif

    if (rank == root && collision_counter_reduce) {
        for(int i = 0; i < (collision_counter / MAX_COLLISIONS) + 1; i++)
        {
            free(result_collisions_array[i]);
        }
    }
    free(result_collisions_array);

    MPI_Finalize();

    return 0;
}