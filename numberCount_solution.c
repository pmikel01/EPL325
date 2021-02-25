/********************************************************
 * numberCount_solution.c
 * Login to machine 103ws??.in.cs.ucy.ac.cy
 * Compile Using:
 * gcc -lpthread -Werror -Wall -O3 numberCount_solution.c -o numberCount_solution.out
 * Run Using:
 * 	./numberCount_solution.out 10 1000000000 100 3
 * Number of Arguments 5
 * Number of pThreads 10 Array Size: 1000000000. SetSize = 100. CountNumber 3.
 */
#include <pthread.h>
#include "support.h"
#define CONST_CHUNKSIZE 1000000
#define DISTANCE 100
typedef struct {
  int       id;
  int 		numThreads;
  int       arraySize;
  int  	    *theArray; 
  int 		countNumber;
  int 		count;
  int 		chunk_size;
} package_t; 

static int chunk_start;
int arraySizeGLOBAL=0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Count the instances of countNumber */
int countNumb_Serial(int size, int countNumber, int * theArray){
  int i, count = 0 ;
  for (i = 0; i < size; i++){
	if (theArray[i] == countNumber)count++;
	}
  return count;
}
/* Count the instances of countNumber. Access data at a *distance* each time*/
int countNumb_Serial_interleaved(int size, int countNumber, int * theArray, int distance){
  int i,j, count = 0;
  for (j = 0; j<distance;j++)
	  for (i = j; i < size; i+=distance){
		if (theArray[i] == countNumber)count++;
		}
  return count;
}
/***********************************
 * Static pThreads Implementation
 **********************************/
void *countNumberStatic(void *arg)
{	int i;
//int count=0;
	package_t *p=(package_t *)arg;
	startTime(p->id);
	for(i = p->chunk_size*p->id; i < p->chunk_size*(p->id+1); i++){
		if (p->theArray[i]==p->countNumber){
				p->count++;
				//count++;	// For Local count
				//printf("\a"); // Use this to create extra load on ++
		}
	}
	//p->count=count; // For Local count
	stopTime(p->id);
	pthread_exit(NULL);
}
int countNumber_pThreadStatic(int arraySize, int countNumber, int * theArray, int numberOfThreads){
	int i=0, count=0;
	pthread_t threads[numberOfThreads];
	package_t *p[numberOfThreads];
	int chunk_size = arraySize/numberOfThreads;
    for (i=0; i < numberOfThreads; i++){
			p[i] = (package_t *)malloc(sizeof(package_t));
			p[i]->id = i;
			p[i]->arraySize=arraySize;
			p[i]->theArray = theArray;
			p[i]->countNumber = countNumber;
			p[i]->count = 0;
			p[i]->chunk_size = (i==numberOfThreads-1)?arraySize-i*chunk_size:chunk_size;
            pthread_create(&threads[i], NULL, countNumberStatic, (void*)p[i]);
    }

    for (i=0; i < numberOfThreads; i++){
        pthread_join(threads[i], NULL);
		printf("Thread %d: ",i);elapsedTime(i);
		count += p[i]->count;
	}
    return count;
}
/***********************************
 * Static Interleaved pThreads Implementation
 **********************************/
