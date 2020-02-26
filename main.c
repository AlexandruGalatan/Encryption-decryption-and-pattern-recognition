#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ERROR 1

typedef struct image image;
typedef struct pixel pixel;
typedef struct detection detection;

struct pixel{
    unsigned char r, g, b;
};

struct image{
    FILE *IMAGE;
    unsigned int WIDTH, HEIGHT, PADDING;
    unsigned char *HEADER;
    pixel *PIX;

};

struct detection{
    float corelationScore;
    int x, y, draw, width, height;
    pixel *color;
};

unsigned int* XORSHIFT32(unsigned int seed, unsigned int n)
{
    unsigned int *R, r, i;

    R = (unsigned int*) malloc(n * sizeof(unsigned int));
    r = seed;

    if (!R)
    {
        printf("Nu s-a putut aloca memorie");
        exit(ERROR);
    }

    for (i = 0; i < n; i++)
    {
        r = r ^ r << 13;
        r = r ^ r >> 17;
        r = r ^ r << 5;
        R[i] = r;
    }
    return R;
}

unsigned int* generatePermutation(unsigned int *R, unsigned int WH)
{
    unsigned int *P, i;

    P = (unsigned int*) malloc(WH * sizeof(unsigned int));

    if (!P)
    {
        printf("Nu s-a putut aloca memorie");
        exit(ERROR);
    }

    for (i = 1; i <= WH - 1 ; i++)
    {
        P[i-1] = R[WH - i - 1] % (i + 1);
    }

    return P;
}

void scrambleImage(image **imageToScramble, unsigned int *P)
{
    unsigned int i, nr, WH;
    pixel AUX;
    image *img;

    img = *imageToScramble;
    WH = img->WIDTH * img->HEIGHT;

    for (i = 1; i <= WH - 1 ; i++)
    {
        nr = P[i-1];
        AUX = img->PIX[i];
        img->PIX[i] = img->PIX[nr];
        img->PIX[nr] = AUX;
    }
}

void unScrambleImage(image **imageToScramble, unsigned int *R)
{
    unsigned int i, nr, WH;
    pixel AUX;
    image *img;

    img = *imageToScramble;
    WH = img->WIDTH * img->HEIGHT;

    for (i = WH - 1; i >= 1; i--)
    {
        nr = R[i-1];
        AUX = img->PIX[i];
        img->PIX[i] = img->PIX[nr];
        img->PIX[nr] = AUX;
    }
}

void grayscaleImage(image **imageToGrayscale)
{
    unsigned int i, j;
    unsigned char newColor;
    pixel *CURRENT_PIXEL;
    image *img;

    img = *imageToGrayscale;

	for(i = 0; i < img->HEIGHT * img->WIDTH; i++)
	{
        CURRENT_PIXEL = img->PIX + i;
        newColor = 0.299 * CURRENT_PIXEL->r + 0.587 * CURRENT_PIXEL->g + 0.114 * CURRENT_PIXEL->b;
        CURRENT_PIXEL->r = CURRENT_PIXEL->g = CURRENT_PIXEL->b = newColor;
	}
}

void XORPX(pixel *PIX, unsigned int X)
{
    PIX->b ^= X & 0xFF;

    PIX->g ^= (X >> 8) & 0xFF;

    PIX->r ^= (X >> 16) & 0xFF;
}

void XORPP(pixel *P0, pixel *P1, pixel *P2)
{
    P0->b = P1->b ^ P2->b;
    P0->g = P1->g ^ P2->g;
    P0->r = P1->r ^ P2->r;
}

void substitute(image **imageToSubstitute, unsigned int SV, unsigned int *R)
{
    unsigned int i, j, WH;
    unsigned char newColor;
    pixel *CURRENT_PIXEL, *LAST_PIXEL;
    image *img;

    img = *imageToSubstitute;

    WH = img->WIDTH * img->HEIGHT;


    for (i = 0; i < WH; i++)
    {
        CURRENT_PIXEL = img->PIX + i;

        if (i == 0)
        {
            XORPX(CURRENT_PIXEL, SV);
            XORPX(CURRENT_PIXEL, R[WH - 1]);
        }
        else
        {
            LAST_PIXEL = img->PIX + i - 1;
            XORPP(CURRENT_PIXEL, LAST_PIXEL, CURRENT_PIXEL);
            XORPX(CURRENT_PIXEL, R[WH + i - 1]);
        }


    }
}

