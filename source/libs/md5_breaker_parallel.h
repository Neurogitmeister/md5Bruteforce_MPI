#include <openssl/md5.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "arguments_parse.h"
#include "expand_cstring_array.h"
#include "combinatory_userlib.h"
#include "md5_userlib.h"

#include "compilation_info.h"


#ifndef BENCHMARK
    #undef BENCHMARK_FIRST_COLLISION
    #undef BENCHMARK_SEPARATE_LENGTHS
    #undef BENCHMARK_TOTAL_PERMS
    #undef JUST_DO_IT
#endif


/* * * * * * * * * * * * * * * * * * * * * *
*                                          *
*    Определение глобальных переменных     *
*   исходя из выбранных флагов компиляции   *
*                                          *
*   ! НЕ РЕКОММЕНДУЕТСЯ РЕДАКТИРОВАТЬ !    *
*                                          *
* * * * * * * * * * * * * * * * * * * * * */

#ifndef BENCHMARK
    int wanted_rank = RANK_CHECK; // Ранк, варианты перестановок которого требуется отобразить,      задаётся аргументом main
#endif

#ifdef MALLOC
    unsigned max_wanted_length; // Маскимальный размер слова при поиске коллизий,   задаётся аргументом main
#endif


#ifdef MALLOC

    char* alphabet;		// Алфавит,                                                     задаётся аргументом main и обрабатывается
	unsigned* CLI;		// (Current Letter Index) Массив индексов букв в алфавите для текущего слова (комбинации) для хэширования
	char* current_word; // Текущее слово (комбинация) для хэширования
    char** collisions  = NULL; // Массив для хранения MAX_COLLISIONS или одной последних коллизий

#else

    char alphabet[ALPH_SIZE];
    unsigned CLI [LINE_SIZE];
	char current_word[LINE_SIZE];
    char collisions[LINE_SIZE][MAX_COLLISIONS]; // Массив для хранения MAX_COLLISIONS последних коллизий
    char collision_overflow = 0;                // Флаг переполнения массива коллизий

#endif

unsigned char md5_wanted [MD5_DIGEST_LENGTH]; // Хеш искомой функции,                    задаётся аргументом main
unsigned char current_key[MD5_DIGEST_LENGTH]; // Хэш текущего слова
unsigned alphabet_length;           // Мощность полученного алфавита
unsigned wanted_length;             // Текущая длина искомой строки
unsigned long long perm_running = 0;// Счетчик пермутаций (перестановок)
unsigned char collision_counter = 0;// Счетчик найденных коллизий
unsigned char collision_found = 0;
double collision_time_stamp = 0.0;
double global_time_start = 0.0;

unsigned long long chunk;           // Длина блока вычислений для процесса
unsigned char collision_counter_reduce = 0;          // Сумма флагов коллизий всех процессов

int commsize;                       // Число процессов 
int rank;                           // Номер данного процесса



