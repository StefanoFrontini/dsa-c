#include <stdio.h>

int main(void) {
  FILE *fp;
  int x = 32;
  fp = fopen("output.txt", "w");
  //   fp = stdout;
  fputc('B', fp);
  fputc('\n', fp);
  fprintf(fp, "int x = %d\n", x);
  fputs("Hello world\n", fp);
  fclose(fp);
}

// int main(void) {
//   FILE *fp; // Variable to represent open file
//   int linecount = 0;
//   char buf[1024];

//   fp = fopen("hello.txt", "r"); // Open file for reading

//   while(fgets(buf, sizeof buf, fp) != NULL){
//     printf("%d: %s", ++linecount, buf);
//   }

// //   while ((c = fgetc(fp)) != EOF) {
// //     printf("%c", c); // Print char to stdout
// //   }

//   //   int c = fgetc(fp); // Read a single character

//   fclose(fp); // Close the file when done
// }