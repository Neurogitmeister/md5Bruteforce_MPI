#include <mpi.h>
#include <stdio.h>

// Вывод в консоль уникальных сообщений процессов из commsize в порядке возрастания ранга
void MPI_Print_in_rank_order(int commsize, int rank, char* message) {
	MPI_Recv(NULL, 0, MPI_INT, (rank > 0) ? rank - 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	printf(message, NULL);
	MPI_Ssend(NULL, 0, MPI_INT, rank != commsize - 1 ? rank + 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD);
	if(rank == commsize - 1)
		printf("\n");
}

void MPI_Print_in_rank_order_unique(int commsize, int rank, char* message) {
    unsigned short len = strlen(message) + 1;

    char prev_message[len];

    if(!rank) printf("rank    0 - ");

    MPI_Recv(prev_message, len, MPI_CHAR, (rank > 0) ? rank - 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if(!strcmp(message, prev_message));
        else if (rank){
            printf("%-4d : %s\nrank %4d - ", rank - 1, prev_message, rank);
        }
	MPI_Ssend(message, len, MPI_CHAR, rank != commsize - 1 ? rank + 1 : MPI_PROC_NULL, 0, MPI_COMM_WORLD);

	if(rank == commsize - 1)
		printf("%-4d : %s\n", rank, message);
}
