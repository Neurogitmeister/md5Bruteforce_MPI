#include <string.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <mpi.h>

#define BUFF_SIZE 50 // Максимальная длина обрабатываемой строки

typedef unsigned long long ull_t;
int alphabet_length, wanted_length; // Длина алфавита, длина искомой строки
int thf[BUFF_SIZE]; // Индексный массив
char *alphabet; // Алфавит поиска
char *wanted; // Искомая строка
unsigned char md5_input[MD5_DIGEST_LENGTH]; // Хеш искомой функции
static int count_perm = 0;
int isKey = 0;
ull_t start_comb, comb_per_proc;
int ind[256];
char *buffer_current_line;
int maximum_length;

size_t comb(char *, ull_t);

void setIndex() {
    // Формирование индексного массива символо для перебора
    for (int i = 0; i < alphabet_length; i++) {
        ind[alphabet[i]] = i;
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
        if (a[i] != md5_input[i]) return -1;
    return 0;
}

void repeat_permutations(int k) {
    if (k == wanted_length) {
        char current_line[wanted_length + 1];
        unsigned char cur_key[MD5_DIGEST_LENGTH];
        //Заполнение текущей строки
        int i;
        for (i = 0; i < wanted_length; i++) {
            current_line[i] = alphabet[thf[i]];
        }
        current_line[i] = '\0';

        MD5((const unsigned char *) current_line,
            strlen(current_line),
            (cur_key)
        );
        count_perm++; //счетчик перебранных вариантов

        /*printf("%s|%d\n", current_line, count_perm);
        printf("(%5d)[%2d] \t(%s)\t", (int) count_perm, (int) strlen(current_line), current_line);
        print(cur_key);
        printf(" ");
        print(md5_input);*/

        // compare - сравнение
        if (compare_hash(cur_key) == 0) {
            isKey = 1;
            print(cur_key);
            printf(" ");
            print(md5_input);
            printf(" <= Password is %s\n", current_line);
            //exit(0);
            // MPI_Abort(MPI_COMM_WORLD, 0);
        }

        //printf("\n");
    } else {
        for (int j = ind[buffer_current_line[k]]; j < alphabet_length && count_perm < comb_per_proc; j++) {
            thf[k] = j;
            // Уходим в рекурсию для 1 элемента
            repeat_permutations(k + 1);
        }
        buffer_current_line[k] = alphabet[0];
    }
}

long getSizePerm() {
    long count_permutations = 0;

    for (int i = 1; i <= wanted_length; i++)
        count_permutations += pow(alphabet_length, i);

    return count_permutations;
}

// определение Откуда начинать по номеру комбинации
size_t comb(char *str, ull_t combination) {
    ull_t sum = 0;
    ull_t cur_len = 0;
    for (ull_t i = 1; i <= wanted_length; ++i) {
        ull_t tmp = (ull_t) pow(alphabet_length, i);
        if (sum + tmp >= combination) {
            cur_len = i;
            break;
        }
        sum += tmp;
    }
    combination -= (sum);
    ull_t ind;
    ull_t p = (ull_t) pow(alphabet_length, cur_len - 1);
    for (ull_t i = 0; i < cur_len; ++i) {
        ind = (ull_t) floor(combination / p);
        combination -= (ind * p);
        str[i] = alphabet[ind];
        p /= alphabet_length;
    }
    str[cur_len] = '\0';

    return cur_len;
}

//количество эллементов каждому процессу
ull_t get_block_size(ull_t n, int rank, int nprocs) {
    ull_t s = n / nprocs;
    if (n % nprocs > rank)
        s++;
    return s;
}

//стартовые позиции
ull_t get_start_block(ull_t n, ull_t rank, int nprocs) {
    ull_t rem = n % nprocs;
    return n / nprocs * rank + ((rank >= rem) ? rem : rank);
}

int main(int argc, char **args) {

 // Всего потоков
    int commsize = 4;
    int rank = 3;

    
        MPI_Init(&argc, &args);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &commsize);
    


	    if (argc != 4) {
		if (rank == 0) printf("Format: \"Alphabet\" \"Wanted\" \"Lenght\"\n");
		return EXIT_FAILURE;
	    }

    alphabet = args[1];
    wanted = args[2];

    alphabet_length = (int) strlen(alphabet); // Длина алфавита
    wanted_length = atoi(args[3]); // Искомая строка

    buffer_current_line = malloc(sizeof(char) * wanted_length + 1);

    if (rank == 0) printf("Alphabet: %s\nWanted: %s\n\n", alphabet, wanted);

    MD5((const unsigned char *) wanted,
        strlen(wanted),
        (md5_input)
    ); // Считаем 1 раз хеш для искомой строки

    qsort(alphabet, (size_t) alphabet_length, sizeof(char), (__compar_fn_t) strcmp);

   
    long sizePerm = getSizePerm();
    comb_per_proc = get_block_size((ull_t) sizePerm, rank, commsize);
    start_comb = get_start_block((ull_t) sizePerm, (ull_t) rank, commsize);

    size_t n = comb(buffer_current_line, start_comb);
    maximum_length = wanted_length;
    wanted_length = (int) n;
    // Сколько каждый поток обрабатывает перестановок
    setIndex();
	double time = MPI_Wtime();
    do {
        repeat_permutations(0);
        wanted_length++;
        //memset(buffer_current_line, alphabet[0], (size_t) alphabet_length);
        buffer_current_line[wanted_length] = '\0';
    } while ((wanted_length <= maximum_length) && (count_perm < comb_per_proc));
	time = MPI_Wtime() - time;
	printf("time = %lf\n", time);
    // if (!isKey) printf("Sorry, but you password not found\n");
    if(rank == 0) printf("All permutations %d %d\n", (int) sizePerm, (int) comb_per_proc);
    MPI_Finalize();
    return 0;
}