void *countNumberStatic_interleaved(void *arg)
{	int i,j;
	package_t *p=(package_t *)arg;
	startTime(p->id);
	/*for(i = p->id; i < p->arraySize; i+=p->numThreads){
		if (p->theArray[i]==p->countNumber)
			p->count++;
	}*/
	for (j = 0; j<DISTANCE;j+=p->numThreads)
		for(i = p->id+j; i < p->arraySize; i+=DISTANCE){
			if (p->theArray[i]==p->countNumber)
				p->count++;
	}
	stopTime(p->id);
	pthread_exit(NULL);
}
int countNumber_pThreadStatic_interleaved(int arraySize, int countNumber, int * theArray, int numberOfThreads){
	int i=0, count=0;
	pthread_t threads[numberOfThreads];
	package_t *p[numberOfThreads];
	int chunk_size = arraySize/numberOfThreads;
    for (i=0; i < numberOfThreads; i++){
			p[i] = (package_t *)malloc(sizeof(package_t));
			p[i]->id = i;
			p[i]->numThreads = numberOfThreads;
			p[i]->arraySize=arraySize;
			p[i]->theArray = theArray;
			p[i]->countNumber = countNumber;
			p[i]->count = 0;
			p[i]->chunk_size = (i==numberOfThreads-1)?arraySize-i*chunk_size:chunk_size;
            pthread_create(&threads[i], NULL, countNumberStatic_interleaved, (void*)p[i]);
    }

    for (i=0; i < numberOfThreads; i++){
        pthread_join(threads[i], NULL);
		printf("Thread %d: ",i);elapsedTime(i);
		count += p[i]->count;
	}
    return count;
}

/***********************************
 * Dynamic pThreads Implementation
 **********************************/
int getNextChunkStart(){
	/*It's better to lock in getNextChunkStart()*/
	pthread_mutex_lock(&mutex);
		chunk_start+=CONST_CHUNKSIZE;
	pthread_mutex_unlock(&mutex);
	return ((chunk_start<arraySizeGLOBAL)?chunk_start:-1);
}

void *countNumberDynamic(void *arg)
{	int i,start;
	package_t *p=(package_t *)arg;
	startTime(p->id);
	while(1){
		start=getNextChunkStart();
		if (start<0) {stopTime(p->id);pthread_exit(NULL);}
		p->chunk_size+=1;
		for(i = start; i < start+CONST_CHUNKSIZE && i < p->arraySize; i++){
			if (p->theArray[i]==p->countNumber){
				p->count++;
				//printf("\a"); // Use this to create extra load on ++
			}
		}
	}
	stopTime(p->id);
	pthread_exit(NULL);
}
int countNumber_pThreadDynamic(int arraySize, int countNumber, int * theArray, int numberOfThreads){
	int i=0, count=0;
	chunk_start=-CONST_CHUNKSIZE;
	pthread_t threads[numberOfThreads];
	package_t *p[numberOfThreads];
    for (i=0; i < numberOfThreads; i++){
			p[i] = (package_t *)malloc(sizeof(package_t));
			p[i]->id = i;
			p[i]->arraySize=arraySize;
			p[i]->theArray = theArray;
			p[i]->countNumber = countNumber;
			p[i]->count = 0;
			p[i]->chunk_size = 0; // In the Dynamic we use this to count how many chuncks the thread processed.
            pthread_create(&threads[i], NULL, countNumberDynamic, (void*)p[i]);
    }

    for (i=0; i < numberOfThreads; i++){
        pthread_join(threads[i], NULL);
		printf("Thread %d:Processed %d\tchunks.",i,p[i]->chunk_size);elapsedTime(i);
		count += p[i]->count;
	}
    return count;
}

