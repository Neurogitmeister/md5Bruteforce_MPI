/* * * * * * * * * * * * * * * * * * * * *
*           Флаги компиляции.            *
*   Регулируют наличие участков кода.    *
* * * * * * * * * * * * * * * * * * * * */

#define PARALLEL



// Выход из цикла перебора после нахождения первой коллизии (или после итерации, кратной SYNC_CONST для параллельной версии)
// #define STOP_ON_FIRST



// Позволяет использовать динамическую память для массива индексов букв, массива алфавита и массива коллизий
// #define MALLOC



// Использование замеров производительности, исключение вывода на экран на участках замера
#define BENCHMARK



/* * * * * * * * * * * * * * * * * * * */ #ifdef BENCHMARK /* * * * * * * * * * * * * * * * * * * * * */ 

// Вывод на экран времени нахождения первой коллизии

#define BENCHMARK_FIRST_COLLISION



// Выводит статистики времен по каждой отдельной длине слова

// #define BENCHMARK_SEPARATE_LENGTHS



// Вывод статистики по проверенным коллизиям с начальной длины до всех остальных 
// (например для начальной длины 12 : 12-12, 12-13, 12-14, ... , и т.д.)

#define BENCHMARK_TOTAL_PERMS



// Позволяет выводить на экран время выполнения рекурсивной функции первых N_CHECK c FIRST_RANK_TO_CHECK процессов

#define TIME_CHECK



// Я сам всё знаю, дайте мне лишь время, ну и перестановок отсыпьте

// #define JUST_DO_IT


/* * * * * * * * * * * * * * * * * * * * * */  #endif  /* * * * * * * * * * * * * * * * * * * * * * * */ 




/* * * * * * * * * * * * * * * * * * * * * *
*               Константы                  *
* * * * * * * * * * * * * * * * * * * * * */



// Число перестановок до проверки (синхронизации) на наличие коллизий в процессах
#define SYNC_CONST 50


// Номер первого процесса из списка на вывод замеров времени выполнения основного цикла программы
#define FIRST_RANK_TO_CHECK 4


// Число процессов для выведения замеров времени выполнения основного цикла программы
#define N_CHECK 128


// Номер процесса, чьи граничные перестановки букв в слове  для каждой итерации по длине слова требуется отобразить  (-1 : вывод всех)
#define RANK_CHECK -2


// Размер массива коллизий, хранящийся каждым процессом
#define MAX_COLLISIONS 8


// Мощность алфавита для перебора
#define ALPH_SIZE 150


// Максимальный размер обрабатываемого слова
#define LINE_SIZE 50