void invertedSubstitution(image **imageToSubstitute, unsigned int SV, unsigned int *R)
{
    unsigned int j, WH;
    int i;
    unsigned char newColor;
    pixel *CURRENT_PIXEL, *LAST_PIXEL;
    image *img;

    img = *imageToSubstitute;

    WH = img->WIDTH * img->HEIGHT;


    for (i = WH - 1; i >= 0; i--)
    {
        CURRENT_PIXEL = img->PIX + i;

        if (i == 0)
        {
            XORPX(CURRENT_PIXEL, SV);
            XORPX(CURRENT_PIXEL, R[WH - 1]);
        }
        else
        {
            LAST_PIXEL = img->PIX + i - 1;
            XORPP(CURRENT_PIXEL, LAST_PIXEL, CURRENT_PIXEL);
            XORPX(CURRENT_PIXEL, R[WH + i - 1]);
        }
    }
}

void encryption(image **img, unsigned int key, unsigned int SV)
{
    unsigned int *R, *P, WH, i;

    WH = (*img)->WIDTH * (*img)->HEIGHT;

    R = XORSHIFT32(key, 2 * WH - 1);
    P = generatePermutation(R, WH);

    printf("\nSe cripteaza imaginea...\n\n");
    scrambleImage(&(*img), P);
    substitute(&(*img), SV, R);
}

void decryption(image **img, unsigned int key, unsigned int SV)
{
    unsigned int *R, *P, WH, i;

    WH = (*img)->WIDTH * (*img)->HEIGHT;

    R = XORSHIFT32(key, 2 * WH - 1);
    P = generatePermutation(R, WH);

    printf("\nSe decripteaza imaginea...\n\n");
    invertedSubstitution(&(*img), SV, R);
    unScrambleImage(&(*img), P);
}

unsigned int getImageDetails(image** imageFile)
{
    image *img = *imageFile;
    unsigned int i;

    img->HEADER = (unsigned char*) malloc(sizeof(unsigned char) * 54);
    if (!img->HEADER)
    {
        printf("Nu s-a putut aloca memorie");
        exit(ERROR);
    }

    fseek(img->IMAGE, 0, SEEK_SET);
    for(i = 0; i < 54; i++)
        fread(img->HEADER + i, 1, 1, img->IMAGE);

    fseek(img->IMAGE, 18, SEEK_SET);
    fread(&img->WIDTH, sizeof(unsigned int), 1, img->IMAGE);
    fread(&img->HEIGHT, sizeof(unsigned int), 1, img->IMAGE);

    if(img->WIDTH % 4 != 0)
        img->PADDING = 4 - (3 * img->WIDTH) % 4;
    else
        img->PADDING = 0;

    return 0;
}

unsigned int liniarizeImage(image **imageFile)
{
    unsigned int i, j;
    pixel *CURRENT_PIXEL;
    image *img = *imageFile;

    img->PIX = (pixel*) malloc(sizeof(pixel) * (img->WIDTH * img->HEIGHT));

    if (!img->IMAGE)
    {
        printf("Lipseste fisierul imaginii");
        exit(ERROR);
    }
    if (!img->PIX)
    {
        printf("Nu s-a putut aloca memorie");
        exit(ERROR);
    }

    fseek(img->IMAGE, 54, SEEK_SET);

    for(i = 0; i < img->HEIGHT; i++)
    {
        for(j = 0; j < img->WIDTH; j++)
        {
            CURRENT_PIXEL = (img->PIX + (img->HEIGHT - i - 1) * img->WIDTH + j); //(img->PIX + i * img->WIDTH + j);//

            fread(&CURRENT_PIXEL->b, 1, 1, img->IMAGE);
            fread(&CURRENT_PIXEL->g, 1, 1, img->IMAGE);
            fread(&CURRENT_PIXEL->r, 1, 1, img->IMAGE);
        }
        fseek(img->IMAGE,img->PADDING,SEEK_CUR);
    }

    return 0;
}

