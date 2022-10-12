#include <iostream>
#include <math.h>
#include <bitset>
using namespace std;

#define PLAIN_TEXT_SIZE	64

int initial_message_permutation[] =	   {58, 50, 42, 34, 26, 18, 10, 2,
										60, 52, 44, 36, 28, 20, 12, 4,
										62, 54, 46, 38, 30, 22, 14, 6,
										64, 56, 48, 40, 32, 24, 16, 8,
										57, 49, 41, 33, 25, 17,  9, 1,
										59, 51, 43, 35, 27, 19, 11, 3,
										61, 53, 45, 37, 29, 21, 13, 5,
										63, 55, 47, 39, 31, 23, 15, 7};

int final_message_permutation[] =  {40,  8, 48, 16, 56, 24, 64, 32,
									39,  7, 47, 15, 55, 23, 63, 31,
									38,  6, 46, 14, 54, 22, 62, 30,
									37,  5, 45, 13, 53, 21, 61, 29,
									36,  4, 44, 12, 52, 20, 60, 28,
									35,  3, 43, 11, 51, 19, 59, 27,
									34,  2, 42, 10, 50, 18, 58, 26,
									33,  1, 41,  9, 49, 17, 57, 25};


unsigned long long PermutedChoice1(unsigned long long key)
{
    
}

unsigned long long LeftCircularShift(unsigned long long key)
{
    
}

unsigned long long PermutedChoice2(unsigned long long key)
{

}
/*
Function To Initially Permute the plaintext before entering the feistel rounds
*/
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

unsigned long long SwapHalves(unsigned long long text)
{

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

}

unsigned long long SBox(unsigned long long text)
{

}

unsigned long long Permutation(unsigned long long text)
{

}

unsigned long long FeistelFunction(unsigned long long text, unsigned long long k)
{

}

char *encrypt(char *text, char key)
{

}

char *decrypt(char *cipher, char key)
{
    
}

int main(int argc, char* argv[])
{
	unsigned long long test =  8123362036854775807;
	unsigned long long answer = InitialPermutation(test);
	bitset<64> x(test);
	cout << "Original Text : " << x << endl;

	answer = InitialPermutation(test);
	bitset<64> y(answer);
	cout << "Output Of Permutation : " << y << endl;

	answer = InverseInitialPermutation(answer);
	bitset<64> z(answer);
	cout << "Reverse of the first Permutation : " << z;
    
	return 0;
}
