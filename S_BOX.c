#include <stdio.h>  
// The main procedure of the S_BOX which will take 48 bit and return 32 bit
long long S_BOX(long long input);
// Leaf function for the S_BOX function
long S1_API(long S1_InputData);
long S2_API(long S2_InputData);
long S3_API(long S3_InputData);
long S4_API(long S4_InputData);
long S5_API(long S5_InputData);
long S6_API(long S6_InputData);
long S7_API(long S7_InputData);
long S8_API(long S8_InputData);

//Tables of the S_BOX
int S1[4][16] = { {14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7},
                {0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8},
                {4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0},
                {15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13} };
int S2[4][16] = { {15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10},
                {3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5},
                {0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15 },
                {13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9} };

int S3[4][16] = { {10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8},
                {13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1},
                {13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7},
                {1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12 } };

int S4[4][16] = { { 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15},
                {13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9},
                {10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4},
                {3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14 } };

int S5[4][16] = {{2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9 },
                {14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6},
                {4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14},
                {11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3 }};

int S6[4][16] = { {12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11 },
                {10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8},
                {9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6},
                {4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13}};

int S7[4][16] = { { 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1},
                {13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6},
                {1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2},
                {6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12} };

int S8[4][16] = { {13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7},
                {1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2 },
                {7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8},
                {2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11 }};



int main()
{ 
    //Example
    long long input = 0b000011000010110001110000110000011010110011011100;
    long long output = S_BOX(input);


}

long long S_BOX(long long input)
{
    
    long long output;
    int TotalInputSize = 48;
    int TotalOutputSize = 32;
    int BoxInputSize = 6;
    int BoxOutputSize = 4;
    int S1_Offset = 42;
    int S2_Offset = 36;
    int S3_Offset = 30;
    int S4_Offset = 24; 
    int S5_Offset = 18;
    int S6_Offset = 12;
    int S7_Offset = 6;
    int S8_Offset = 0;
    int S1_Output_Offset = 28;
    int S2_Output_Offset = 24;
    int S3_Output_Offset = 20;
    int S4_Output_Offset = 16;
    int S5_Output_Offset = 12;
    int S6_Output_Offset = 8;
    int S7_Output_Offset = 4;
    int S8_Output_Offset = 0;
    

    long S1_InputData = (input & (0b111111000000000000000000000000000000000000000000)) >> S1_Offset;
    long S2_InputData = (input & (0b000000111111000000000000000000000000000000000000)) >> S2_Offset;
    long S3_InputData = (input & (0b000000000000111111000000000000000000000000000000)) >> S3_Offset;
    long S4_InputData = (input & (0b000000000000000000111111000000000000000000000000)) >> S4_Offset;
    long S5_InputData = (input & (0b000000000000000000000000111111000000000000000000)) >> S5_Offset;
    long S6_InputData = (input & (0b000000000000000000000000000000111111000000000000)) >> S6_Offset;
    long S7_InputData = (input & (0b000000000000000000000000000000000000111111000000)) >> S7_Offset;
    long S8_InputData = (input & (0b000000000000000000000000000000000000000000111111)) >> S8_Offset;



    
    output = (S8_API(S8_InputData) << S8_Output_Offset) | ((S7_API(S7_InputData) << S7_Output_Offset)) | ((S6_API(S6_InputData) << S6_Output_Offset)) | (S5_API(S5_InputData) << S5_Output_Offset) | ((S4_API(S4_InputData) << S4_Output_Offset)) | ((S3_API(S3_InputData) << S3_Output_Offset)) | ((S2_API(S2_InputData) << S2_Output_Offset)) | ((S1_API(S1_InputData) << S1_Output_Offset));

    return output;
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