image* readImage(char *imagePath)
{
    FILE* IMAGE;
    image *newImage;

    IMAGE = fopen(imagePath, "rb");

    if (!IMAGE)
    {
        printf("Nu s-a putut deschide imaginea %s.", imagePath);
        exit(ERROR);
    }

    newImage = (image*) malloc(sizeof(image));

    if (!newImage)
    {
        printf("Nu s-a putut aloca memorie");
        exit(ERROR);
    }

    newImage->IMAGE = IMAGE;

    getImageDetails(&newImage);

    liniarizeImage(&newImage);

    fclose(newImage->IMAGE);

    return newImage;
}

void outputImage(image* IMAGE, unsigned char *imagePath)
{
    pixel *CURRENT_PIXEL;
    FILE *OUT;
    int i, j, pad, zero;

    OUT = fopen(imagePath, "wb");
    zero = 0;

    for (i = 0; i< 54; i++)
        fwrite(IMAGE->HEADER + i, 1, 1, OUT);

    for(i = 0; i < IMAGE->HEIGHT; i++)
    {
        for(j = 0; j < IMAGE->WIDTH; j++)
        {
            CURRENT_PIXEL = (IMAGE->PIX + (IMAGE->HEIGHT - i - 1) * IMAGE->WIDTH + j);//(IMAGE->PIX + i * IMAGE->WIDTH + j);//

            fwrite(&CURRENT_PIXEL->b, 1, 1, OUT);
            fwrite(&CURRENT_PIXEL->g, 1, 1, OUT);
            fwrite(&CURRENT_PIXEL->r, 1, 1, OUT);
        }

        for(pad = 0; pad < IMAGE->PADDING; pad++)
            fwrite(&zero, 1, 1, OUT);
        //fseek(OUT,IMAGE->PADDING,SEEK_CUR);
    }
    fclose(OUT);
}

void readString(unsigned char **readTo, char* showString)
{
    unsigned char *newText;
    newText = (unsigned char*) malloc(256 * sizeof(unsigned char));

    if (!newText)
    {
        printf("Nu s-a putut aloca memorie");
        exit(ERROR);
    }

    printf("%s", showString);

    fgets(newText, 256, stdin);
    newText[strlen(newText) - 1] = '\0';

    //free(*readTo);

    *readTo = newText;
}

void chiTest(unsigned int *a, double f, char c)
{
    double chi, curent;
    unsigned int i;

    chi = 0;
    for (i = 0; i < 256; i++)
    {
        curent = (a[i] - f);
        curent *= curent;
        curent /= f;

        chi += curent;
    }

    printf("%c: %.2f\n", c, chi);
}

void chi2(image *IMAGE)
{
    pixel *CURRENT_PIXEL;
    double f, chi, curent;
    unsigned int i, j, col, *r, *g, *b;

    f = IMAGE->WIDTH * IMAGE->HEIGHT / 256;

    r = (unsigned int*) calloc(256, sizeof(unsigned int));
    g = (unsigned int*) calloc(256, sizeof(unsigned int));
    b = (unsigned int*) calloc(256, sizeof(unsigned int));

    if (!r || !g || !b)
    {
        printf("Nu s-a putut aloca memorie");
        exit(ERROR);
    }

    for(i = 0; i < IMAGE->HEIGHT; i++)
    {
        for(j = 0; j < IMAGE->WIDTH; j++)
        {
            CURRENT_PIXEL = (IMAGE->PIX + i * IMAGE->WIDTH + j);

            r[CURRENT_PIXEL->r]++;
            g[CURRENT_PIXEL->g]++;
            b[CURRENT_PIXEL->b]++;
        }
    }

    chiTest(r, f, 'R');
    chiTest(g, f, 'G');
    chiTest(b, f, 'B');

    free(r);
    free(g);
    free(b);
}

pixel* S(image* IMG, int x, int y)
{
    return (IMG->PIX + y * IMG->WIDTH + x);
}

