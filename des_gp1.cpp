/* Use instructions:
 * -Needs to be compiled with -pthread flag to link POSIX threads library
 * -Compiling with -O3 flag gives a 3-fold performance improvement
 * -Compilation Command: g++ des_gp1.cpp -pthread -O3 -o des_gp1
 */

/* Compile Time Configuration */
#define OUTPUT_HEX 0
#define OUTPUT_TIME_TAKEN 0
#define ERROR_CHECKS 0

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#if OUTPUT_TIME_TAKEN
#include <time.h>
#endif

#define BLOCK_SIZE 2048
#define THREAD_COUNT 16

#define PLAIN_TEXT_SIZE	64
#define MUSK 0x8000000000000000

unsigned long long keys[16] = {0};

const unsigned char S1[4][16] = { {14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7},
                                  {0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8},
                                  {4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0},
                                  {15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13} };
const unsigned char S2[4][16] = { {15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10},
                                  {3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5},
                                  {0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15 },
                                  {13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9} };

const unsigned char S3[4][16] = { {10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8},
                                  {13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1},
                                  {13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7},
                                  {1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12 } };

const unsigned char S4[4][16] = { { 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15},
                                  {13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9},
                                  {10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4},
                                  {3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14 } };

const unsigned char S5[4][16] = {{2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9 },
                                 {14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6},
                                 {4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14},
                                 {11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3 }};

const unsigned char S6[4][16] = { {12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11 },
                                  {10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8},
                                  {9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6},
                                  {4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13}};

const unsigned char S7[4][16] = { { 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1},
                                  {13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6},
                                  {1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2},
                                  {6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12} };

const unsigned char S8[4][16] = { {13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7},
                                  {1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2 },
                                  {7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8},
                                  {2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11 }};


const unsigned char initial_message_permutation[] =	   {58, 50, 42, 34, 26, 18, 10, 2,
										                60, 52, 44, 36, 28, 20, 12, 4,
										                62, 54, 46, 38, 30, 22, 14, 6,
										                64, 56, 48, 40, 32, 24, 16, 8,
										                57, 49, 41, 33, 25, 17,  9, 1,
										                59, 51, 43, 35, 27, 19, 11, 3,
										                61, 53, 45, 37, 29, 21, 13, 5,
										                63, 55, 47, 39, 31, 23, 15, 7};

const unsigned char final_message_permutation[] =  {40,  8, 48, 16, 56, 24, 64, 32,
									                39,  7, 47, 15, 55, 23, 63, 31,
									                38,  6, 46, 14, 54, 22, 62, 30,
									                37,  5, 45, 13, 53, 21, 61, 29,
									                36,  4, 44, 12, 52, 20, 60, 28,
									                35,  3, 43, 11, 51, 19, 59, 27,
									                34,  2, 42, 10, 50, 18, 58, 26,
									                33,  1, 41,  9, 49, 17, 57, 25};

const unsigned char message_expansion[] =  {32,  1,  2,  3,  4,  5,
							                 4,  5,  6,  7,  8,  9,
							                 8,  9, 10, 11, 12, 13,
							                12, 13, 14, 15, 16, 17,
							                16, 17, 18, 19, 20, 21,
							                20, 21, 22, 23, 24, 25,
							                24, 25, 26, 27, 28, 29,
							                28, 29, 30, 31, 32,  1};

const unsigned char keyPC1[56] = { 57, 49, 41, 33, 25, 17,  9,
                                    1, 58, 50, 42, 34, 26, 18,
                                   10,  2, 59, 51, 43, 35, 27,
                                   19, 11,  3, 60, 52, 44, 36,
                                   63, 55, 47, 39, 31, 23, 15,
                                    7, 62, 54, 46, 38, 30, 22,
                                   14,  6, 61, 53, 45, 37, 29,
                                   21, 13,  5, 28, 20, 12,  4 };

