#include <sys/time.h>
#include <stdlib.h>

// Возвращает текущее машинное время в секундах
double Wtime(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec * 1E-6;
}

void PrintTime(double seconds) {
	int trunc = (int)seconds;
	int msec = (int)(seconds * 1000) - trunc * 1000;
	printf( "%d:%02d:%02d.%02d", trunc/3600, trunc/60, trunc, msec );
}