float calculateCorelation(image *FI, image* TEMPLATE)
{
    int n, i, j;
    float S_, FI_, sigmaS, sigmaFI, aux, corr;

    n = TEMPLATE->HEIGHT * TEMPLATE->WIDTH;

    //calculeaza S_
    S_ = 0;
    for (i = 0; i < TEMPLATE->WIDTH * TEMPLATE->HEIGHT; i++)
        S_+= (TEMPLATE->PIX + i)->r;
    S_/=(TEMPLATE->WIDTH * TEMPLATE->HEIGHT);

    //calculeaza FI_
    FI_ = 0;
    for (i = 0; i < FI->WIDTH * FI->HEIGHT; i++)
        FI_+= (FI->PIX + i)->r;
    FI_/=(FI->WIDTH * FI->HEIGHT);

    //calculeaza sigmaS
    sigmaS = 0;
    for(i = 0; i < TEMPLATE->HEIGHT; i++)
    {
        for(j = 0; j < TEMPLATE->WIDTH; j++)
            {
                aux = S(TEMPLATE, i, j)->r - S_;
                aux*=aux;
                sigmaS+=aux;
            }
    }
    sigmaS/=(n-1);
    sigmaS = sqrt(sigmaS);

    //calculeaza sigmaFI
    sigmaFI = 0;
    for(i = 0; i < FI->HEIGHT; i++)
    {
        for(j = 0; j < FI->WIDTH; j++)
            {
                aux = S(FI, i, j)->r - S_;
                aux*=aux;
                sigmaFI+=aux;
            }
    }
    sigmaFI/=(n-1);
    sigmaFI = sqrt(sigmaFI);

    //calculeaza corr
    corr = 0;
    for(i = 0; i < TEMPLATE->HEIGHT; i++)
    {
        for(j = 0; j < TEMPLATE->WIDTH; j++)
        {
            aux = (S(FI, j, i)->r - FI_) * (S(TEMPLATE, j, i)->r - S_);
            aux/=(sigmaFI * sigmaS);
            corr+=aux;
        }
    }
    corr/=n;

    //printf(" sS: %3.2f, sFI: %3.2f ",sigmaS, sigmaFI);
    return corr;
}

void copyImage(image* source, image* target, int x, int y)
{
    int i, j, height, width;
    pixel *CURRENT_SOURCE_PIXEL, *CURRENT_TARGET_PIXEL;

    for(i = 0; i < (int)target->HEIGHT; i++)
    {
        for(j = 0; j < (int)target->WIDTH; j++)
        {

            CURRENT_TARGET_PIXEL = S(target, j, i);
            if (i + y > (int) source->HEIGHT || j + x > (int) source->WIDTH || i + y < 0 || j + x < 0)
                {
                    CURRENT_TARGET_PIXEL->r = 0;
                    CURRENT_TARGET_PIXEL->g = 0;
                    CURRENT_TARGET_PIXEL->b = 0;
                }
            else
                {
                    CURRENT_SOURCE_PIXEL = S(source, j + x, i + y);
                    CURRENT_TARGET_PIXEL->r = CURRENT_SOURCE_PIXEL->r;
                    CURRENT_TARGET_PIXEL->g = CURRENT_SOURCE_PIXEL->g;
                    CURRENT_TARGET_PIXEL->b = CURRENT_SOURCE_PIXEL->b;
                }

        }
    }

}

void drawRectangle(image *TARGET, detection *DETECTION)
{
    if (DETECTION->draw == 0)
        return;

    int i, j, x, y;
    pixel *CURRENT_TARGET_PIXEL;

    x = DETECTION->x;
    y = DETECTION->y;

    for(i = 0; i < DETECTION->height; i++)
    {
        for(j = 0; j < DETECTION->width; j++)
        {

            if ((i != 0 && i != DETECTION->height - 1 && j != 0 && j != DETECTION->width - 1) || i + y < 0 || i + y >= TARGET->HEIGHT || j + x < 0 || j + x >= TARGET->WIDTH)
                continue;
            CURRENT_TARGET_PIXEL = S(TARGET, j + x, i + y);
            CURRENT_TARGET_PIXEL->r = DETECTION->color->r;
            CURRENT_TARGET_PIXEL->g = DETECTION->color->g;
            CURRENT_TARGET_PIXEL->b = DETECTION->color->b;
        }
    }
}