const unsigned char keyPC2[48] = { 14, 17, 11, 24, 1,  5,  3,  28,
                                   15, 6,  21, 10, 23, 19, 12, 4,
                                   26, 8,  16, 7,  27, 20, 13, 2,
                                   41, 52, 31, 37, 47, 55, 30, 40,
                                   51, 45, 33, 48, 44, 49, 39, 56,
                                   34, 53, 46, 42, 50, 36, 29, 32 };

unsigned long long PermutedChoice1(unsigned long long key)
{
    unsigned long long PC1 = 0;
    
    for (unsigned char i = 0; i < 56; i++)
    {
        PC1 |= ((key >> (64 - keyPC1[i])) & 0x01) << 55-i;
    }
    return PC1;
}

unsigned long long PermutedChoice2(unsigned long long key)
{
    unsigned long long PC2 = 0;
    
    for (unsigned char i = 0; i < 48; i++)
    {
        PC2 |= ((key >> (56 - keyPC2[i])) & 0x01) << 47-i;
    }
    return PC2;

}

unsigned long long InitialPermutation(unsigned long long text)
{
	unsigned long long temp = 0;
	unsigned long long bitVal = 0;
	for(unsigned long long i = 0 ; i < PLAIN_TEXT_SIZE ; i++)
	{
		bitVal = (text & (1ULL << (initial_message_permutation[i]-1) ) ) >> (initial_message_permutation[i]-1); 
		temp |= bitVal << i;
	}
	return temp;
}

unsigned long long InverseInitialPermutation(unsigned long long text)
{
	unsigned long long temp = 0;
	unsigned long long bitVal = 0;
	for(int i = 0; i < PLAIN_TEXT_SIZE; i++)
	{	
		bitVal = (text & (1ULL << (final_message_permutation[i]-1) ) ) >> (final_message_permutation[i]-1);
		temp |=  bitVal << i; 
	}
	return temp;
}

unsigned long long ExpansionPermutation(unsigned long long text)
{
    char i,position;
	unsigned long long expended = 0;
	unsigned long long shifted = text << 32;
	for(i=0;i<48;i++){
		position = message_expansion[i] - 1;
		unsigned long long singlebit = shifted << position;
		expended = expended | ((singlebit & MUSK) >> 16 + i);
	}
	return expended;
}


long S1_API(long S1_InputData)
{
    int row = ((S1_InputData & (0b100000)) >> 4) + (S1_InputData & 0b1);
    int coloumn = ((S1_InputData & (0b011110)) >> 1);

    return S1[row][coloumn];
    
}

long S2_API(long S2_InputData)
{
    int row = ((S2_InputData & (0b100000)) >> 4) + (S2_InputData & 0b1);
    int coloumn = ((S2_InputData & (0b011110)) >> 1);

    return S2[row][coloumn];

}

long S3_API(long S3_InputData)
{
    int row = ((S3_InputData & (0b100000)) >> 4) + (S3_InputData & 0b1);
    int coloumn = ((S3_InputData & (0b011110)) >> 1);

    return S3[row][coloumn];

}
long S4_API(long S4_InputData)
{
    int row = ((S4_InputData & (0b100000)) >> 4) + (S4_InputData & 0b1);
    int coloumn = ((S4_InputData & (0b011110)) >> 1);

    return S4[row][coloumn];

}
long S5_API(long S5_InputData)
{
    int row = ((S5_InputData & (0b100000)) >> 4) + (S5_InputData & 0b1);
    int coloumn = ((S5_InputData & (0b011110)) >> 1);

    return S5[row][coloumn];

}
long S6_API(long S6_InputData)
{
    int row = ((S6_InputData & (0b100000)) >> 4) + (S6_InputData & 0b1);
    int coloumn = ((S6_InputData & (0b011110)) >> 1);

    return S6[row][coloumn];

}
long S7_API(long S7_InputData)
{
    int row = ((S7_InputData & (0b100000)) >> 4) + (S7_InputData & 0b1);
    int coloumn = ((S7_InputData & (0b011110)) >> 1);

    return S7[row][coloumn];

}
long S8_API(long S8_InputData)
{
    int row = ((S8_InputData & (0b100000)) >> 4) + (S8_InputData & 0b1);
    int coloumn = ((S8_InputData & (0b011110)) >> 1);

    return S8[row][coloumn];

}

