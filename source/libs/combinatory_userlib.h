#include <string.h>
#include <math.h>

// Нахождение общего числа пермутаций приходящееся на один из commsize процессов с рангом rank
unsigned long long get_perm_total_for_rank(int alph_len, unsigned min_word_len, unsigned max_word_len, int commsize, int rank) {

    unsigned long long total_perms = 0;
    unsigned long long perms = 0;
    for (int i = min_word_len; i <= max_word_len; i++)
    {
        unsigned long long n = (int)pow(alph_len, i);
        unsigned long long chunk = n / commsize;
            perms = (n % commsize > rank) ? chunk + 1 : chunk;
            total_perms += perms;
    }
    return total_perms;
}

// Формула суммы геометрической прогресси с элемента номер m по элемент номер n из ряда для множителя base
unsigned long long geometric_series_sum(int base, unsigned m, unsigned n) {
    if(m < n)
        return (pow(base, m) - pow(base, n + 1)) / (1 - base);
    else
        return 0;
}

// Предсказание количества пермутаций для каждой итерации главного цикла по длине слов для каждого процесса
void print_perms_info(unsigned alphabet_length, unsigned min_length, unsigned max_length, int commsize) {

    printf("Permutations expected (in execution order) :\n");
    
    if(commsize == 1)  {
        // Последовательная версия
        printf("\n");
        unsigned long long total_perms = 0;
        if(min_length < max_length)
            for (unsigned i = min_length; i <= max_length; i++)
            {
                unsigned long long n = (int)pow(alphabet_length, i);
                total_perms += n;
                printf("\tLine length = %d : %lld\n", i, n);
            }
        else
            for (unsigned i = max_length; i <= min_length; i--)
            {
                unsigned long long n = (int)pow(alphabet_length, i);
                total_perms += n;
                printf("\tLine length = %d : %lld\n", i, n);
            }
        char total[1000];
        char delim[1000];
        int i = 0;
        sprintf(total, "total          %lld\n",total_perms);
        while(i < strlen(total))
            delim[i++] = '-';
        delim[i-1] = '\0';
        total[i] = '\0';
        printf("\t%s\n\t%s\n", delim, total);
        return;
    }

    // Многопроцессорна версия
    unsigned long long int *perms, *total_perms;
    char total[1000];
    char delim[1000];

    total_perms = malloc(sizeof(unsigned long long int)*(commsize+1));
    perms = malloc(sizeof(unsigned long long int)*commsize);
    for (int j = 0; j < commsize + 1; j++)
        total_perms[j] = 0;

    char len;
    unsigned char incr;
    // Переключение порядка рассчёта и вывода по диапазону
    if(min_length <= max_length) {
        len = min_length;
        incr = 1;
    } else {
        unsigned temp = min_length;
        min_length = max_length;
        max_length = temp;

        len = max_length;
        incr = -1;
    }
    while (len <= max_length && len >= min_length) {
        // Вычисления теоретического количества пермутаций под процесс
        unsigned long long n = (int)pow(alphabet_length, len);
        unsigned long long chunk = n / commsize;
        for(int j = 0; j < commsize; j++){
            perms[j] = (n % commsize > j) ? chunk + 1 : chunk;
            total_perms[j] += perms[j];
            total_perms[commsize] += perms[j];
        }
        
        printf("\n  Line length %d :\n", len);
        for(int j = 0; j < commsize; j++){
            int count = 0;
            while(perms[j] == perms[j+1])
            {
                count++;
                j++;
            }
            if(count == 0)
                printf("\tprocess %d \t%lld\n", j, perms[j]);
            else
                printf("\tprocesses %d-%d \t%lld\n", j-count, j, perms[j]);
        }
        // Формирорование строки вывода кол-ва всех текущих перестановок для текущей длины i
        sprintf(total, "total          %lld\n",n);

        // Формирование разделителя
        int i = 0;
        while(i < strlen(total))
            delim[i++] = '-';
        delim[i-1] = '\0';

        // Вывод разделителя и кол-ва перестановок
        printf("\t%s\n\t%s", delim, total);

        len+=incr;
    }

    // Подсчёт и вывод суммы итераций для каждого процесса, искоючая одинаковые результаты
    if(incr == 1)
        printf("\n  Line length %d-%d :\n", min_length, max_length);
    else
        printf("\n  Line length %d-%d :\n", max_length, min_length);
    for(int j = 0; j < commsize; j++){
        int count = 0;
        while(total_perms[j] == total_perms[j+1])
        {
            count++;
            j++;
        }
        if(count == 0)
            printf("\tprocess %d \t%lld\n", j, total_perms[j]);
        else
            printf("\tprocesses %d-%d \t%lld\n", j-count, j, total_perms[j]);
    }

    sprintf(total, "total          %lld",total_perms[commsize]);
    int i = 0;
    while(i < strlen(total))
        delim[i++] = '-';
    delim[i-1] = '\0';
    printf("\t%s\n\t%s\n", delim, total);

}
   