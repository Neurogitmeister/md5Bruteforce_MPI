#include <sys/time.h>
#include <stdlib.h>

// Возвращает текущее машинное время в секундах
double Wtime(){
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return tv.tv_sec + (double)tv.tv_usec / 1000000;
}

double PrintTime(double seconds) {
	printf( "%d:%d:%d", (int)(seconds/3800), (int)(seconds/60), (int)seconds );
}
