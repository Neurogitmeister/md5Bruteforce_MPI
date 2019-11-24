// Парсинг алфавита: возможность задания диапазонов в виде "а-б", печечисляемых через запятую 
// (скоро и исключение повторений)
// Возвращает:
// 0 - успех,
// 1 - неверное форматирование.

unsigned char get_alphabet_size(char* alph_input)
{
    unsigned char len = strlen(alph_input);

    // Определение конечной длины алфавита и выявление ошибки форматирования
    unsigned char new_alph_len = 0;
    for(int i = 0; i < len; i++)
    {
        if( alph_input[i] == '-' && i && i - len + 1)
        {
            if(alph_input[i+2] == ',' || alph_input[i+2] == '\0' ) {
                int temp = alph_input[i+1] - alph_input[i-1];
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
void parse_alphabet(char* alph_input, char* alph_output)
{
    unsigned char len = strlen(alph_input);

    // Заполнение алфавита из входной строки
    int counter = 0;
    for(int i = 0; i < len; i++)
    {
        if( alph_input[i] == '-' && i && i - len + 1)
        {
            if(alph_input[i+2] == ',' || alph_input[i+2] == '\0' ) {
                // a-z,A-Z
                char first = alph_input[i-1];
                char last = alph_input[i+1];
                while(first < last) {
                    alph_output[counter++] = ++first;
                }
                while(first > last) {
                    alph_output[counter++] = --first;
                }
                i+=2;
            }
        } else {
            alph_output[counter++] = alph_input[i];
        }
    }
    alph_output[counter] = '\0';
}