unsigned long long SBox(unsigned long long text)
{
    long long output;
    static const int TotalInputSize = 48;
    static const int TotalOutputSize = 32;
    static const int BoxInputSize = 6;
    static const int BoxOutputSize = 4;
    static const int S1_Offset = 42;
    static const int S2_Offset = 36;
    static const int S3_Offset = 30;
    static const int S4_Offset = 24; 
    static const int S5_Offset = 18;
    static const int S6_Offset = 12;
    static const int S7_Offset = 6;
    static const int S8_Offset = 0;
    static const int S1_Output_Offset = 28;
    static const int S2_Output_Offset = 24;
    static const int S3_Output_Offset = 20;
    static const int S4_Output_Offset = 16;
    static const int S5_Output_Offset = 12;
    static const int S6_Output_Offset = 8;
    static const int S7_Output_Offset = 4;
    static const int S8_Output_Offset = 0;
    

    long S1_InputData = (text & (0b111111000000000000000000000000000000000000000000)) >> S1_Offset;
    long S2_InputData = (text & (0b000000111111000000000000000000000000000000000000)) >> S2_Offset;
    long S3_InputData = (text & (0b000000000000111111000000000000000000000000000000)) >> S3_Offset;
    long S4_InputData = (text & (0b000000000000000000111111000000000000000000000000)) >> S4_Offset;
    long S5_InputData = (text & (0b000000000000000000000000111111000000000000000000)) >> S5_Offset;
    long S6_InputData = (text & (0b000000000000000000000000000000111111000000000000)) >> S6_Offset;
    long S7_InputData = (text & (0b000000000000000000000000000000000000111111000000)) >> S7_Offset;
    long S8_InputData = (text & (0b000000000000000000000000000000000000000000111111)) >> S8_Offset;



    
    output = (S8_API(S8_InputData) << S8_Output_Offset) | ((S7_API(S7_InputData) << S7_Output_Offset)) | ((S6_API(S6_InputData) << S6_Output_Offset)) | (S5_API(S5_InputData) << S5_Output_Offset) | ((S4_API(S4_InputData) << S4_Output_Offset)) | ((S3_API(S3_InputData) << S3_Output_Offset)) | ((S2_API(S2_InputData) << S2_Output_Offset)) | ((S1_API(S1_InputData) << S1_Output_Offset));

    return output;
}

unsigned long long Permutation(unsigned long long text)
{
    static const int table[] = {16,  7, 20, 21, 29, 12, 28, 17,
								1, 15, 23, 26, 5, 18, 31, 10,
								2,  8, 24, 14, 32, 27,  3,  9,
							    19, 13, 30,  6, 22, 11,  4, 25};
    unsigned long long permuted = 0;
    for (int i = 0; i < 32; i++)
    {
        permuted |= ((text >> (32 - table[i])) & 0x01) << 31-i;
    }
    return permuted;
}

unsigned long long FeistelFunction(unsigned long long text, unsigned long long key)
{
    return (Permutation(SBox(ExpansionPermutation(text & 0xFFFFFFFF) ^ key)) ^ (text >> 32)) | (text << 32);
}

void generateKeys(unsigned long long key)
{
    static const int keyShifts[16] = {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};
    key = PermutedChoice1(key);
    for (int i = 0; i < 16; i++)
    {
        unsigned long long leftKey = ((key & 0x00FFFFFFF0000000) << keyShifts[i]) | ((key & 0x00FFFFFFF0000000) >> 28-keyShifts[i]);
        leftKey &= 0x00FFFFFFF0000000;

        unsigned long long rightKey = ((key & 0x000000000FFFFFFF) << keyShifts[i]) | ((key & 0x000000000FFFFFFF) >> 28-keyShifts[i]);
        rightKey &= 0x000000000FFFFFFF;

        key =  leftKey | rightKey;
        
        keys[i] = PermutedChoice2(key);
    }
}

