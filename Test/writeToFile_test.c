#include <stdio.h>

#define filename "output.txt"

int writeFile(char message[1000]);
int appendFile(char message[1000]);
void readFile();

int main() {
   writeFile("write test : okay\n");
   appendFile("append test : okay\n");
   readFile();
   return 0;
}

int writeFile(char message[1000])
{
	FILE *fp;
   
   	fp = fopen(filename, "w"); // opening the file in write mode
   
  	if(fp == NULL) { // checking if file was opened successfully
      	printf("Error opening file!");
      	return 1;
   	}
   
   	fprintf(fp, "%s", message); // writing text to the file
   
   	fclose(fp); // closing the file

   	return 0;
}

int appendFile(char message[1000])
{
	FILE *fp;
   
   	fp = fopen(filename, "a"); // opening the file in append mode
   
  	if(fp == NULL) { // checking if file was opened successfully
      	printf("Error opening file!");
      	return 1;
   	}
   
   	fprintf(fp, "%s", message); // writing text to the file
   
   	fclose(fp); // closing the file

   	return 0;
}

void readFile()
{
	FILE *fp;
   char ch;

   fp = fopen(filename, "r"); // opening the file in read mode

   if(fp == NULL) { // checking if file was opened successfully
      printf("Error opening file!");
      return;
   }

   // reading and printing each character in the file
   while((ch = fgetc(fp)) != EOF) {
      putchar(ch);
   }

   fclose(fp); // closing the file
}