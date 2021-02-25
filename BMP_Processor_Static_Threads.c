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
            for (i_WIDTH=0 ; i_WIDTH < p->width ; i_WIDTH++) {
                if ((i_HEIGHT > p->filterSize) && (i_HEIGHT < (p->height) - p->filterSize)
                    && (i_WIDTH > p->filterSize) && (i_WIDTH < (p->width) - p->filterSize)) {
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R = p->PIXEL[i_HEIGHT][i_WIDTH].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G = p->PIXEL[i_HEIGHT][i_WIDTH].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B = p->PIXEL[i_HEIGHT][i_WIDTH].B;
                    for (j = i_HEIGHT-p->filterSize;  j<= i_HEIGHT + p->filterSize; j++)
                        for (i = i_WIDTH-p->filterSize;  i<= i_WIDTH + p->filterSize; i++){
                            p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R += (int) (p->PIXEL[j][i].R) * (p->factor);
                            p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G += (int) (p->PIXEL[j][i].G) * (p->factor);
                            p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B += (int) (p->PIXEL[j][i].B) * (p->factor);
                        }
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
            for (i_WIDTH=p->startWidth ; i_WIDTH <= p->endWidth ; i_WIDTH++) {
                if ((i_HEIGHT > p->filterSize) && (i_HEIGHT < (p->height) - p->filterSize)
                    && (i_WIDTH > p->filterSize) && (i_WIDTH < (p->width) - p->filterSize)) {
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R = p->PIXEL[i_HEIGHT][i_WIDTH].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G = p->PIXEL[i_HEIGHT][i_WIDTH].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B = p->PIXEL[i_HEIGHT][i_WIDTH].B;
                    for (j = i_HEIGHT-p->filterSize;  j<= i_HEIGHT + p->filterSize; j++)
                        for (i = i_WIDTH-p->filterSize;  i<= i_WIDTH + p->filterSize; i++){
                            p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R += (int) (p->PIXEL[j][i].R) * (p->factor);
                            p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G += (int) (p->PIXEL[j][i].G) * (p->factor);
                            p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B += (int) (p->PIXEL[j][i].B) * (p->factor);
                        }
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
            for (i_WIDTH=p->startWidth ; i_WIDTH < p->width ; i_WIDTH++) {
                if ((i_HEIGHT > p->filterSize) && (i_HEIGHT < (p->height) - p->filterSize)
                    && (i_WIDTH > p->filterSize) && (i_WIDTH < (p->width) - p->filterSize)) {
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R = p->PIXEL[i_HEIGHT][i_WIDTH].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G = p->PIXEL[i_HEIGHT][i_WIDTH].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B = p->PIXEL[i_HEIGHT][i_WIDTH].B;
                    for (j = i_HEIGHT-p->filterSize;  j<= i_HEIGHT + p->filterSize; j++)
                        for (i = i_WIDTH-p->filterSize;  i<= i_WIDTH + p->filterSize; i++){
                            p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R += (int) (p->PIXEL[j][i].R) * (p->factor);
                            p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G += (int) (p->PIXEL[j][i].G) * (p->factor);
                            p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B += (int) (p->PIXEL[j][i].B) * (p->factor);
                        }
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
            for (i_WIDTH=0 ; i_WIDTH < p->endWidth ; i_WIDTH++) {
                if ((i_HEIGHT > p->filterSize) && (i_HEIGHT < (p->height) - p->filterSize)
                    && (i_WIDTH > p->filterSize) && (i_WIDTH < (p->width) - p->filterSize)) {
//                    printf("%d  %d\n",i_HEIGHT,i_WIDTH);
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R = p->PIXEL[i_HEIGHT][i_WIDTH].R;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G = p->PIXEL[i_HEIGHT][i_WIDTH].G;
                    p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B = p->PIXEL[i_HEIGHT][i_WIDTH].B;
                    for (j = i_HEIGHT-p->filterSize;  j<= i_HEIGHT + p->filterSize; j++)
                        for (i = i_WIDTH-p->filterSize;  i<= i_WIDTH + p->filterSize; i++){
                            p->PIXEL_OUT[i_HEIGHT][i_WIDTH].R += (int) (p->PIXEL[j][i].R) * (p->factor);
                            p->PIXEL_OUT[i_HEIGHT][i_WIDTH].G += (int) (p->PIXEL[j][i].G) * (p->factor);
                            p->PIXEL_OUT[i_HEIGHT][i_WIDTH].B += (int) (p->PIXEL[j][i].B) * (p->factor);
                        }
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
			if (CONVOLUTION(&BFH, &BIH, fp, new_name, filterSize, factor, 1) == 1) {
				printf("Ektropi, mi diathesimi mnimi");
				return (0);
			}
			}
			stopTime(0);	elapsedTime(0);
	return (0);

}
