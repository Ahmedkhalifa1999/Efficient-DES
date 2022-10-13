#include <stdio.h>
int main(void)
{

  char data[8];
    unsigned long long bin;
  FILE *file;
  char data2[8];
 
  file = fopen("C:\\Users\\user\\OneDrive\\Desktop\\test.txt", "r");

  if (file == NULL)
  {
    printf("Error opening file.\n");
    return 1;
  }
  

  int line = 0;

  while (!feof(file) && !ferror(file))
{
    if (fgets(data, 8, file) != NULL)
    {
        
        
        //printf("%o",bin);
        memcpy(&bin, data, sizeof bin);
        //SHABAAAAAAAAAAAAAAAAAAAAAB 
        //l bin howa l unsigned long long elli shayel l data 5alas l 7aga et7attet feeh

        //da print bashof beeh l data ka string
        printf("%s \n",data);
        //da knt ba validate beeh l conversion
        printf("%llu",bin);
        //hna knt ba3ml validation tany enni lmma a7awel le string mfish sha2laba 

        //GOOD LUCK!!!!!!!
        memcpy(data2,&bin,sizeof bin);
        printf("%s \n",data2);
        
    }
      

}
  fclose(file);


  return 0;
}
