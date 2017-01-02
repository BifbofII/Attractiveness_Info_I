/******************************************************************************************
 * Name: attractiveness.h
 ******************************************************************************************
 * Author: Christoph Jabs
 ******************************************************************************************/

#include<stdio.h>

#ifndef MATRIX_H_
#define MATRIX_H_

#if defined(_WIN32) //If operating system is Windows
#define OS 1

#elif defined(__linux__) //If operating system is Linux
#define OS 2

#elif defined(__APPLE__) && defined(__MACH__) //If operating system is MacOS
#define OS 3

#else
#define OS 0
#endif

#define STR_LEN 200 //String length used throughout the code
#define NUM_CMD 3 //Number of system commands used

typedef struct { //Matrix of integers
	int** data;
	int lines;
	int columns;
} IntMatrix;

typedef struct { //Array of strings
	char** data;
	int lines;
	int length;
} StringArray;

typedef struct { //All information about a subject
	char* image;
	int number;
	int numberInstances;
	float score;
	int glasses;
	int ethnicity;
	int age;
	int rank;
} Subject;

typedef struct { //Characteristic every subject has
	int value;
	int numberInstances;
	int numberSubjects;
	float avgScore;
	float varScore;
} Characteristic;

extern const char urlPref[STR_LEN]; //Image URL prefix
extern char dataPath[STR_LEN]; //Path to source data
extern char createdDataPath[STR_LEN]; //Path to output folder
extern char systemCommands[NUM_CMD][STR_LEN]; //Array of commands to execute on the system

//For calling system commands
enum {
	wget, open, gnuplot
};

//Error return values
enum {
	ER_MEM = 100, ER_FILE, ER_PIPE, ER_MNUM, ER_OS, ER_CMD, ER_PATH
};

int setup(); //Set global variables according to operating system
Subject* readSubjects(FILE* fpDataMatrix, FILE* fpImages, FILE* fpGlasses,
		FILE* fpEthnicity, FILE* fpAges); //Save all the information about all the subjects to an array of Subject structures
IntMatrix readIntMatrix(FILE* source); //Read an integer matrix from a space separated text file
StringArray readStringArray(FILE* source); //Read an text file to an array of strings terminated by \n
FILE* openFile(char* file, char* modus); //Open a file for reading from dataPath or writing to createdDataPath
Characteristic* evaluateCharacteristic(Subject* source, int numberSubjects,
		char characteristic); //Get all values from one characteristic, sort them and calculate the average and variance in score
float* calculateColumnAverage(IntMatrix* dataMatrix); //Calculate the average of each column of an integer array
int getColumns(FILE* source); //Get number of columns in a space separated text file
int getLines(FILE* source); //Get number of lines in a text file
int getLineLength(FILE* source); //Get length of the longest line in a text file
int printSubject(FILE* fp, Subject subject); //Print all information about a subject to a stream
int showFile(char *file, int created); //Open file in preferred application for file type
int downloadFile(char *file, char *target); //Download file specified in URL to target
char* makeWindowsPath(char* path); //Change unix syntax path to Windows syntax path
int makeDirectory(char* path); //Make a directory

#endif /* MATRIX_H_ */