int
main(int argc, char **argv)
{
	int  ArraySize=0, setSize=0, countNumber=0, count=0, threadNumber=0;
	int * theArray;
	if (argc < 5){
		printf("Use: numberCount.out <NumberOfThreadss> <ArraySize> <setSize> <numberToCount>\n");
		printf("NOTE: if numberOfChunks/chankSize > 100 the the number represents chankSize else is numberOfChunks\n");
	}else{
		threadNumber = atoi(argv[1]);
		ArraySize = atoi(argv[2]);
		setSize = atoi(argv[3]);
		countNumber = atoi(argv[4]);
		arraySizeGLOBAL=ArraySize;
	}
	printf("Number of Arguments %d\n",argc);
	printf("Number of pThreads %d Array Size: %d. SetSize = %d. CountNumber %d.\n", threadNumber, ArraySize, setSize,countNumber);
	theArray = (int *)malloc(sizeof(int)*ArraySize);
  /**********************************************************************/
	initData(ArraySize, setSize, theArray);
	//initDataSKEWED(ArraySize, setSize, theArray, countNumber);
  /**********************************************************************/
	#ifdef DEBUG
		printf("Uniformly Distributed Data:\n");
		printArray(theArray,ArraySize);
	#endif
	printf("Counting %d in Randomly Distributed Data\n",countNumber);
	//printf("Counting %d in SKEWED !!!! Distributed Data\n",countNumber);
	startTime(999);
	count = countNumb_Serial(ArraySize, countNumber, theArray);
	stopTime(999);elapsedTime(999);printf("SERIAL: number of instances found %d (%f%%):\n\n",count,(float)100*count/ArraySize);
	startTime(999);
	count = countNumb_Serial_interleaved(ArraySize, countNumber, theArray, DISTANCE);
	stopTime(999);elapsedTime(999);printf("SERIAL Interleaved: number of instances found %d (%f%%):\n\n",count,(float)100*count/ArraySize);
	//Run Using Static distribution between the threads
	startTime(999);
	count = countNumber_pThreadStatic(ArraySize, countNumber, theArray,threadNumber);
	stopTime(999);elapsedTime(999);printf("STATIC-pThreads: number of instances found %d (%f%%):\n\n",count,(float)100*count/ArraySize);
	//Run Using Static Interleaved distribution between the threads
	startTime(999);
	count = countNumber_pThreadStatic_interleaved(ArraySize, countNumber, theArray,threadNumber);
	stopTime(999);elapsedTime(999);printf("STATIC_interleaved-pThreads: number of instances found %d (%f%%):\n\n",count,(float)100*count/ArraySize);
	//Run Using Dynamic distribution between the threads
	startTime(999);
	count = countNumber_pThreadDynamic(ArraySize, countNumber, theArray,threadNumber);
	stopTime(999);elapsedTime(999);printf("DYNAMIC-pThreads: number of instances found %d (%f%%):\n\n",count,(float)100*count/ArraySize);
  /**********************************************************************/
    CountingSort(ArraySize, setSize, theArray);
	printf("*** AFTER SORTING THE DATA ***\n");
  	printf("Counting %d on Sorted Data\n",countNumber);
	//printf("Counting %d in SKEWED !!!! Distributed Data\n",countNumber);
	startTime(999);
	count = countNumb_Serial(ArraySize, countNumber, theArray);
	stopTime(999);elapsedTime(999);printf("SERIAL: number of instances found %d (%f%%):\n\n",count,(float)100*count/ArraySize);
	startTime(999);
	count = countNumb_Serial_interleaved(ArraySize, countNumber, theArray, DISTANCE);
	stopTime(999);elapsedTime(999);printf("SERIAL Interleaved: number of instances found %d (%f%%):\n\n",count,(float)100*count/ArraySize);
	//Run Using Static distribution between the threads
	//Run Using Static distribution between the threads
	startTime(999);
	count = countNumber_pThreadStatic(ArraySize, countNumber, theArray,threadNumber);
	stopTime(999);elapsedTime(999);printf("STATIC-pThreads: number of instances found %d (%f%%):\n\n",count,(float)100*count/ArraySize);
	//Run Using Static Interleaved distribution between the threads
	startTime(999);
	count = countNumber_pThreadStatic_interleaved(ArraySize, countNumber, theArray,threadNumber);
	stopTime(999);elapsedTime(999);printf("STATIC_interleaved-pThreads: number of instances found %d (%f%%):\n\n",count,(float)100*count/ArraySize);
	//Run Using Dynamic distribution between the threads
	startTime(999);
	count = countNumber_pThreadDynamic(ArraySize, countNumber, theArray,threadNumber);
	stopTime(999);elapsedTime(999);printf("DYNAMIC-pThreads: number of instances found %d (%f%%):\n\n",count,(float)100*count/ArraySize);
  /**********************************************************************/
  return 0;
} 	

