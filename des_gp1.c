#define MUSK 0x8000000000000000

int message_expansion[] =  {32,  1,  2,  3,  4,  5,
							 4,  5,  6,  7,  8,  9,
							 8,  9, 10, 11, 12, 13,
							12, 13, 14, 15, 16, 17,
							16, 17, 18, 19, 20, 21,
							20, 21, 22, 23, 24, 25,
							24, 25, 26, 27, 28, 29,
							28, 29, 30, 31, 32,  1};

unsigned long long PermutedChoice1(unsigned long long key)//Amin
{
    
}

unsigned long long LeftCircularShift(unsigned long long key)
{
    
}

unsigned long long PermutedChoice2(unsigned long long key)//Amin
{

}

unsigned long long InitialPermutation(unsigned long long text)
{

}

unsigned long long SwapHalves(unsigned long long text)
{

}

unsigned long long InverseInitialPermutation(unsigned long long text)
{

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
    return 0;
}