detection* detectNumber(image* IMG, image* TEMPLATE, float PS, unsigned char R, unsigned char G, unsigned char B)
{
    int i, j, n, maxWidth, maxHeight, currentDetection;
    float S_, sigmaS, sigmaFI, aux, corr;
    image *FI;
    detection *detectionsFound;

    n = TEMPLATE->HEIGHT * TEMPLATE->WIDTH;

    int jumpSize = 10;

    detectionsFound = (detection*) malloc(jumpSize * sizeof(detection));
    if (!detectionsFound)
    {
        printf("Nu s-a putut aloca memorie");
        exit(ERROR);
    }
    detectionsFound->x = jumpSize; //aici pastram numarul de detectii gasite

    currentDetection = 0;

    //se creeaza o noua imagine in care vor fi salvate parti din imaginea cea mare
    FI = (image*) malloc(sizeof(image));
    if (!FI)
    {
        printf("Nu s-a putut aloca memorie");
        exit(ERROR);
    }

    FI->PIX = (pixel*) malloc(sizeof(pixel) * (TEMPLATE->WIDTH * TEMPLATE->HEIGHT));
    if (!FI->PIX)
    {
        printf("Nu s-a putut aloca memorie");
        exit(ERROR);
    }

    FI->WIDTH = TEMPLATE->WIDTH;
    FI->HEIGHT = TEMPLATE->HEIGHT;
    //


    maxHeight = IMG->HEIGHT + TEMPLATE->HEIGHT;
    maxWidth = IMG->WIDTH + TEMPLATE->WIDTH;

    for(i = -TEMPLATE->HEIGHT; i < maxHeight; i++)
    {
        for(j = -TEMPLATE->WIDTH; j < maxWidth ; j++)
        {
            copyImage(IMG, FI, j, i);
            corr = calculateCorelation(FI, TEMPLATE);

            if (corr > PS)
            {
                currentDetection++;
                if (currentDetection == detectionsFound->x)
                {
                    detectionsFound->x = detectionsFound->x + jumpSize;
                    detectionsFound = (detection*) realloc(detectionsFound, (detectionsFound->x + 1) * sizeof(detection));
                    if (!detectionsFound)
                    {
                        printf("Nu s-a putut realoca memorie");
                        exit(ERROR);
                    }
                }

                detectionsFound[currentDetection].color = (pixel*) malloc(sizeof(pixel));
                detectionsFound[currentDetection].color->r = R;
                detectionsFound[currentDetection].color->g = G;
                detectionsFound[currentDetection].color->b = B;

                detectionsFound[currentDetection].width = TEMPLATE->WIDTH;
                detectionsFound[currentDetection].height = TEMPLATE->HEIGHT;
                detectionsFound[currentDetection].draw = 1;
                detectionsFound[currentDetection].corelationScore = corr;
                detectionsFound[currentDetection].x = j;
                detectionsFound[currentDetection].y = i;
            }
        }
    }

    detectionsFound[0].x = currentDetection;
    detectionsFound = (detection*) realloc(detectionsFound, (detectionsFound->x + 1) * sizeof(detection));
    if (!detectionsFound)
    {
        printf("Nu s-a putut realoca memorie");
        exit(ERROR);
    }

    return detectionsFound;
}

void freeImage(image **img)
{
    free((*img)->PIX);
    free(*img);
}

int compareDetections (const void * a, const void * b) {
    int scorA, scorB;

    scorA = (*(detection*)a).corelationScore;
    scorB = (*(detection*)b).corelationScore;

    return scorA < scorB;
}

int overlap(detection *a, detection *b)
{
    int aLX, aLY, aRX, aRY, bLX, bLY, bRX, bRY;

    aLX = a->x;
    aLY = a->y;
    aRX = a->x + a->width;
    aRY = a->y + a->height;

    bLX = b->x;
    bLY = b->y;
    bRX = b->x + b->width;
    bRY = b->y + b->height;


    // e la stanga
    if (aLX > bRX || bLX > aRX)
        return 0;

    // e deasupra
    if (aLY > bRY || bLY > aRY)
        return 0;

    return 1;
}

int min(int a, int b)
{
    if (a < b)
        return a;
    return b;
}

int max(int a, int b)
{
    if (a > b)
        return a;
    return b;
}


