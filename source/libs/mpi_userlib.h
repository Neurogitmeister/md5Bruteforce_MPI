#include <mpi.h>
#include <stdio.h>
#include <string.h>

// Вывод в консоль сообщений процессов из commsize в порядке возрастания ранга
void MPI_Print_in_rank_order(int commsize, int rank, char* message) {

    MPI_Barrier(MPI_COMM_WORLD);

	MPI_Recv(NULL, 0, MPI_INT, (rank > 0) ? rank - 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	printf(message, NULL);
	MPI_Ssend(NULL, 0, MPI_INT, rank != commsize - 1 ? rank + 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD);
	if(rank == commsize - 1)
		printf("\n");
}
// Вывод неповторяющихся сообщений процессов из commsize в порядке возрастания с указанием дипазона рангов
void MPI_Print_in_rank_order_unique(int commsize, int rank, char* message) {

    unsigned short len = strlen(message) + 1;
    char* prev_message = malloc(sizeof(char) * len);
    unsigned char print_message = 0;

    if(rank == 0) {
        printf("rank    0 - ");
        fflush(stdout);
    }
    else {
        MPI_Recv(prev_message, len, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if(strcmp(message, prev_message))
                    print_message = 1;
        
    }
    MPI_Send(message, len, MPI_CHAR, rank != commsize - 1 ? rank + 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD);

	MPI_Recv(NULL, 0, MPI_CHAR, (rank > 0) ? rank - 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (print_message == 1) {
                printf("%-4d : %s\n", rank - 1, prev_message);
                printf("rank %4d - ", rank);
                fflush(stdout);
        }
	MPI_Ssend(NULL, 0, MPI_CHAR, rank != commsize - 1 ? rank + 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD);

	if(rank == commsize - 1) {
		printf("%-4d : %s\n", rank, message);
        fflush(stdout);
    }
    free(prev_message);
    return;
}