unsigned long long encrypt(unsigned long long text)
{
    text = InitialPermutation(text);
    unsigned long long cipher = FeistelFunction(text, keys[0]);
    for (int i = 1; i < 16; i++)
    {
        cipher = FeistelFunction(cipher, keys[i]);
    }
    cipher = (cipher << 32) | (cipher >> 32);
    return InverseInitialPermutation(cipher);
}

unsigned long long decrypt(unsigned long long cipher)
{
    cipher = InitialPermutation(cipher);
    unsigned long long text = FeistelFunction(cipher, keys[15]);
    for (int i = 14; i >= 0; i--)
    {
        text = FeistelFunction(text, keys[i]);
    }
    text = (text << 32) | (text >> 32);
    return InverseInitialPermutation(text);
}

unsigned long long IHateEndinanness(unsigned long long text)
{
    text = (text << 32) | (text >> 32);
    text = ((text << 16) & 0xFFFF0000FFFF0000) | ((text >> 16) & 0x0000FFFF0000FFFF);
    text = ((text << 8) & 0xFF00FF00FF00FF00) | ((text >> 8) & 0x00FF00FF00FF00FF);
    return text;
}

unsigned long long data[THREAD_COUNT][BLOCK_SIZE];
#if OUTPUT_HEX
char cipherHex[THREAD_COUNT][17*BLOCK_SIZE + 1];
#endif

typedef struct {
    char ThreadNumber;
    int maximum;
} ThreadArgs;

void * encryptBlock(void *args)
{
    char threadNumber = ((ThreadArgs *)args)->ThreadNumber;
    int maximum = ((ThreadArgs *)args)->maximum;
    for (int i = 0; i < maximum; i++)
    {
        data[threadNumber][i] = IHateEndinanness(data[threadNumber][i]);
        data[threadNumber][i] = encrypt(data[threadNumber][i]);
        data[threadNumber][i] = IHateEndinanness(data[threadNumber][i]);
    }
    #if OUTPUT_HEX
    int i;
    for (i = 0; i < maximum; i++)
    {
        sprintf(&cipherHex[threadNumber][17*i], "%016llX\n", data[threadNumber][i]);
    }
    #endif
    return NULL;
}

