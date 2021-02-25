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
#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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
		char * new_name, int filterSize, float factor);
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

void pixelProcessor(tbyte **PIXEL, tbyte **PIXEL_OUT, int i_WIDTH, int i_HEIGHT, int filterSize, float factor){
	int i,j;
	PIXEL_OUT[i_WIDTH][i_HEIGHT].R = PIXEL[i_WIDTH][i_HEIGHT].R;
	PIXEL_OUT[i_WIDTH][i_HEIGHT].G = PIXEL[i_WIDTH][i_HEIGHT].G;
	PIXEL_OUT[i_WIDTH][i_HEIGHT].B = PIXEL[i_WIDTH][i_HEIGHT].B;
	for (j = i_HEIGHT-filterSize;  j<= i_HEIGHT + filterSize; j++)
		for (i = i_WIDTH-filterSize;  i<= i_WIDTH + filterSize; i++){
			PIXEL_OUT[i_WIDTH][i_HEIGHT].R += (int) (PIXEL[i][j].R) * (factor);
			PIXEL_OUT[i_WIDTH][i_HEIGHT].G += (int) (PIXEL[i][j].G) * (factor);
			PIXEL_OUT[i_WIDTH][i_HEIGHT].B += (int) (PIXEL[i][j].B) * (factor);
			
		}
}

// Convolution Function
int CONVOLUTION(BITMAP_FILE_HEADER *BFH, BITMAP_INFO_HEADER *BIH, FILE *fp,
		char * new_name, int filterSize, float factor) {
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
	//****************** START OF CODE OF INTEREST ***********************************//
	//PERNI ENA-ENA TA PIXEL THS EIKONAS.
	for (i_WIDTH = filterSize; i_WIDTH < (BIH->biHeight)-filterSize; i_WIDTH++) {
		for (i_HEIGHT = filterSize; i_HEIGHT < (BIH->biWidth)-filterSize; i_HEIGHT+=1) {
			if ((i_WIDTH > filterSize) && (i_WIDTH < (BIH->biHeight) - filterSize)
					&& (i_HEIGHT > filterSize) && (i_HEIGHT < (BIH->biWidth) - filterSize)) {
						pixelProcessor(PIXEL, PIXEL_OUT, i_WIDTH, i_HEIGHT, filterSize, factor);
			} else {
				byte NEW_PIXEL;
				NEW_PIXEL = (int) (PIXEL[i_WIDTH][i_HEIGHT].R * RED
						+ PIXEL[i_WIDTH][i_HEIGHT].G * GREEN
						+ PIXEL[i_WIDTH][i_HEIGHT].B * BLUE);
				PIXEL_OUT[i_WIDTH][i_HEIGHT].R = NEW_PIXEL;
				PIXEL_OUT[i_WIDTH][i_HEIGHT].G = NEW_PIXEL;
				PIXEL_OUT[i_WIDTH][i_HEIGHT].B = NEW_PIXEL;
			}
		}
	}
	//****************** END OF CODE OF INTEREST ***********************************//
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
			if (CONVOLUTION(&BFH, &BIH, fp, new_name, filterSize, factor) == 1) {
				printf("Ektropi, mi diathesimi mnimi");
				return (0);
			}
			}
			stopTime(0);	elapsedTime(0);
	return (0);

}
