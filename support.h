#include <sys/time.h>

typedef struct {
    struct timeval startTime;
    struct timeval endTime;
} Timer;

Timer timer[1000];

void startTime(int i);
void stopTime(int i);
void elapsedTime(int i);


void startTime(int i) {
	#ifdef DEBUG
		printf("Start Timer...");
	#endif
    gettimeofday(&(timer[i].startTime), NULL);
}

void stopTime(int i) {
    #ifdef DEBUG
		printf("Stop Timer...");
	#endif
    gettimeofday(&(timer[i].endTime), NULL);
}

void elapsedTime(int i) {
	float elapseTime = (float) ((timer[i].endTime.tv_sec - timer[i].startTime.tv_sec) \
                + (timer[i].endTime.tv_usec - timer[i].startTime.tv_usec)/1.0e6);
	#ifdef DEBUG
		printf("Elapsed Time: ");
	#endif
	printf("%4.2f Sec.\n",elapseTime);
}

