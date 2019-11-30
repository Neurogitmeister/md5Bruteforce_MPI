#include <stdio.h>

#include "../libs/wtime.h"

#define N 1000

int main() {

    double time_min = 0.123456789;

    double times_complete[] = { 0.133567, 0.153432412, 0.1513423, 0.32534452325, 0.123460000, 0.12345700000, 0.1234599999, 0.1234568 };

    double time, timesum = 0.0;
    {
        long long int time_a, time_b;
        for(int i = 0 ; i < N; i++) {
        for(int j = 0; j < 8; j++) {
            time = Wtime();
                time_a = time_min * 1E+6;
                time_b = times_complete[j] * 1E+6;

                if(time_a == time_b) {
                   printf("%lf ", times_complete[j]);
                }
            time = Wtime() - time;
            timesum += time;
        }
        }
    }

    printf("\nTruncate: time = %.15lf\n\n", timesum);
    timesum = 0.0;
    {
        double time_compare;
        for(int i = 0 ; i < N; i++){
        for(int j = 0; j < 8; j++) {
        
            time = Wtime();
            if(time_min > (times_complete[j] - times_complete[j] * 0.000001)) {
                printf("%lf ", times_complete[j]);
            }
            time = Wtime() - time;
            timesum += time;
        }
        }
    }

    printf("\nCompare : time = %.15lf\n\n", timesum);

    double other_time_min = time_min;

    double time_diff = other_time_min - time_min;
    printf("%.15lf\n", time_diff);
    if(time_diff == 0.0) {
        printf("suck my cock\n");
    }
    return 0;
}
