#include <string.h>


// Предсказание количества пермутаций для каждой итерации главного цикла по длине слов для каждого процесса
void print_perms_info(unsigned alphabet_length, unsigned max_wanted_length, int commsize){

    printf("Permutations expected:\n");
    unsigned long long int *perms, *total_perms;

    total_perms = malloc(sizeof(unsigned long long int)*(commsize+1));
    perms = malloc(sizeof(unsigned long long int)*commsize);

    for (int i = 0; i < commsize + 1; i++)
        total_perms[i] = 0;

    for (int i = 1; i <= max_wanted_length; i++)
    {
        unsigned long long n = (int)pow(alphabet_length, i);
        unsigned long long chunk = n / commsize;
        for(int j = 0; j < commsize; j++){
            perms[j] = (n % commsize > j) ? chunk + 1 : chunk;
            total_perms[j] += perms[j];
            total_perms[commsize] += perms[j];
        }
        printf("\n  Line length : %d\n\n",i);
        for(int j = 0; j < commsize; j++){
            int count = 0;
            while(perms[j] == perms[j+1])
            {
                count++;
                j++;
            }
            if(count == 0)
                printf("\tprocess %d :\t%lld\n", j, perms[j]);
            else
                printf("\tprocess %d-%d :\t%lld\n", j-count, j, perms[j]);
        }
        
        char total[1000];
        sprintf(total, "total          %lld\n",n);
        {
            char delim[1000];
            int i = 0;
            while(i < strlen(total))
                delim[i++] = '-';
            delim[i-1] = '\0';
            printf("\t%s\n\t%s", delim, total);
        }
        
    }

    printf("\n  Total :\n\n");
    for(int j = 0; j < commsize; j++){
            int count = 0;
            while(total_perms[j] == total_perms[j+1])
            {
                count++;
                j++;
            }
            if(count == 0)
                printf("\tprocess %d :\t%lld\n", j, total_perms[j]);
            else
                printf("\tprocess %d-%d :\t%lld\n", j-count, j, total_perms[j]);
        }
        char total[1000];
        sprintf(total, "total          %lld",total_perms[commsize]);
        {
            char delim[1000];
            int i = 0;
            while(i < strlen(total))
                delim[i++] = '-';
            delim[i-1] = '\0';
            printf("\t%s\n\t%s\n", delim, total);
        }
}
