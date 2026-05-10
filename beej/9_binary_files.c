#include <stdio.h>

// int main(void)
// {
//     FILE *fp;
//     unsigned char bytes[6] = {5, 37, 0, 88, 255, 12};

//     fp = fopen("output.bin", "wb");  // wb mode for "write binary"!

//     // In the call to fwrite, the arguments are:
//     //
//     // * Pointer to data to write
//     // * Size of each "piece" of data
//     // * Count of each "piece" of data
//     // * FILE*

//     fwrite(bytes, sizeof(char), 6, fp);

//     fclose(fp);
// }

int main(void) {
  FILE *fp;
  unsigned char c;
  fp = fopen("output.bin", "rb");
  while (fread(&c, sizeof(char), 1, fp) > 0) {
    printf("%x\n", c);
  }
  fclose(fp);
}

// int main(void){
//     FILE *fp;
//     unsigned short v = 0x1234;
//     fp = fopen("output.bin", "wb");
//     fwrite(&v, sizeof v, 1, fp);
//     fclose(fp);
// }