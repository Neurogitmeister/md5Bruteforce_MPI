
#include <mpi.h>
#include <stdio.h>

#define N 10000000

int main() {
    int rank, commsize;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);


    double time_min = 0.123456;
    double time_complete = 0.133567;
    double time;
    {
        long long int time_a, time_b;
        time = MPI_Wtime();
        for(int i = 0 ; i < N; i++) {

            time_a = time_min * 10000;
            time_b = time_complete * 10000;

            if(time_a == time_b) {
                continue;
            }
        }
        time = MPI_Wtime() - time;
    }
    printf("rank %d, Truncate: time = %lf\n\n",rank, time);
    {
        double time_compare;
        time = MPI_Wtime();
        for(int i = 0 ; i < N; i++) {

            time_compare = time_complete - time_complete * 0.00001;
            if(time_min > time_compare) {
                continue;
            }
        }
        time = MPI_Wtime() - time;
    }
    printf("rank %d, Compare: time = %lf\n\n",rank, time);
    MPI_Finalize();
    return 0;
}
