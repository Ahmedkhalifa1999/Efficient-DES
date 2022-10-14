
#include <stdio.h>






unsigned long long ascii_to_hex(char c)
{
        unsigned long long  num = (int) c;
        if(num < 58 && num > 47)
        {
                return num - 48; 
        }
        if(num < 103 && num > 96)
        {
                return num - 87;
        }
        return num;
}

unsigned long long power(int base , int power)
{
    unsigned long long result=1;
    for(int i=0;i<power;i++)
    {
        result*=base;
    }
    return result;
}



int main(void)
{
    

    printf("%u",power(2,5));
  char data[17];
    unsigned long long bin=0;
  FILE *file;
  FILE *filePtr;
  char data2[16];

 
  file = fopen("C:\\Users\\user\\OneDrive\\Desktop\\hexabexa.hex", "r");

  if (file == NULL)
  {
    printf("Error opening file.\n");
    return 1;
  }
  

  int line = 0;

  while (!feof(file) && !ferror(file))
{
    if (fgets(data, 17, file) != NULL)
    {
        printf("%s \n",data);
        for(int loop =15;loop>=0;loop--)
        {
            unsigned long long hex=ascii_to_hex(data[loop]);
            printf("%x in hex\n",hex);
            bin|=hex<<((15-loop)*4);
            printf("%llx in bin in hex form\n",bin);
            printf("%llu in bin in dec form\n",bin);
            
        }
        //call tha function of encryption

    //////////////////////////////////////////SECTION OF WRITING START\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\    
    filePtr = fopen("C:\\Users\\user\\OneDrive\\Desktop\\hexabexaoutputa.hex", "a");
     
    if(filePtr == NULL)
    {
        printf("Error opening file, to write to it.");
    }
    else
    {
      for(int i=0;i<16;i++)
      {
        fputc(data[i], filePtr);
      }
        
    }
    //////////////////////////////////////////SECTION OF WRITING END [MOVE IT IN THE END OF CODE]\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ 
    }
      

}
  fclose(file);


/*
char receivedByte[] = "0x1F293C";
char *p;
int intNumber = strtol(receivedByte, &p, 16);
printf("The received number is: %x.\n", intNumber);
*/
  return 0;
}