int areaOfIntersection(detection *a, detection *b)
{
    if (overlap(a, b) == 0)
        return 0;

    int aLX, aLY, aRX, aRY, bLX, bLY, bRX, bRY, iLX, iLY, iRX, iRY, area;

    aLX = a->x;
    aLY = a->y;
    aRX = a->x + a->width;
    aRY = a->y + a->height;

    bLX = b->x;
    bLY = b->y;
    bRX = b->x + b->width;
    bRY = b->y + b->height;

    iLX = min(aLX, bLX);
    iLY = min(aLY, bLY);
    iRX = max(aRX, bRX);
    iRY = max(aRY, bRY);

    area = (iRX - iLX) * (iRY - iLY);

    return area;
}

int getDetectionArea(detection *a)
{
    int aLX, aLY, aRX, aRY, area;

    aLX = a->x;
    aLY = a->y;
    aRX = a->x + a->width;
    aRY = a->y + a->height;

    area = (aRX - aLX) * (aRY - aLY);

    return area;
}

void eliminateNonMaxs(detection* detectionArray)
{
    int i, j, area, overlap, areaI, areaJ, aux;

    printf("Se sorteaza detectiile...\n");
    qsort(detectionArray + 1, detectionArray->x, sizeof(detectionArray), compareDetections);

    for (i = 1; i < detectionArray->x; i++)
    {
        if ((detectionArray + i)->draw == 0)
            continue;
        for (j = i + 1; j < detectionArray->x; j++)
        {
            if ((detectionArray + j)->draw == 0)
                continue;
            area = areaOfIntersection(detectionArray + i, detectionArray + j);
            if (area > 0)
            {
                areaI = getDetectionArea(detectionArray + i);
                areaJ = getDetectionArea(detectionArray + j);
                aux = (areaI + areaJ - area);
                if (aux == 0)
                    continue;
                overlap = area / aux;

                if (overlap > 0.2)
                    (detectionArray + j)->draw = 0;
            }
        }
    }

}

detection* getDetectionArray(image **img)
{
    detection *detectionArray, *newDetectionArray;
    image *IMAGE, *TEMPLATE;
    float PS;
    int i, *r, *g, *b, len, j;
    char *fileName, *outStr;

    IMAGE = *img;
    PS = 0.5;

    r = (int*) malloc(9 * sizeof(int));
    g = (int*) malloc(9 * sizeof(int));
    b = (int*) malloc(9 * sizeof(int));
    fileName = (char*) malloc(20 * sizeof(char));
    outStr = (char*) malloc(30 * sizeof(char));

    if (!r || !g || !b || !fileName || !outStr)
    {
        printf("Nu s-a putut aloca memorie");
        exit(ERROR);
    }

    r[0] = 255; r[1] = 0;   r[2] = 0;   r[3] = 255; r[4] = 0;   r[5] = 192; r[6] = 255; r[7] = 128; r[8] = 128;
    g[0] = 255; g[1] = 255; g[2] = 255; g[3] = 0;   g[4] = 0;   g[5] = 192; g[6] = 140; g[7] = 0;   g[8] = 0;
    b[0] = 0;   b[1] = 0;   b[2] = 255; b[3] = 255; b[4] = 255; b[5] = 192; b[6] = 0;   b[7] = 128; b[8] = 0;

    printf("Cifra 0:\n");
    readString(&fileName, "    Sablonul pentru cifra 0: ");
    TEMPLATE = readImage(fileName);
    grayscaleImage(&TEMPLATE);
    detectionArray = detectNumber(IMAGE, TEMPLATE, PS, 255, 0, 0);
    printf("Detectii: %i; Total: %i\n", detectionArray->x, detectionArray->x);

    for(i = 1; i <= 9; i++)
    {
        printf("Cifra %i:\n",i);

        freeImage(&TEMPLATE);

        /*
        strcpy(fileName, "cifra");
        fileName[5] = '0' + i;
        fileName[6] = '\0';
        strcat(fileName,".bmp");
        */

        strcpy(outStr, "    Sablonul pentru cifra  : ");
        outStr[26] = '0' + i;

        readString(&fileName, outStr);

        TEMPLATE = readImage(fileName);
        grayscaleImage(&TEMPLATE);

        newDetectionArray = detectNumber(IMAGE, TEMPLATE, PS, r[i-1], g[i-1], b[i-1]);

        len = detectionArray->x;
        detectionArray->x+=newDetectionArray->x;
        printf("    Detectii: %i; Total: %i\n", newDetectionArray->x, detectionArray->x);

        detectionArray = (detection*) realloc(detectionArray, (detectionArray->x + 1) * sizeof(detection));
        if (!detectionArray)
        {
            printf("    Nu s-a putut realoca memorie");
            exit(ERROR);
        }

        for (j = 1; j <= newDetectionArray->x; j++)
        {
            *(detectionArray + j + len) = *(newDetectionArray + j);
        }

        free(newDetectionArray);
    }
    freeImage(&TEMPLATE);

    eliminateNonMaxs(detectionArray);

    return detectionArray;
}

