unsigned long long keys[16] = {0};

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

}

unsigned long long SBox(unsigned long long text)
{

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

unsigned long long encrypt(unsigned long long text, char key)
{
    generateKeys(key);
    unsigned long long cipher = FesitelFunction(text, keys[0]);
    for (int i = 1; i < 15; i++)
    {
        cipher = FeistelFunction(cipher, keys[i]);
    }
    return cipher;
}

unsigned long long decrypt(unsigned long long cipher, char key)
{
    generateKeys(key);
    unsigned long long text = FesitelFunction(cipher, keys[15]);
    for (int i = 14; i >= 0; i--)
    {
        text = FeistelFunction(text, keys[i]);
    }
    return text;
}

void generateKeys(unsigned long long key)
{
    static const int keyShifts[16] = {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};
    key = PermutedChoice1(key);
    for (int i = 0; i < 16; i++)
    {
        unsigned long* keyPtr = (unsigned long*) &key;
        *keyPtr = (*keyPtr << keyShifts[i]) | (*keyPtr >> 32-keyShifts[i]);
        keyPtr++;
        *keyPtr = (*keyPtr << keyShifts[i]) | (*keyPtr >> 32-keyShifts[i]);
        keys[i] = PermutedChoice2(key);
    }
}

int main(int argc, char* argv[])
{
    return 0;
}
