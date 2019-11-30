#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Определение конечной длины алфавита и выявление ошибки форматирования (возврат 0)
unsigned get_unrolled_alphabet_size(char* alph_formatted_input)
{
    unsigned char len = strlen(alph_formatted_input);
    unsigned char new_alph_len = 0;
    for(int i = 0; i < len; i++)
    {

        // a-z,A-Z поиск 
        if( alph_formatted_input[i] == '-' && i && i - len + 1)
        {
            if(alph_formatted_input[i+2] == ',' || alph_formatted_input[i+2] == '\0' ) {
                int temp = alph_formatted_input[i+1] - alph_formatted_input[i-1];
                if (temp < 0) temp *= -1;
                new_alph_len += temp;
                i+=2;
            }
            else 
                return 0; 
        } else {
            new_alph_len++;
        }
    }
    return new_alph_len;

}

int comparator(const void* p1, const void* p2){
    int l = (int)(*(char*)p1);
    int r = (int)(*(char*)p2);  
    return (l - r); 
}

/*
Парсинг алфавита: 
    -- возможность задания диапазонов в виде "а-б" печечисляемых через запятую;
    -- сортировка алфавита по возрастанию;
    -- исключение повторений;
    -- запись результата в массив с подсчитанным функцией размером, равным размеру получаемого алфавита
*/
char parse_alphabet_malloc(
    char* alph_formatted_input, // Форматированный с использованием диапазонов a-z алфавит
    char** alph_output,         // Получаемый алфавит
    unsigned* alph_out_length)  // Длина получаемого алфавита
{
    unsigned char in_len = strlen(alph_formatted_input);
    unsigned alph_unrolled_len = get_unrolled_alphabet_size(alph_formatted_input);
    if(alph_unrolled_len == 0) return 1;

    char buf[alph_unrolled_len+1];
    int counter = 0;

    // Заполнение алфавита из входной строки
    for(int i = 0; i < in_len; i++)
    {
        if( alph_formatted_input[i] == '-' && i && i - in_len + 1)
        {
            if(alph_formatted_input[i+2] == ',' || alph_formatted_input[i+2] == '\0' ) {
                char first = alph_formatted_input[i-1];
                char last = alph_formatted_input[i+1];

                while(first < last) {
                    buf[counter++] = ++first;
                    // alph_output[counter++] = ++first;
                }
                while(first > last) {
                    buf[counter++] = --first;
                    // alph_output[counter++] = --first;
                }
                i+=2;
            }
        } else {
            buf[counter++] = alph_formatted_input[i];
            // alph_output[counter++] = alph_formatted_input[i];
        }
    }

    buf[counter] = '\0';
    // printf("%s, len = %d\n", buf, counter);
    qsort(buf,strlen(buf),1, comparator);
    // printf("%s\n", buf);
    // for(int i = 0; i < counter; i++)
    //     printf("%c\n", buf[i]);
    counter = 0;
    *alph_out_length = 0;
    char buf_unique[alph_unrolled_len+1];

    while(buf[counter] != '\0')
    {
        buf_unique[*alph_out_length] = buf[counter];
        counter++;
        while(buf_unique[*alph_out_length] == buf[counter]) {
            counter++;
        }
        *alph_out_length = *alph_out_length + 1;
    }
    buf_unique[*alph_out_length] = '\0';

    *alph_output = (char*)malloc(sizeof(char) * (*alph_out_length + 1));

    if(*alph_output == NULL) {
        return 2;
    }

    counter = 0;
    while(buf_unique[counter] != '\0') {
        *(*alph_output + counter) = buf_unique[counter];
        counter++;
    }
    *(*alph_output + counter) = '\0';

    return 0;
}


/*
Парсинг алфавита: 
    -- возможность задания диапазонов в виде "а-б" печечисляемых через запятую;
    -- сортировка алфавита по возрастанию;
    -- исключение повторений;
*/
char parse_alphabet(
    char* alph_formatted_input, // Форматированный с использованием диапазонов a-z алфавит
    char alph_output[],         // Получаемый алфавит
    unsigned* alph_out_length)  // Длина получаемого алфавита
{
    unsigned char in_len = strlen(alph_formatted_input);
    unsigned alph_unrolled_len = get_unrolled_alphabet_size(alph_formatted_input);
    if(alph_unrolled_len == 0) return 1;

    char buf[alph_unrolled_len+1];
    int counter = 0;

    // Заполнение алфавита из входной строки
    for(int i = 0; i < in_len; i++)
    {
        if( alph_formatted_input[i] == '-' && i && i - in_len + 1)
        {
            if(alph_formatted_input[i+2] == ',' || alph_formatted_input[i+2] == '\0' ) {
                char first = alph_formatted_input[i-1];
                char last = alph_formatted_input[i+1];

                while(first < last) {
                    buf[counter++] = ++first;
                    // alph_output[counter++] = ++first;
                }
                while(first > last) {
                    buf[counter++] = --first;
                    // alph_output[counter++] = --first;
                }
                i+=2;
            }
        } else {
            buf[counter++] = alph_formatted_input[i];
            // alph_output[counter++] = alph_formatted_input[i];
        }
    }

    buf[counter] = '\0';
    qsort(buf,strlen(buf),1, comparator);


    counter = 0;
    *alph_out_length = 0;
    while(buf[counter] != '\0')
    {
        alph_output[*alph_out_length] = buf[counter];
        counter++;
        while(alph_output[*alph_out_length] == buf[counter]) {
            counter++;
        }
        *alph_out_length = *alph_out_length + 1;
    }
    alph_output[*alph_out_length] = '\0';

    return 0;
}


// Определение длин искомых строк из 3 аргумента. Минимальная длина искомой строки - число до разделителя, максимальная длина искомой строки - после.
char parse_arg_to_unsigned(char* input, char delim, unsigned int *num_1, unsigned int *num_2) {
    char parse[256];
    int i = 0, j = 0;
    while(isdigit(input[i]))
    {
        parse[i] = input[i];
        i++;
    }
    if(( input[i] == delim || input[i] == '\0') && i != 0) {
        parse[i] = '\0';
        // printf("min %s\n", parse);
        *num_1 = atoi(parse);
    } else 
        return 1;
    if(*num_1 == 0) return -1;

    if(input[i] == '\0')
    {
        *num_2 = *num_1;
        *num_1 = 1;
    } else {

        i++;
        while(isdigit(input[i]))
        {
            parse[j] = input[i];
            i++;
            j++;
        }
        if (input[i] != '\0' || j == 0) { return 2; }

        parse[j] = '\0';

        // printf("max %s\n", parse);
        *num_2 = atoi(parse);
        if(*num_2 == 0) return -2;
        // if (*num_2 < *num_1){

        //     unsigned int temp = *num_2;
        //     *num_2 = *num_1;
        //     *num_1 = temp;
        // }
    }
    return 0;
}