void * decryptBlock(void *args)
{
    char threadNumber = ((ThreadArgs *)args)->ThreadNumber;
    int maximum = ((ThreadArgs *)args)->maximum;
    for (int i = 0; i < maximum; i++)
    {
        data[threadNumber][i] = IHateEndinanness(data[threadNumber][i]);
        data[threadNumber][i] = decrypt(data[threadNumber][i]);
        data[threadNumber][i] = IHateEndinanness(data[threadNumber][i]);
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    #if OUTPUT_TIME_TAKEN
    clock_t start = clock();
    #endif
    if (argc == 5 && strcmp(argv[1], "encrypt") == 0)
    {
        FILE *keyFile = fopen(argv[3], "r");
        unsigned long long key = 0;
        #if ERROR_CHECKS
        if (keyFile == NULL)
        { 
            printf("No Key File\n");
            return 0;
        }
        #endif
        fscanf(keyFile, "%016llX", &key);
        generateKeys(key);

        FILE* textFile = fopen(argv[2], "rb");
        FILE* cipherFile = fopen(argv[4], "wb");
        #if OUTPUT_HEX
        FILE* cipherHexFile = fopen("cipherHEX.txt", "w");
        #endif
        #if ERROR_CHECKS
        if (textFile == NULL)
        {
            printf("No Plain Text File\n");
            return 0;
        }
        if (cipherFile == NULL)
        {
            printf("Cannot Create Cipher File\n");
            return 0;
        }
        #endif
        size_t blockCount = fread((void*) data, sizeof(unsigned long long), THREAD_COUNT*BLOCK_SIZE, textFile);
        while (blockCount > 0) {
            int maximum = blockCount < THREAD_COUNT*BLOCK_SIZE? blockCount:THREAD_COUNT*BLOCK_SIZE;
            int remaining = maximum;
            pthread_t threads[THREAD_COUNT];
            ThreadArgs args[THREAD_COUNT];
            int threadCount = 0;
            for (int i = 0; i < THREAD_COUNT; i++)
            {
                args[i].ThreadNumber = i;
                args[i].maximum = remaining < BLOCK_SIZE? remaining:BLOCK_SIZE;
                pthread_create(&threads[i], NULL, &encryptBlock, (void *)&args[i]);
                remaining -= args[i].maximum;
                threadCount++;
                if (remaining == 0) break;
            }
            for (int i = 0; i < threadCount; i++)
            {
                pthread_join(threads[i], NULL);
            }

            fwrite((const void*) data, sizeof(unsigned long long), maximum, cipherFile);
            #if OUTPUT_HEX
            for (int i = 0; i < THREAD_COUNT; i++)
            {
                fputs(cipherHex[i], cipherHexFile);
            }
            #endif
            
            blockCount = fread((void*) data, sizeof(unsigned long long), THREAD_COUNT*BLOCK_SIZE, textFile);
        }
        fclose(keyFile);
        fclose(textFile);
        fclose(cipherFile);
        #if OUTPUT_HEX
        fclose(cipherHexFile);
        #endif
        printf("Encryption Done\n");
    }
    else if(argc == 5 && strcmp(argv[1], "decrypt") == 0)
    {
        FILE *keyFile = fopen(argv[3], "r");
        unsigned long long key = 0;
        #if ERROR_CHECKS
        if (keyFile == NULL)
        { 
            printf("No Key File\n");
            return 0;
        }
        #endif
        fscanf(keyFile, "%llX", &key);
        generateKeys(key);

        FILE* textFile = fopen(argv[4], "wb");
        FILE* cipherFile = fopen(argv[2], "rb");
        #if ERROR_CHECKS
        if (textFile == NULL)
        {
            printf("Cannot Create Plain Text File\n");
            return 0;
        }
        if (cipherFile == NULL)
        {
            printf("No Cipher File\n");
            return 0;
        }
        #endif
        size_t blockCount = fread((void*) data, sizeof(unsigned long long), THREAD_COUNT*BLOCK_SIZE, cipherFile);
        while (blockCount > 0) {
            int maximum = blockCount < THREAD_COUNT*BLOCK_SIZE? blockCount:THREAD_COUNT*BLOCK_SIZE;
            int remaining = maximum;
            pthread_t threads[THREAD_COUNT];
            ThreadArgs args[THREAD_COUNT];
            int threadCount = 0;
            for (int i = 0; i < THREAD_COUNT; i++)
            {
                args[i].ThreadNumber = i;
                args[i].maximum = remaining < BLOCK_SIZE? remaining:BLOCK_SIZE;
                pthread_create(&threads[i], NULL, &decryptBlock, (void *)&args[i]);
                remaining -= args[i].maximum;
                threadCount++;
                if (remaining == 0) break;
            }

            for (int i = 0; i < threadCount; i++)
            {
                pthread_join(threads[i], NULL);
            }

            fwrite((const void*) data, sizeof(unsigned long long), maximum, textFile);
            blockCount = fread((void*) data, sizeof(unsigned long long), THREAD_COUNT*BLOCK_SIZE, cipherFile);
        }
        fclose(keyFile);
        fclose(textFile);
        fclose(cipherFile);
        printf("Decryption Done\n");
    }
    else printf("Invalid Arguments\n");
    #if OUTPUT_TIME_TAKEN
    clock_t end = clock();
    printf("Process took %f seconds\n", (float)(end-start)/CLOCKS_PER_SEC);
    #endif
    return 0;
}