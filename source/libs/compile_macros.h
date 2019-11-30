#include <openssl/md5.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "arguments_parse.h"
#include "expand_cstring_array.h"
#include "combinatory_userlib.h"
#include "md5_userlib.h"

/* * * * * * * * * * * * * * * * * * * * *
*           Флаги компиляции.            *
*   Регулируют наличие участков кода.    *
* * * * * * * * * * * * * * * * * * * * */

#define PARALLEL

// Выход из цикла перебора после нахождения первой коллизии (или после итерации, кратной SYNC_CONST для параллельной версии)
// #define STOP_ON_FIRST

// Позволяет использовать динамическую память для массива индексов букв, массива алфавита и массива коллизий
#define MALLOC

// Использование замеров производительности, исключение вывода на экран на участках замера
// #define BENCHMARK

#ifdef BENCHMARK

    #define BENCHMARK_FIRST_COLLISION
    // Замеряет и выводит статистику по каждой длине слова
    #define BENCHMARK_SEPARATE_LENGTHS

    // Замеряет и выводит статистику по всем проверенным коллизиям с минимальной по каждую длину слова
    #define BENCHMARK_TOTAL_PERMS

    // Позволяет выводить на экран время выполнения рекурсивной функции на первых N_CHECK процессах
    // #define TIME_CHECK

    // Я сам всё знаю, дайте мне на экран лишь время и количества перестановок
    #define JUST_DO_IT

#endif

/* * * * * * * * * * * * * * * * * * * * * *
*               Константы                  *
* * * * * * * * * * * * * * * * * * * * * */

// Число перестановок до проверки (синхронизации) на наличие коллизий в процессах
#define SYNC_CONST 50

// Число процессов для выведения отдельных замеров времени
#define N_CHECK 128

// Номер процесса, чьи крайние пермутации букв требуется отследить для каждой итерации по длине слова (-1 : вывод всех)
#define RANK_CHECK -2

// Размер массива коллизий, хранящийся каждым процессом
#define MAX_COLLISIONS 8

#ifndef MALLOC
// (используются в статически аллоцированных массивах)

    // Мощность алфавита для перебора
    #define ALPH_SIZE 150

    // Максимальный размер обрабатываемого слова
    #define LINE_SIZE 50

#endif

/* * * * * * * * * * * * * * * * * * * * * *
*                                          *
*    Определение глобальных переменных     *
*   исходя из выбранных фагов компиляции   *
*                                          *
*   ! НЕ РЕКОММЕНДУЕТСЯ РЕДАКТИРОВАТЬ !    *
*                                          *
* * * * * * * * * * * * * * * * * * * * * */

#if  !defined(MEASURE_TIME) && defined(PARALLEL)

    int wanted_rank = RANK_CHECK; // Ранк, варианты перестановок которого требуется отобразить,      задаётся аргументом main

    // Глобальные версии переменных для использования форматирования printf во время работы рекурсивного перебора
    unsigned min_wanted_length; // Минимальный размер слова при поиске коллизий,   задаётся аргументом main

#endif

#if  defined(MALLOC) && defined(PARALLEL)
    unsigned max_wanted_length; // Маскимальный размер слова при поиске коллизий,   задаётся аргументом main
#endif


#ifdef MALLOC

    unsigned* CLI;  // (Current Letter Index) Массив индексов букв для текущей перестановки букв
    char* alphabet; // Алфавит,                                                         задаётся аргументом main и обрабатывается
    char** collisions = NULL; // Массив для хранения MAX_COLLISIONS или одной последних коллизий

#else

    char alphabet[ALPH_SIZE];
    unsigned CLI[LINE_SIZE];
    char collisions[LINE_SIZE][MAX_COLLISIONS]; // Массив для хранения MAX_COLLISIONS_PER_RPC последних коллизий
    char collision_overflow = 0;                // Флаг переполнения массива коллизий

#endif

unsigned char md5_wanted[MD5_DIGEST_LENGTH]; // Хеш искомой функции,                    задаётся аргументом main
unsigned alphabet_length;           // Мощность полученного алфавита
unsigned wanted_length;             // Текущая длина искомой строки
unsigned long long perm_running = 0;// Счетчик пермутаций (перестановок)
unsigned char collision_counter = 0;// Счетчик найденных коллизий
unsigned char collision_found = 0;
double collision_time_stamp = 0.0;

#ifdef PARALLEL

unsigned long long chunk;           // Длина блока вычислений для процесса
unsigned char collision_counter_reduce = 0;          // Сумма флагов коллизий всех процессов

int commsize;                       // Число процессов 
int rank;                           // Номер данного процесса

#endif


