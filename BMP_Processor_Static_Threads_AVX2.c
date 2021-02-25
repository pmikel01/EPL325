/*============================================================================
 File Name		: BMP_Processor_Serial_std.c
 Author			: Petros Panayi (R) Spring 2021
 Compile and Run: gcc -O3 -Wall -Werror BMP_Processor_Serial_std.c -o BMP_Processor_Serial_std.out
 ** If you want to see the output images use Debug Version **
 [DEBUG Vers.]	: gcc -O3 -Wall -Werror -DDEBUG BMP_Processor_Serial_std.c -o BMP_Processor_Serial_std.out
				: ./BMP_Processor_Serial_std.out 5 image4.bmp
 Comments		: Serial implementation of BMP processor for EPL325 Projects
				: images should be in the same folder as the executable
 ============================================================================
 Profile the Code (pixelProcessor is 99.79% time and called 10736840):
 gcc -O3 -Wall -Werror -pg BMP_Processor_Serial_std.c -o BMP_Processor_Serial_std.out
./BMP_Processor_Serial_std.out 5 lady.bmp
 gprof ./BMP_Processor_Serial_std.out gmon.out > analysis.txt
less analysis.txt
  %   cumulative   self              self     total
 time   seconds   seconds    calls  ms/call  ms/call  name
 98.94      8.06     8.06  5347020     0.00     0.00  pixelProcessor
  1.23      8.16     0.10       20     5.01   408.17  CONVOLUTION
  0.00      8.16     0.00       20     0.00     0.00  load_HEADER
  0.00      8.16     0.00        1     0.00     0.00  elapsedTime
  0.00      8.16     0.00        1     0.00     0.00  startTime
  0.00      8.16     0.00        1     0.00     0.00  stopTime
index % time    self  children    called     name
                0.10    8.06      20/20          main [2]
[1]    100.0    0.10    8.06      20         CONVOLUTION [1]
                8.06    0.00 5347020/5347020     pixelProcessor [3]
-----------------------------------------------
                                                 <spontaneous>
[2]    100.0    0.00    8.16                 main [2]
                0.10    8.06      20/20          CONVOLUTION [1]
                0.00    0.00      20/20          load_HEADER [4]
				-----------------------------------------------
                8.06    0.00 5347020/5347020     CONVOLUTION [1]
[3]     98.8    8.06    0.00 5347020         pixelProcessor [3]
-----------------------------------------------

============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <immintrin.h>
#include "support.h"

#define RED 0.299
#define GREEN 0.587
#define BLUE 0.114

typedef unsigned char byte;
typedef unsigned int dword;
typedef unsigned short int word;

//DOMI DEDOMENO POY PERIEXEI TA TRIA XROMATA TON PIXEL.
typedef struct {
	byte R;
	byte G;
	byte B;
}__attribute__((packed)) tbyte;

//DOMI DEDOMENO POY PERIEXEI TA STOIXIA TOY BITMAP FILE HEADER.
typedef struct {
	byte bfType1;
	byte bfType2;
	dword bfSize;
	word bfReserved1;
	word bfReserved2;
	dword dfOffBits;
}__attribute__((packed)) BITMAP_FILE_HEADER;

typedef struct {
    tbyte **PIXEL;
    tbyte **PIXEL_OUT;
    int startWidth;
    int startHeight;
    int endWidth;
    int endHeight;
    int filterSize;
    float factor;
    int thread_num;
    dword width;
    dword height;
}__attribute__((packed)) ppArgs;

//DOMI DEDOMENO POY PERIEXEI TA STOIXIA TOY BITMAP INFO HEADER.
typedef struct {
	dword biSize;
	dword biWidth;
	dword biHeight;
	word biPlanes;
	word biBitCount;
	dword biCompression;
	dword biSizeImage;
	dword biXPelsPerMeter;
	dword biYPelsPerMeter;
	dword biClrUsed;
	dword biClrImportant;
}__attribute__((packed)) BITMAP_INFO_HEADER;

//DILOSI SINARTISEO
int load_HEADER(BITMAP_FILE_HEADER *BFH, BITMAP_INFO_HEADER *BIH, FILE *fp);
void LIST(BITMAP_FILE_HEADER *BFH, BITMAP_INFO_HEADER *BIH);
int CONVOLUTION(BITMAP_FILE_HEADER *BFH, BITMAP_INFO_HEADER *BIH, FILE *fp,
		char * new_name, int filterSize, float factor, int threads_sum);
int NEW_FILE_HEADER(BITMAP_FILE_HEADER *BFH, BITMAP_INFO_HEADER *BIH,
		FILE *new_fp);

//SINARTISI POY DIABAZI TO HEADER ENOS ARXEIOY 'BMP'.
//DEXETE OS ORIZMATA TA DIO MERI TOY HEADER KAI TO ARXEIO.
//EPISTREFI 0 GIA OMALI EKTELESI KAI 1 GIA PROBLIMATIKI.
int load_HEADER(BITMAP_FILE_HEADER *BFH, BITMAP_INFO_HEADER *BIH, FILE *fp) {
	//BITMAP FILE HEADER
	fread((void *) &(BFH->bfType1), sizeof(byte), 1, fp);
	fread((void *) &(BFH->bfType2), sizeof(byte), 1, fp);

	//EAN TO ARXIO DEN EINAI 'BMP' EPISTREFI 1-PROBLIMA.
	if (BFH->bfType1 != 'B' || BFH->bfType2 != 'M')
		return (1);

	fread((void *) &(BFH->bfSize), sizeof(dword), 1, fp);
	fread((void *) &(BFH->bfReserved1), sizeof(word), 1, fp);
	fread((void *) &(BFH->bfReserved2), sizeof(word), 1, fp);
	fread((void *) &(BFH->dfOffBits), sizeof(dword), 1, fp);
	//BITMAP INFO HEADER.
	fread((void *) &(BIH->biSize), sizeof(dword), 1, fp);
	fread((void *) &(BIH->biWidth), sizeof(dword), 1, fp);
	fread((void *) &(BIH->biHeight), sizeof(dword), 1, fp);
	fread((void *) &(BIH->biPlanes), sizeof(word), 1, fp);
	fread((void *) &(BIH->biBitCount), sizeof(word), 1, fp);

	//EAN DEN EINAI ARXIO 24-bit EPISTREFI 1-PROBLIMA.
	if (BIH->biBitCount != 24)
		return (1);

	fread((void *) &(BIH->biCompression), sizeof(dword), 1, fp);

	//EAN EINAI SIMPIEZMENO ARXIO EPISTREFI 1-PROBLIMA.
	if (BIH->biCompression != 0)
		return (1);

	fread((void *) &(BIH->biSizeImage), sizeof(dword), 1, fp);
	fread((void *) &(BIH->biXPelsPerMeter), sizeof(dword), 1, fp);
	fread((void *) &(BIH->biYPelsPerMeter), sizeof(dword), 1, fp);
	fread((void *) &(BIH->biClrUsed), sizeof(dword), 1, fp);
	fread((void *) &(BIH->biClrImportant), sizeof(dword), 1, fp);

	//OTAN GINI KANONIKA H ANAGNOSI TOY ARXIOY EPISTREFI 0-KANONIKA.
	return (0);
}

//DEXETE OS ORIZMATA TA DIO MERI TOY HEADER KAI TA EKTIPONI.
void LIST(BITMAP_FILE_HEADER *BFH, BITMAP_INFO_HEADER *BIH) {

	printf("\nBITMAP_FILE_HEADER\n");
	printf("==================\n");
	printf("bfType: %c%c\n", BFH->bfType1, BFH->bfType2);
	printf("bfSize: %d\n", BFH->bfSize);
	printf("bfReserved1: %d\n", BFH->bfReserved1);
	printf("bfReserved2: %d\n", BFH->bfReserved2);
	printf("bfOffBits: %d\n", BFH->dfOffBits);
	printf("\nBITMAP_INFO_HEADER\n");
	printf("==================\n");
	printf("biSize: %d\n", BIH->biSize);
	printf("biWidth: %d\n", BIH->biWidth);
	printf("biHeight: %d\n", BIH->biHeight);
	printf("biPlanes: %d\n", BIH->biPlanes);
	printf("biBitCount: %d\n", BIH->biBitCount);
	printf("biCompression: %d\n", BIH->biCompression);
	printf("biSizeImage: %d\n", BIH->biSizeImage);
	printf("biXPelsPerMeter: %d\n", BIH->biXPelsPerMeter);
	printf("biYPelsPerMeter: %d\n", BIH->biYPelsPerMeter);
	printf("biClrUsed: %d\n", BIH->biClrUsed);
	printf("biClrImportant: %d\n", BIH->biClrImportant);
	printf("\n***************************************************************************\n");
}

void *pixelProcessor(void *arg){
    ppArgs *p=(ppArgs *)arg;
	int i,j;
    startTime(p->thread_num);
    int i_HEIGHT, i_WIDTH;

    //PERNI ENA-ENA TA PIXEL THS EIKONAS.
    for (i_HEIGHT=p->startHeight ; i_HEIGHT <= p->endHeight ; i_HEIGHT++) {
        if(i_HEIGHT!=p->endHeight && i_HEIGHT!=p->startHeight) {
            for (i_WIDTH=0 ; i_WIDTH < p->width && (i_WIDTH+8 <= (p->width)) ; i_WIDTH+=8) {
                if ((i_HEIGHT > p->filterSize) && (i_HEIGHT < (p->height) - p->filterSize)
                    && (i_WIDTH > p->filterSize) && (i_WIDTH < (p->width) - p->filterSize)) {

                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R = p->PIXEL[i_HEIGHT][i_WIDTH].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G = p->PIXEL[i_HEIGHT][i_WIDTH].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B = p->PIXEL[i_HEIGHT][i_WIDTH].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].R = p->PIXEL[i_HEIGHT][i_WIDTH+1].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].G = p->PIXEL[i_HEIGHT][i_WIDTH+1].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].B = p->PIXEL[i_HEIGHT][i_WIDTH+1].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].R = p->PIXEL[i_HEIGHT][i_WIDTH+2].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].G = p->PIXEL[i_HEIGHT][i_WIDTH+2].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].B = p->PIXEL[i_WIDTH][i_WIDTH+2].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].R = p->PIXEL[i_HEIGHT][i_WIDTH+3].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].G = p->PIXEL[i_HEIGHT][i_WIDTH+3].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].B = p->PIXEL[i_HEIGHT][i_WIDTH+3].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].R = p->PIXEL[i_HEIGHT][i_WIDTH+4].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].G = p->PIXEL[i_HEIGHT][i_WIDTH+4].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].B = p->PIXEL[i_HEIGHT][i_WIDTH+4].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].R = p->PIXEL[i_HEIGHT][i_WIDTH+5].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].G = p->PIXEL[i_HEIGHT][i_WIDTH+5].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].B = p->PIXEL[i_HEIGHT][i_WIDTH+5].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].R = p->PIXEL[i_HEIGHT][i_WIDTH+6].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].G = p->PIXEL[i_HEIGHT][i_WIDTH+6].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].B = p->PIXEL[i_HEIGHT][i_WIDTH+6].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].R = p->PIXEL[i_HEIGHT][i_WIDTH+7].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].G = p->PIXEL[i_HEIGHT][i_WIDTH+7].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].B = p->PIXEL[i_HEIGHT][i_WIDTH+7].B;

                    __m256i sumRedVec = _mm256_set_epi32(p->PIXEL[i_WIDTH][i_HEIGHT].R, p->PIXEL[i_WIDTH][i_HEIGHT+1].R, p->PIXEL[i_WIDTH][i_HEIGHT+2].R, p->PIXEL[i_WIDTH][i_HEIGHT+3].R, p->PIXEL[i_WIDTH][i_HEIGHT+4].R, p->PIXEL[i_WIDTH][i_HEIGHT+5].R, p->PIXEL[i_WIDTH][i_HEIGHT+6].R, p->PIXEL[i_WIDTH][i_HEIGHT+7].R);
                    __m256i sumGreenVec = _mm256_set_epi32(p->PIXEL[i_WIDTH][i_HEIGHT].G, p->PIXEL[i_WIDTH][i_HEIGHT+1].G, p->PIXEL[i_WIDTH][i_HEIGHT+2].G, p->PIXEL[i_WIDTH][i_HEIGHT+3].G, p->PIXEL[i_WIDTH][i_HEIGHT+4].G, p->PIXEL[i_WIDTH][i_HEIGHT+5].G, p->PIXEL[i_WIDTH][i_HEIGHT+6].G, p->PIXEL[i_WIDTH][i_HEIGHT+7].G);
                    __m256i sumBlueVec = _mm256_set_epi32(p->PIXEL[i_WIDTH][i_HEIGHT].B, p->PIXEL[i_WIDTH][i_HEIGHT+1].B, p->PIXEL[i_WIDTH][i_HEIGHT+2].B, p->PIXEL[i_WIDTH][i_HEIGHT+3].B, p->PIXEL[i_WIDTH][i_HEIGHT+4].B, p->PIXEL[i_WIDTH][i_HEIGHT+5].B, p->PIXEL[i_WIDTH][i_HEIGHT+6].B, p->PIXEL[i_WIDTH][i_HEIGHT+7].B);

                    for (i = i_HEIGHT-p->filterSize;  i<= i_HEIGHT + p->filterSize; i++) {
                        for (j = i_WIDTH-p->filterSize;  j<= i_WIDTH + p->filterSize; j++){
                            __m256 factorVec = _mm256_set1_ps(p->factor);
                            __m256 redFactorVec = _mm256_set1_ps(0.0);
                            __m256 greenFactorVec = _mm256_set1_ps(0.0);
                            __m256 blueFactorVec = _mm256_set1_ps(0.0);

                            __m256 redVec = _mm256_set_ps((float)p->PIXEL[i][j].R, (float)p->PIXEL[i][j+1].R, (float)p->PIXEL[i][j+2].R, (float)p->PIXEL[i][j+3].R, (float)p->PIXEL[i][j+4].R, (float)p->PIXEL[i][j+5].R, (float)p->PIXEL[i][j+6].R, (float)p->PIXEL[i][j+7].R);
                            __m256 greenVec = _mm256_set_ps((float)p->PIXEL[i][j].G, (float)p->PIXEL[i][j+1].G, (float)p->PIXEL[i][j+2].G, (float)p->PIXEL[i][j+3].G, (float)p->PIXEL[i][j+4].G, (float)p->PIXEL[i][j+5].G, (float)p->PIXEL[i][j+6].G, (float)p->PIXEL[i][j+7].G);
                            __m256 blueVec = _mm256_set_ps((float)p->PIXEL[i][j].B, (float)p->PIXEL[i][j+1].B, (float)p->PIXEL[i][j+2].B, (float)p->PIXEL[i][j+3].B, (float)p->PIXEL[i][j+4].B, (float)p->PIXEL[i][j+5].B, (float)p->PIXEL[i][j+6].B, (float)p->PIXEL[i][j+7].B);

                            redFactorVec = _mm256_mul_ps(redVec, factorVec);
                            greenFactorVec = _mm256_mul_ps(greenVec, factorVec);
                            blueFactorVec = _mm256_mul_ps(blueVec, factorVec);

                            sumRedVec = _mm256_add_epi32(sumRedVec, _mm256_cvtps_epi32(redFactorVec));
                            sumGreenVec = _mm256_add_epi32(sumGreenVec, _mm256_cvtps_epi32(greenFactorVec));
                            sumBlueVec = _mm256_add_epi32(sumBlueVec, _mm256_cvtps_epi32(blueFactorVec));
                        }
                    }
                    int* xx = (int*)&sumRedVec;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R += (int) xx[0];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].R += (int) xx[1];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].R += (int) xx[2];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].R += (int) xx[3];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].R += (int) xx[4];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].R += (int) xx[5];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].R += (int) xx[6];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].R += (int) xx[7];

                    int* xx2 = (int*)&sumGreenVec;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G += (int) xx2[0];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].G += (int) xx2[1];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].G += (int) xx2[2];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].G += (int) xx2[3];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].G += (int) xx2[4];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].G += (int) xx2[5];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].G += (int) xx2[6];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].G += (int) xx2[7];

                    int* xx3 = (int*)&sumBlueVec;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B += (int) xx3[0];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].B += (int) xx3[1];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].B += (int) xx3[2];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].B += (int) xx3[3];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].B += (int) xx3[4];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].B += (int) xx3[5];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].B += (int) xx3[6];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].B += (int) xx3[7];

                } else {
                    byte NEW_PIXEL;
                    NEW_PIXEL = (int) (p->PIXEL[i_HEIGHT][i_WIDTH].R * RED
                                       + p->PIXEL[i_HEIGHT][i_WIDTH].G * GREEN
                                       + p->PIXEL[i_HEIGHT][i_WIDTH].B * BLUE);
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R = NEW_PIXEL;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G = NEW_PIXEL;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B = NEW_PIXEL;
                }
            }
        } else if(i_HEIGHT==p->endHeight && i_HEIGHT==p->startHeight){
            for (i_WIDTH=p->startWidth ; i_WIDTH <= p->endWidth && (i_WIDTH+8 <= (p->width)) ; i_WIDTH+=8) {
                if ((i_HEIGHT > p->filterSize) && (i_HEIGHT < (p->height) - p->filterSize)
                    && (i_WIDTH > p->filterSize) && (i_WIDTH < (p->width) - p->filterSize)) {

                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R = p->PIXEL[i_HEIGHT][i_WIDTH].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G = p->PIXEL[i_HEIGHT][i_WIDTH].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B = p->PIXEL[i_HEIGHT][i_WIDTH].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].R = p->PIXEL[i_HEIGHT][i_WIDTH+1].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].G = p->PIXEL[i_HEIGHT][i_WIDTH+1].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].B = p->PIXEL[i_HEIGHT][i_WIDTH+1].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].R = p->PIXEL[i_HEIGHT][i_WIDTH+2].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].G = p->PIXEL[i_HEIGHT][i_WIDTH+2].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].B = p->PIXEL[i_WIDTH][i_WIDTH+2].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].R = p->PIXEL[i_HEIGHT][i_WIDTH+3].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].G = p->PIXEL[i_HEIGHT][i_WIDTH+3].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].B = p->PIXEL[i_HEIGHT][i_WIDTH+3].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].R = p->PIXEL[i_HEIGHT][i_WIDTH+4].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].G = p->PIXEL[i_HEIGHT][i_WIDTH+4].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].B = p->PIXEL[i_HEIGHT][i_WIDTH+4].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].R = p->PIXEL[i_HEIGHT][i_WIDTH+5].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].G = p->PIXEL[i_HEIGHT][i_WIDTH+5].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].B = p->PIXEL[i_HEIGHT][i_WIDTH+5].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].R = p->PIXEL[i_HEIGHT][i_WIDTH+6].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].G = p->PIXEL[i_HEIGHT][i_WIDTH+6].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].B = p->PIXEL[i_HEIGHT][i_WIDTH+6].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].R = p->PIXEL[i_HEIGHT][i_WIDTH+7].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].G = p->PIXEL[i_HEIGHT][i_WIDTH+7].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].B = p->PIXEL[i_HEIGHT][i_WIDTH+7].B;

                    __m256i sumRedVec = _mm256_set_epi32(p->PIXEL[i_WIDTH][i_HEIGHT].R, p->PIXEL[i_WIDTH][i_HEIGHT+1].R, p->PIXEL[i_WIDTH][i_HEIGHT+2].R, p->PIXEL[i_WIDTH][i_HEIGHT+3].R, p->PIXEL[i_WIDTH][i_HEIGHT+4].R, p->PIXEL[i_WIDTH][i_HEIGHT+5].R, p->PIXEL[i_WIDTH][i_HEIGHT+6].R, p->PIXEL[i_WIDTH][i_HEIGHT+7].R);
                    __m256i sumGreenVec = _mm256_set_epi32(p->PIXEL[i_WIDTH][i_HEIGHT].G, p->PIXEL[i_WIDTH][i_HEIGHT+1].G, p->PIXEL[i_WIDTH][i_HEIGHT+2].G, p->PIXEL[i_WIDTH][i_HEIGHT+3].G, p->PIXEL[i_WIDTH][i_HEIGHT+4].G, p->PIXEL[i_WIDTH][i_HEIGHT+5].G, p->PIXEL[i_WIDTH][i_HEIGHT+6].G, p->PIXEL[i_WIDTH][i_HEIGHT+7].G);
                    __m256i sumBlueVec = _mm256_set_epi32(p->PIXEL[i_WIDTH][i_HEIGHT].B, p->PIXEL[i_WIDTH][i_HEIGHT+1].B, p->PIXEL[i_WIDTH][i_HEIGHT+2].B, p->PIXEL[i_WIDTH][i_HEIGHT+3].B, p->PIXEL[i_WIDTH][i_HEIGHT+4].B, p->PIXEL[i_WIDTH][i_HEIGHT+5].B, p->PIXEL[i_WIDTH][i_HEIGHT+6].B, p->PIXEL[i_WIDTH][i_HEIGHT+7].B);

                    for (i = i_HEIGHT-p->filterSize;  i<= i_HEIGHT + p->filterSize; i++) {
                        for (j = i_WIDTH-p->filterSize;  j<= i_WIDTH + p->filterSize; j++){
                            __m256 factorVec = _mm256_set1_ps(p->factor);
                            __m256 redFactorVec = _mm256_set1_ps(0.0);
                            __m256 greenFactorVec = _mm256_set1_ps(0.0);
                            __m256 blueFactorVec = _mm256_set1_ps(0.0);

                            __m256 redVec = _mm256_set_ps((float)p->PIXEL[i][j].R, (float)p->PIXEL[i][j+1].R, (float)p->PIXEL[i][j+2].R, (float)p->PIXEL[i][j+3].R, (float)p->PIXEL[i][j+4].R, (float)p->PIXEL[i][j+5].R, (float)p->PIXEL[i][j+6].R, (float)p->PIXEL[i][j+7].R);
                            __m256 greenVec = _mm256_set_ps((float)p->PIXEL[i][j].G, (float)p->PIXEL[i][j+1].G, (float)p->PIXEL[i][j+2].G, (float)p->PIXEL[i][j+3].G, (float)p->PIXEL[i][j+4].G, (float)p->PIXEL[i][j+5].G, (float)p->PIXEL[i][j+6].G, (float)p->PIXEL[i][j+7].G);
                            __m256 blueVec = _mm256_set_ps((float)p->PIXEL[i][j].B, (float)p->PIXEL[i][j+1].B, (float)p->PIXEL[i][j+2].B, (float)p->PIXEL[i][j+3].B, (float)p->PIXEL[i][j+4].B, (float)p->PIXEL[i][j+5].B, (float)p->PIXEL[i][j+6].B, (float)p->PIXEL[i][j+7].B);

                            redFactorVec = _mm256_mul_ps(redVec, factorVec);
                            greenFactorVec = _mm256_mul_ps(greenVec, factorVec);
                            blueFactorVec = _mm256_mul_ps(blueVec, factorVec);

                            sumRedVec = _mm256_add_epi32(sumRedVec, _mm256_cvtps_epi32(redFactorVec));
                            sumGreenVec = _mm256_add_epi32(sumGreenVec, _mm256_cvtps_epi32(greenFactorVec));
                            sumBlueVec = _mm256_add_epi32(sumBlueVec, _mm256_cvtps_epi32(blueFactorVec));
                        }
                    }
                    int* xx = (int*)&sumRedVec;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R += (int) xx[0];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].R += (int) xx[1];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].R += (int) xx[2];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].R += (int) xx[3];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].R += (int) xx[4];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].R += (int) xx[5];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].R += (int) xx[6];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].R += (int) xx[7];

                    int* xx2 = (int*)&sumGreenVec;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G += (int) xx2[0];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].G += (int) xx2[1];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].G += (int) xx2[2];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].G += (int) xx2[3];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].G += (int) xx2[4];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].G += (int) xx2[5];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].G += (int) xx2[6];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].G += (int) xx2[7];

                    int* xx3 = (int*)&sumBlueVec;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B += (int) xx3[0];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].B += (int) xx3[1];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].B += (int) xx3[2];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].B += (int) xx3[3];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].B += (int) xx3[4];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].B += (int) xx3[5];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].B += (int) xx3[6];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].B += (int) xx3[7];

                } else {
                    byte NEW_PIXEL;
                    NEW_PIXEL = (int) (p->PIXEL[i_HEIGHT][i_WIDTH].R * RED
                                       + p->PIXEL[i_HEIGHT][i_WIDTH].G * GREEN
                                       + p->PIXEL[i_HEIGHT][i_WIDTH].B * BLUE);
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R = NEW_PIXEL;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G = NEW_PIXEL;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B = NEW_PIXEL;
                }
            }
        } else if(i_HEIGHT==p->startHeight){
            for (i_WIDTH=p->startWidth ; i_WIDTH < p->width && (i_WIDTH+8 <= (p->width)) ; i_WIDTH+=8) {
                if ((i_HEIGHT > p->filterSize) && (i_HEIGHT < (p->height) - p->filterSize)
                    && (i_WIDTH > p->filterSize) && (i_WIDTH < (p->width) - p->filterSize)) {

                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R = p->PIXEL[i_HEIGHT][i_WIDTH].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G = p->PIXEL[i_HEIGHT][i_WIDTH].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B = p->PIXEL[i_HEIGHT][i_WIDTH].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].R = p->PIXEL[i_HEIGHT][i_WIDTH+1].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].G = p->PIXEL[i_HEIGHT][i_WIDTH+1].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].B = p->PIXEL[i_HEIGHT][i_WIDTH+1].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].R = p->PIXEL[i_HEIGHT][i_WIDTH+2].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].G = p->PIXEL[i_HEIGHT][i_WIDTH+2].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].B = p->PIXEL[i_WIDTH][i_WIDTH+2].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].R = p->PIXEL[i_HEIGHT][i_WIDTH+3].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].G = p->PIXEL[i_HEIGHT][i_WIDTH+3].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].B = p->PIXEL[i_HEIGHT][i_WIDTH+3].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].R = p->PIXEL[i_HEIGHT][i_WIDTH+4].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].G = p->PIXEL[i_HEIGHT][i_WIDTH+4].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].B = p->PIXEL[i_HEIGHT][i_WIDTH+4].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].R = p->PIXEL[i_HEIGHT][i_WIDTH+5].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].G = p->PIXEL[i_HEIGHT][i_WIDTH+5].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].B = p->PIXEL[i_HEIGHT][i_WIDTH+5].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].R = p->PIXEL[i_HEIGHT][i_WIDTH+6].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].G = p->PIXEL[i_HEIGHT][i_WIDTH+6].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].B = p->PIXEL[i_HEIGHT][i_WIDTH+6].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].R = p->PIXEL[i_HEIGHT][i_WIDTH+7].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].G = p->PIXEL[i_HEIGHT][i_WIDTH+7].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].B = p->PIXEL[i_HEIGHT][i_WIDTH+7].B;

                    __m256i sumRedVec = _mm256_set_epi32(p->PIXEL[i_WIDTH][i_HEIGHT].R, p->PIXEL[i_WIDTH][i_HEIGHT+1].R, p->PIXEL[i_WIDTH][i_HEIGHT+2].R, p->PIXEL[i_WIDTH][i_HEIGHT+3].R, p->PIXEL[i_WIDTH][i_HEIGHT+4].R, p->PIXEL[i_WIDTH][i_HEIGHT+5].R, p->PIXEL[i_WIDTH][i_HEIGHT+6].R, p->PIXEL[i_WIDTH][i_HEIGHT+7].R);
                    __m256i sumGreenVec = _mm256_set_epi32(p->PIXEL[i_WIDTH][i_HEIGHT].G, p->PIXEL[i_WIDTH][i_HEIGHT+1].G, p->PIXEL[i_WIDTH][i_HEIGHT+2].G, p->PIXEL[i_WIDTH][i_HEIGHT+3].G, p->PIXEL[i_WIDTH][i_HEIGHT+4].G, p->PIXEL[i_WIDTH][i_HEIGHT+5].G, p->PIXEL[i_WIDTH][i_HEIGHT+6].G, p->PIXEL[i_WIDTH][i_HEIGHT+7].G);
                    __m256i sumBlueVec = _mm256_set_epi32(p->PIXEL[i_WIDTH][i_HEIGHT].B, p->PIXEL[i_WIDTH][i_HEIGHT+1].B, p->PIXEL[i_WIDTH][i_HEIGHT+2].B, p->PIXEL[i_WIDTH][i_HEIGHT+3].B, p->PIXEL[i_WIDTH][i_HEIGHT+4].B, p->PIXEL[i_WIDTH][i_HEIGHT+5].B, p->PIXEL[i_WIDTH][i_HEIGHT+6].B, p->PIXEL[i_WIDTH][i_HEIGHT+7].B);

                    for (i = i_HEIGHT-p->filterSize;  i<= i_HEIGHT + p->filterSize; i++) {
                        for (j = i_WIDTH-p->filterSize;  j<= i_WIDTH + p->filterSize; j++){
                            __m256 factorVec = _mm256_set1_ps(p->factor);
                            __m256 redFactorVec = _mm256_set1_ps(0.0);
                            __m256 greenFactorVec = _mm256_set1_ps(0.0);
                            __m256 blueFactorVec = _mm256_set1_ps(0.0);

                            __m256 redVec = _mm256_set_ps((float)p->PIXEL[i][j].R, (float)p->PIXEL[i][j+1].R, (float)p->PIXEL[i][j+2].R, (float)p->PIXEL[i][j+3].R, (float)p->PIXEL[i][j+4].R, (float)p->PIXEL[i][j+5].R, (float)p->PIXEL[i][j+6].R, (float)p->PIXEL[i][j+7].R);
                            __m256 greenVec = _mm256_set_ps((float)p->PIXEL[i][j].G, (float)p->PIXEL[i][j+1].G, (float)p->PIXEL[i][j+2].G, (float)p->PIXEL[i][j+3].G, (float)p->PIXEL[i][j+4].G, (float)p->PIXEL[i][j+5].G, (float)p->PIXEL[i][j+6].G, (float)p->PIXEL[i][j+7].G);
                            __m256 blueVec = _mm256_set_ps((float)p->PIXEL[i][j].B, (float)p->PIXEL[i][j+1].B, (float)p->PIXEL[i][j+2].B, (float)p->PIXEL[i][j+3].B, (float)p->PIXEL[i][j+4].B, (float)p->PIXEL[i][j+5].B, (float)p->PIXEL[i][j+6].B, (float)p->PIXEL[i][j+7].B);

                            redFactorVec = _mm256_mul_ps(redVec, factorVec);
                            greenFactorVec = _mm256_mul_ps(greenVec, factorVec);
                            blueFactorVec = _mm256_mul_ps(blueVec, factorVec);

                            sumRedVec = _mm256_add_epi32(sumRedVec, _mm256_cvtps_epi32(redFactorVec));
                            sumGreenVec = _mm256_add_epi32(sumGreenVec, _mm256_cvtps_epi32(greenFactorVec));
                            sumBlueVec = _mm256_add_epi32(sumBlueVec, _mm256_cvtps_epi32(blueFactorVec));
                        }
                    }
                    int* xx = (int*)&sumRedVec;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R += (int) xx[0];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].R += (int) xx[1];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].R += (int) xx[2];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].R += (int) xx[3];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].R += (int) xx[4];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].R += (int) xx[5];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].R += (int) xx[6];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].R += (int) xx[7];

                    int* xx2 = (int*)&sumGreenVec;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G += (int) xx2[0];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].G += (int) xx2[1];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].G += (int) xx2[2];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].G += (int) xx2[3];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].G += (int) xx2[4];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].G += (int) xx2[5];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].G += (int) xx2[6];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].G += (int) xx2[7];

                    int* xx3 = (int*)&sumBlueVec;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B += (int) xx3[0];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].B += (int) xx3[1];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].B += (int) xx3[2];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].B += (int) xx3[3];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].B += (int) xx3[4];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].B += (int) xx3[5];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].B += (int) xx3[6];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].B += (int) xx3[7];

                } else {
                    byte NEW_PIXEL;
                    NEW_PIXEL = (int) (p->PIXEL[i_HEIGHT][i_WIDTH].R * RED
                                       + p->PIXEL[i_HEIGHT][i_WIDTH].G * GREEN
                                       + p->PIXEL[i_HEIGHT][i_WIDTH].B * BLUE);
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R = NEW_PIXEL;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G = NEW_PIXEL;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B = NEW_PIXEL;
                }
            }
        }else if(i_HEIGHT==p->endHeight){
            for (i_WIDTH=0 ; i_WIDTH < p->endWidth && (i_WIDTH+8 <= (p->width)) ; i_WIDTH+=8) {
                if ((i_HEIGHT > p->filterSize) && (i_HEIGHT < (p->height) - p->filterSize)
                    && (i_WIDTH > p->filterSize) && (i_WIDTH < (p->width) - p->filterSize)) {

                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R = p->PIXEL[i_HEIGHT][i_WIDTH].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G = p->PIXEL[i_HEIGHT][i_WIDTH].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B = p->PIXEL[i_HEIGHT][i_WIDTH].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].R = p->PIXEL[i_HEIGHT][i_WIDTH+1].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].G = p->PIXEL[i_HEIGHT][i_WIDTH+1].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].B = p->PIXEL[i_HEIGHT][i_WIDTH+1].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].R = p->PIXEL[i_HEIGHT][i_WIDTH+2].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].G = p->PIXEL[i_HEIGHT][i_WIDTH+2].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].B = p->PIXEL[i_WIDTH][i_WIDTH+2].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].R = p->PIXEL[i_HEIGHT][i_WIDTH+3].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].G = p->PIXEL[i_HEIGHT][i_WIDTH+3].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].B = p->PIXEL[i_HEIGHT][i_WIDTH+3].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].R = p->PIXEL[i_HEIGHT][i_WIDTH+4].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].G = p->PIXEL[i_HEIGHT][i_WIDTH+4].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].B = p->PIXEL[i_HEIGHT][i_WIDTH+4].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].R = p->PIXEL[i_HEIGHT][i_WIDTH+5].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].G = p->PIXEL[i_HEIGHT][i_WIDTH+5].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].B = p->PIXEL[i_HEIGHT][i_WIDTH+5].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].R = p->PIXEL[i_HEIGHT][i_WIDTH+6].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].G = p->PIXEL[i_HEIGHT][i_WIDTH+6].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].B = p->PIXEL[i_HEIGHT][i_WIDTH+6].B;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].R = p->PIXEL[i_HEIGHT][i_WIDTH+7].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].G = p->PIXEL[i_HEIGHT][i_WIDTH+7].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].B = p->PIXEL[i_HEIGHT][i_WIDTH+7].B;

                    __m256i sumRedVec = _mm256_set_epi32(p->PIXEL[i_WIDTH][i_HEIGHT].R, p->PIXEL[i_WIDTH][i_HEIGHT+1].R, p->PIXEL[i_WIDTH][i_HEIGHT+2].R, p->PIXEL[i_WIDTH][i_HEIGHT+3].R, p->PIXEL[i_WIDTH][i_HEIGHT+4].R, p->PIXEL[i_WIDTH][i_HEIGHT+5].R, p->PIXEL[i_WIDTH][i_HEIGHT+6].R, p->PIXEL[i_WIDTH][i_HEIGHT+7].R);
                    __m256i sumGreenVec = _mm256_set_epi32(p->PIXEL[i_WIDTH][i_HEIGHT].G, p->PIXEL[i_WIDTH][i_HEIGHT+1].G, p->PIXEL[i_WIDTH][i_HEIGHT+2].G, p->PIXEL[i_WIDTH][i_HEIGHT+3].G, p->PIXEL[i_WIDTH][i_HEIGHT+4].G, p->PIXEL[i_WIDTH][i_HEIGHT+5].G, p->PIXEL[i_WIDTH][i_HEIGHT+6].G, p->PIXEL[i_WIDTH][i_HEIGHT+7].G);
                    __m256i sumBlueVec = _mm256_set_epi32(p->PIXEL[i_WIDTH][i_HEIGHT].B, p->PIXEL[i_WIDTH][i_HEIGHT+1].B, p->PIXEL[i_WIDTH][i_HEIGHT+2].B, p->PIXEL[i_WIDTH][i_HEIGHT+3].B, p->PIXEL[i_WIDTH][i_HEIGHT+4].B, p->PIXEL[i_WIDTH][i_HEIGHT+5].B, p->PIXEL[i_WIDTH][i_HEIGHT+6].B, p->PIXEL[i_WIDTH][i_HEIGHT+7].B);

                    for (i = i_HEIGHT-p->filterSize;  i<= i_HEIGHT + p->filterSize; i++) {
                        for (j = i_WIDTH-p->filterSize;  j<= i_WIDTH + p->filterSize; j++){
                            __m256 factorVec = _mm256_set1_ps(p->factor);
                            __m256 redFactorVec = _mm256_set1_ps(0.0);
                            __m256 greenFactorVec = _mm256_set1_ps(0.0);
                            __m256 blueFactorVec = _mm256_set1_ps(0.0);

                            __m256 redVec = _mm256_set_ps((float)p->PIXEL[i][j].R, (float)p->PIXEL[i][j+1].R, (float)p->PIXEL[i][j+2].R, (float)p->PIXEL[i][j+3].R, (float)p->PIXEL[i][j+4].R, (float)p->PIXEL[i][j+5].R, (float)p->PIXEL[i][j+6].R, (float)p->PIXEL[i][j+7].R);
                            __m256 greenVec = _mm256_set_ps((float)p->PIXEL[i][j].G, (float)p->PIXEL[i][j+1].G, (float)p->PIXEL[i][j+2].G, (float)p->PIXEL[i][j+3].G, (float)p->PIXEL[i][j+4].G, (float)p->PIXEL[i][j+5].G, (float)p->PIXEL[i][j+6].G, (float)p->PIXEL[i][j+7].G);
                            __m256 blueVec = _mm256_set_ps((float)p->PIXEL[i][j].B, (float)p->PIXEL[i][j+1].B, (float)p->PIXEL[i][j+2].B, (float)p->PIXEL[i][j+3].B, (float)p->PIXEL[i][j+4].B, (float)p->PIXEL[i][j+5].B, (float)p->PIXEL[i][j+6].B, (float)p->PIXEL[i][j+7].B);

                            redFactorVec = _mm256_mul_ps(redVec, factorVec);
                            greenFactorVec = _mm256_mul_ps(greenVec, factorVec);
                            blueFactorVec = _mm256_mul_ps(blueVec, factorVec);

                            sumRedVec = _mm256_add_epi32(sumRedVec, _mm256_cvtps_epi32(redFactorVec));
                            sumGreenVec = _mm256_add_epi32(sumGreenVec, _mm256_cvtps_epi32(greenFactorVec));
                            sumBlueVec = _mm256_add_epi32(sumBlueVec, _mm256_cvtps_epi32(blueFactorVec));
                        }
                    }
                    int* xx = (int*)&sumRedVec;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R += (int) xx[0];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].R += (int) xx[1];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].R += (int) xx[2];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].R += (int) xx[3];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].R += (int) xx[4];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].R += (int) xx[5];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].R += (int) xx[6];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].R += (int) xx[7];

                    int* xx2 = (int*)&sumGreenVec;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G += (int) xx2[0];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].G += (int) xx2[1];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].G += (int) xx2[2];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].G += (int) xx2[3];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].G += (int) xx2[4];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].G += (int) xx2[5];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].G += (int) xx2[6];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].G += (int) xx2[7];

                    int* xx3 = (int*)&sumBlueVec;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B += (int) xx3[0];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+1].B += (int) xx3[1];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+2].B += (int) xx3[2];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+3].B += (int) xx3[3];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+4].B += (int) xx3[4];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+5].B += (int) xx3[5];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+6].B += (int) xx3[6];
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH+7].B += (int) xx3[7];

                } else {
                    byte NEW_PIXEL;
                    NEW_PIXEL = (int) (p->PIXEL[i_HEIGHT][i_WIDTH].R * RED
                                       + p->PIXEL[i_HEIGHT][i_WIDTH].G * GREEN
                                       + p->PIXEL[i_HEIGHT][i_WIDTH].B * BLUE);
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R = NEW_PIXEL;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G = NEW_PIXEL;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B = NEW_PIXEL;
                }
            }
        }
    }

    stopTime(p->thread_num);
    pthread_exit(NULL);
}
// Convolution Function
int CONVOLUTION(BITMAP_FILE_HEADER *BFH, BITMAP_INFO_HEADER *BIH, FILE *fp,
		char * new_name, int filterSize, float factor, int threads_sum) {
	int i;
	int padding;
	int i_HEIGHT, i_WIDTH;

	tbyte **PIXEL, **PIXEL_OUT;

	//SINTHIKI POY IPOLOGIZI TO padding.
	if (((BIH->biWidth * 3) % 4) == 0)
		padding = 0;
	else
		padding = 4 - ((3 * BIH->biWidth) % 4);

	//DEZMEYI DINAMIKA XORO STIN MNIMI GIA NA KATAXORITHOY TA PIXEL THS EIKONAS.
	//DIMIOYRGI DINAMIKA ENA DIZDIASTATO PINAKA.
	if ((PIXEL = (tbyte **) malloc((BIH->biHeight) * sizeof(tbyte *))) == NULL)
		return (1);
	if ((PIXEL_OUT = (tbyte **) malloc((BIH->biHeight) * sizeof(tbyte *))) == NULL)
		return (1);
	for (i = 0; i < BIH->biHeight; i++){
		if ((PIXEL[i] = (tbyte *) malloc((BIH->biWidth + padding) * sizeof(tbyte))) == NULL)
			return (1);
		if ((PIXEL_OUT[i] = (tbyte *) malloc((BIH->biWidth + padding) * sizeof(tbyte))) == NULL)
			return (1);
	}

	//DIABAZI TA PIXEL APO TO ARXIO KAI TA KATAXORI STON PINAKA.
	for (i_WIDTH = 0; i_WIDTH < (BIH->biHeight); i_WIDTH++) {
		for (i_HEIGHT = 0; i_HEIGHT < (BIH->biWidth + padding); i_HEIGHT++) {
			//AN EXI TELIOSI H GRAMI THS EIKONAS DIABAZI ENA ENA TA BYTE TOY PADDING ALLIOS DIABAZI PIXEL.
			if (i_HEIGHT >= BIH->biWidth)
				fread((void *) &(PIXEL[i_WIDTH][i_HEIGHT]), sizeof(byte), 1,fp);
			else
				fread((void *) &(PIXEL[i_WIDTH][i_HEIGHT]), sizeof(tbyte), 1,fp);
		}
	}

	fclose(fp);

    pthread_t threads[threads_sum];
    ppArgs *p[threads_sum];
    int chunk_size = (int)(BIH->biWidth * BIH->biHeight) / threads_sum;

    for (i=0; i < threads_sum; i++){
        p[i] = (ppArgs *)malloc(sizeof(ppArgs));
        p[i]->thread_num = i;
        p[i]->PIXEL = PIXEL;
        p[i]->PIXEL_OUT = PIXEL_OUT;
        p[i]->filterSize = filterSize;
        p[i]->factor = factor;
        p[i]->startHeight = (chunk_size*i)/(int)BIH->biWidth;
        p[i]->startWidth = (chunk_size*i)%(int)BIH->biWidth;
        p[i]->width = BIH->biWidth;
        p[i]->height = BIH->biHeight;

        if (i<threads_sum-1) {
            p[i]->endHeight = (chunk_size*(i+1)-1)/(int)BIH->biWidth;
            p[i]->endWidth = (chunk_size*(i+1)-1)%(int)BIH->biWidth;
        } else {
            p[i]->endHeight = BIH->biHeight-1;
            p[i]->endWidth = BIH->biWidth;
        }
        pthread_create(&threads[i], NULL, pixelProcessor, (void*)p[i]);
    }

    for (i=0; i < threads_sum; i++){
        pthread_join(threads[i], NULL);
//        printf("Thread %d: ",i);elapsedTime(i);
    }

#ifdef DEBUG
	FILE *new_fp;
	new_fp=fopen(new_name,"wb");
	//DIMIOYRGA TO NEO FILE EIKONAS ME THN AKOLOYTHI SINARTISI.
	NEW_FILE_HEADER(BFH, BIH, new_fp);

	for(i_WIDTH=0; i_WIDTH<(BIH->biHeight); i_WIDTH++) {
		for(i_HEIGHT=0; i_HEIGHT<(BIH->biWidth + padding); i_HEIGHT++) {

			//AN EXI TELIOSI H GRAMI THS EIKONAS TOPOTHETI ENA ENA TA BYTE TOY PADDING ALLIOS TOPOTHETI PIXEL.
			if(i_HEIGHT>=BIH->biWidth)
			fwrite((void *)&(PIXEL_OUT[i_WIDTH][i_HEIGHT]),sizeof(byte),1,new_fp);
			else
			fwrite((void *)&(PIXEL_OUT[i_WIDTH][i_HEIGHT]),sizeof(tbyte),1,new_fp);
		}
	}

	//KLISIMO ARXIOY KAI APODEZMEYSI MNIMIS.
	fclose(new_fp);
#else
#endif

	for (i = 0; i < BIH->biHeight; i++)
		free(PIXEL[i]);

	free(PIXEL);

	return (0);
}
//SINARTISI POY DIMIOYRGA TO NEO FILE.
int NEW_FILE_HEADER(BITMAP_FILE_HEADER *BFH, BITMAP_INFO_HEADER *BIH,
		FILE *new_fp) {

	fwrite((void *) &(BFH->bfType1), sizeof(byte), 1, new_fp);
	fwrite((void *) &(BFH->bfType2), sizeof(byte), 1, new_fp);
	fwrite((void *) &(BFH->bfSize), sizeof(dword), 1, new_fp);
	fwrite((void *) &(BFH->bfReserved1), sizeof(word), 1, new_fp);
	fwrite((void *) &(BFH->bfReserved2), sizeof(word), 1, new_fp);
	fwrite((void *) &(BFH->dfOffBits), sizeof(dword), 1, new_fp);

	fwrite((void *) &(BIH->biSize), sizeof(dword), 1, new_fp);
	fwrite((void *) &(BIH->biWidth), sizeof(dword), 1, new_fp);
	fwrite((void *) &(BIH->biHeight), sizeof(dword), 1, new_fp);
	fwrite((void *) &(BIH->biPlanes), sizeof(word), 1, new_fp);
	fwrite((void *) &(BIH->biBitCount), sizeof(word), 1, new_fp);
	fwrite((void *) &(BIH->biCompression), sizeof(dword), 1, new_fp);
	fwrite((void *) &(BIH->biSizeImage), sizeof(dword), 1, new_fp);
	fwrite((void *) &(BIH->biXPelsPerMeter), sizeof(dword), 1, new_fp);
	fwrite((void *) &(BIH->biYPelsPerMeter), sizeof(dword), 1, new_fp);
	fwrite((void *) &(BIH->biClrUsed), sizeof(dword), 1, new_fp);
	fwrite((void *) &(BIH->biClrImportant), sizeof(dword), 1, new_fp);

	return (0);
}

int main(int argc, char *argv[]) {
	BITMAP_FILE_HEADER BFH;
	BITMAP_INFO_HEADER BIH;

	FILE *fp;
	
	// Filename for the output image
	char new_name[128];

	float factor = 0.0;
	int filterSize = 1;
	//./a.out filterSize image.bmp
	if (argc != 3){
		printf("Wrong Command Line. Format:./a.out FilterSize BMPimageName.bmp\n");
		return 0;
	}
	
#ifdef DEBUG			
			printf("Convolution process started...\n");
#endif
			startTime(0);
			// First Argument is for the Filter Size
			filterSize = atoi(argv[1]);
			for (factor = 0.00; factor <= 0.1; factor += 0.005) {
				fp = fopen(argv[2], "rb");
				//DIABAZI TO ARXIO KAI AN DEN EINAI APODEKTIS KATASTASIS TO PARALIPI
				if (load_HEADER(&BFH, &BIH, fp) == 1) continue;
				//DIMIOYRGA TON NEO ONOMA TOY ARXIOY
				sprintf(new_name, "output//conv_%dx%d_%3.2f_%s", filterSize,filterSize,factor, argv[2]);
#ifdef DEBUG
			printf("Conversion of %s: Factor:%f, Filter Size %dx%d:\n",argv[1],factor,filterSize,filterSize);
			LIST(&BFH, &BIH);
#else
#endif					
			if (CONVOLUTION(&BFH, &BIH, fp, new_name, filterSize, factor, 4) == 1) {
				printf("Ektropi, mi diathesimi mnimi");
				return (0);
			}
			}
			stopTime(0);	elapsedTime(0);
	return (0);

}