void templateMatchingProgram()
{
    unsigned char *imagePath;
    int i;
    image *IMAGE;
    detection *detectionArray;

    readString(&imagePath, "Numele imaginii: ");
    IMAGE = readImage(imagePath);
    grayscaleImage(&IMAGE);

    detectionArray = getDetectionArray(&IMAGE);

    for(i = 1; i <= detectionArray[0].x; i++)
    {
        drawRectangle(IMAGE, detectionArray + i);
    }

    readString(&imagePath, "Nume output: ");
    outputImage(IMAGE, imagePath);
}

void cryptProgram()
{
    image *IMAGE;
    FILE *KEY;
    int R0, SV;
    char CHI;
    unsigned char *imagePath, *outputPath, *keyPath;

    readString(&imagePath, "Numele imaginii: ");
    IMAGE = readImage(imagePath);

    readString(&keyPath, "Numele cheii: ");

    KEY = fopen(keyPath, "rt");
    if (!KEY)
    {
        printf("Lipseste fisierul cheie");
        exit(ERROR);
    }
    fscanf (KEY, "%i", &R0);
    fscanf (KEY, "%i", &SV);
    fclose(KEY);

    printf("Afiseaza testul chi-patrat? (Y/N)\n");
    scanf("%c", &CHI);
    fflush(stdin);

    if (CHI == 'y')
        CHI = 'Y';


    if (CHI == 'Y')
    {
        printf("Inainte de criptare:\n");
        chi2(IMAGE);
    }

    encryption(&IMAGE, R0, SV);
    if (CHI == 'Y')
    {
        printf("Dupa criptare:\n");
        chi2(IMAGE);
        printf("\n");
    }


    readString(&outputPath, "Nume output: ");
    outputImage(IMAGE, outputPath);

    fclose(IMAGE->IMAGE);

    printf("\n");
}

void decryptProgram()
{
    image *IMAGE;
    FILE *KEY;
    int R0, SV;
    char CHI;
    unsigned char *imagePath, *outputPath, *keyPath;

    readString(&imagePath, "Numele imaginii criptate: ");
    IMAGE = readImage(imagePath);

    readString(&keyPath, "Numele cheii: ");

    KEY = fopen(keyPath, "rt");
    if (!KEY)
    {
        printf("Lipseste fisierul cheie");
        exit(ERROR);
    }
    fscanf (KEY, "%i", &R0);
    fscanf (KEY, "%i", &SV);
    fclose(KEY);

    printf("Afiseaza testul chi-patrat? (Y/N)\n");
    scanf("%c", &CHI);
    fflush(stdin);

    if (CHI == 'y')
        CHI = 'Y';


    if (CHI == 'Y')
    {
        printf("Inainte de decriptare:\n");
        chi2(IMAGE);
    }

    decryption(&IMAGE, R0, SV);

    if (CHI == 'Y')
    {
        printf("Dupa decriptare:\n");
        chi2(IMAGE);
        printf("\n");
    }


    readString(&outputPath, "Nume output: ");
    outputImage(IMAGE, outputPath);

    fclose(IMAGE->IMAGE);

    printf("\n");
}
int main()
{
    int INPUT;

    while (1)
    {
        printf("Introduceti o cifra:\n");
        printf("(1) - Criptare imagine; (2) - Decriptare imagine;\n(3) - Template matching; (4) - Iesire\n");
        scanf("%i", &INPUT);
        fflush(stdin);
        if (INPUT < 1 || INPUT > 3)
            break;

        switch(INPUT) {
            case 1 :
                cryptProgram();
            break;

            case 2:
                decryptProgram();
            break;

            case 3:
                templateMatchingProgram();
            break;

        }
    }

    return 0;
}
