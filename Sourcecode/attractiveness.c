/******************************************************************************************
 * Name: attractiveness.c
 ******************************************************************************************
 * Author: Christoph Jabs
 ******************************************************************************************/

#include "attractiveness.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>

Subject* readSubjects(FILE* fpDataMatrix, FILE* fpImages, FILE* fpGlasses,
		FILE* fpEthnicity, FILE* fpAges) { //Save all the information about all the subjects to an array of Subject structures
	int i; //Counting variable
	Subject* subjects; //All subjects (return value)
	IntMatrix dataMatrix = readIntMatrix(fpDataMatrix); //Data matrix of annotations
	StringArray images = readStringArray(fpImages); //Array of image names
	IntMatrix glasses = readIntMatrix(fpGlasses); //Values for characteristic glasses
	IntMatrix ethnicity = readIntMatrix(fpEthnicity); //Values for characteristic ethnicity
	IntMatrix ages = readIntMatrix(fpAges); //Values for characteristic age

	if (dataMatrix.data == NULL || images.data == NULL || glasses.data == NULL
			|| ethnicity.data == NULL || ages.data == NULL)
		return NULL; //Error allocating memory

	float* score = calculateColumnAverage(&dataMatrix);
	if (score == NULL)
		return NULL; //Error allocating memory

	subjects = malloc(sizeof(Subject) * dataMatrix.columns);
	if (subjects == NULL)
		return NULL; //Error allocating memory

	for (i = 0; i < dataMatrix.columns; i++) { //Write values for all components of subjects
		subjects[i].number = i;
		subjects[i].score = score[i];

		subjects[i].image = malloc(sizeof(char) * images.length);
		if (subjects[i].image == NULL)
			return NULL;

		subjects[i].image = images.data[i];
		subjects[i].glasses = glasses.data[i][0];
		subjects[i].ethnicity = ethnicity.data[i][0];
		subjects[i].age = ages.data[i][0];
		subjects[i].numberInstances = dataMatrix.columns;
		subjects[i].rank = 0;
	}

	return subjects;
}

IntMatrix readIntMatrix(FILE* source) { //Read an integer matrix from a space separated text file
	int i, j, //Counting variables
			position; //Position of cursor
	IntMatrix matrix = { NULL, 0, 0 }; //Read integer matrix (return value)

	int lines = getLines(source);
	int columns = getColumns(source);

	position = ftell(source); //Get position of cursor

	fseek(source, 0, SEEK_SET); //Set cursor to beginning of file

	int** data = (int**) malloc(sizeof(int*) * lines);
	if (data == NULL)
		return matrix; //Error allocating memory

	data[0] = (int*) malloc(sizeof(int) * columns * lines);
	if (data[0] == NULL)
		return matrix; //Error allocating memory

	for (i = 0; i < lines; i++)
		data[i] = data[0] + columns * i;

	for (i = 0; i < lines; i++) {
		for (j = 0; j < columns; j++) {
			fscanf(source, "%d", &data[i][j]);
		}
	}

	fseek(source, position, SEEK_SET); //Set cursor back to where it was

	matrix.data = data;
	matrix.columns = columns;
	matrix.lines = lines;

	return matrix;
}

StringArray readStringArray(FILE* source) { //Read an text file to an array of strings terminated by \n
	int i, //Counting variable
			position; //Position of cursor
	StringArray array = { NULL, 0, 0 };

	int lines = getLines(source);
	int length = getLineLength(source);

	position = ftell(source); //Get cursor position

	fseek(source, 0, SEEK_SET); //Set cursor to beginning of file

	char** data = (char**) malloc(sizeof(char*) * lines);
	if (data == NULL)
		return array; //Error allocating memory

	data[0] = (char*) malloc(sizeof(char) * (length + 1) * lines);
	if (data[0] == NULL)
		return array; //Error allocating memory

	for (i = 0; i < lines; i++)
		data[i] = data[0] + (length + 1) * i;

	for (i = 0; i < lines; i++) {
		fgets(data[i], length + 1, source);
		if (data[i][strlen(data[i]) - 1] == '\n') {
			data[i][strlen(data[i]) - 1] = '\0';
			data[i][strlen(data[i]) - 1] = '\0';
		} else
			fseek(source, 1, SEEK_CUR);
	}

	fseek(source, position, SEEK_SET); //Set cursor back to where it was

	array.data = data;
	array.lines = lines;
	array.length = length;

	return array;
}

FILE* openFile(char* file, char* modus) { //Open a file for reading from dataPath or writing to createdDataPath
	char destiny[STR_LEN]; //Temporary string for complete file path

	sprintf(destiny, "%s%s.txt",
			!strcmp(modus, "r") ? dataPath : createdDataPath, file);

	return fopen(destiny, modus);
}

Characteristic* evaluateCharacteristic(Subject* source, int numberSubjects,
		char characteristic) { //Get all values from one characteristic, sort them and calculate the average and variance in score
	typedef struct _ListEntry { //Entry of linked list
		struct _ListEntry* next;
		int number;
		int value;
	} ListEntry;

	int i, j, //Counting variables
			value, //Temporary value of subject
			numberEntries = 0; //Number of different values for given characteristic
	ListEntry* start = NULL; //Start of linked list
	ListEntry* ptr = NULL; //Temporary Pointer
	ListEntry* lowestPtr = NULL; //Pointer to lowest list entry
	ListEntry* last = NULL; //Pointer to last list entry
	Characteristic* result = NULL; //Evaluated characteristic array (return value)

//Create List of characteristic entities
	for (i = 0; i < numberSubjects; i++) {
		ptr = start;

		//Characteristic is age
		if (characteristic == 'a' || characteristic == 'A')
			value = source[i].age;
		//Characteristic is glasses
		else if (characteristic == 'g' || characteristic == 'G')
			value = source[i].glasses;
		//Characteristic is ethnicity
		else if (characteristic == 'e' || characteristic == 'E')
			value = source[i].ethnicity;
		else
			return NULL; //Error, unspecified characteristic

		if (value != -1) {
			while (ptr != NULL) {
				if (ptr->value == value) {
					ptr->number++;
					break;
				} else
					ptr = ptr->next;
			}

			if (ptr == NULL) {
				numberEntries++;

				ListEntry* newPtr = (ListEntry*) malloc(sizeof(ListEntry));
				if (newPtr == NULL)
					return NULL;

				newPtr->value = value;
				newPtr->next = NULL;
				newPtr->number = 1;

				if (start == NULL)
					start = newPtr;
				else {
					ptr = start;

					while (ptr->next != NULL)
						ptr = ptr->next;

					ptr->next = newPtr;
				}
			}
		}
	}

//Create sorted characteristic array
	result = malloc(sizeof(Characteristic) * numberEntries);
	if (result == NULL)
		return NULL; //Error allocating memory

	for (i = 0; i < numberEntries; i++) {
		last = NULL;
		lowestPtr = start;
		ptr = start;

		while (ptr->next != NULL) {
			if (ptr->next->value < lowestPtr->value) {
				last = ptr;
				lowestPtr = ptr->next;
			}
			ptr = ptr->next;
		}

		result[i].value = lowestPtr->value;
		result[i].numberInstances = numberEntries;
		result[i].avgScore = 0;
		result[i].varScore = 0;
		result[i].numberSubjects = lowestPtr->number;

		if (last == NULL)
			start = lowestPtr->next;
		else
			last->next = lowestPtr->next;
		free(lowestPtr);
	}

	//Calculate avgScore
	for (i = 0; i < result[0].numberInstances; i++) {
		for (j = 0; j < source[0].numberInstances; j++) {
			//Characteristic is age
			if (characteristic == 'a' || characteristic == 'A')
				value = source[j].age;
			//Characteristic is glasses
			else if (characteristic == 'g' || characteristic == 'G')
				value = source[j].glasses;
			//Characteristic is ethnicity
			else if (characteristic == 'e' || characteristic == 'E')
				value = source[j].ethnicity;
			else
				return NULL; //Error, unspecified characteristic

			if (value == result[i].value)
				result[i].avgScore += source[j].score;
		}
		result[i].avgScore /= result[i].numberSubjects;
	}

	//Calculate varScore
	for (i = 0; i < result[0].numberInstances; i++) {
		for (j = 0; j < source[0].numberInstances; j++) {
			//Characteristic is age
			if (characteristic == 'a' || characteristic == 'A')
				value = source[j].age;
			//Characteristic is glasses
			else if (characteristic == 'g' || characteristic == 'G')
				value = source[j].glasses;
			//Characteristic is ethnicity
			else if (characteristic == 'e' || characteristic == 'E')
				value = source[j].ethnicity;
			else
				return NULL; //Error, unspecified characteristic

			if (value == result[i].value)
				result[i].varScore += pow(
						(source[j].score - result[i].avgScore), 2);
		}

		if (result[i].numberSubjects == 1)
			result[i].varScore = 0;
		else {
			result[i].varScore /= result[i].numberSubjects - 1;
			result[i].varScore = sqrt(result[i].varScore);
		}
	}

	return result;
}

float* calculateColumnAverage(IntMatrix* dataMatrix) { //Calculate the average of each column of an integer array
	int i, j, //Counting variables
			number; //Number of correct values
	float* average; //Array of averages (return value)

	average = malloc(sizeof(float) * dataMatrix->columns);
	if (average == NULL)
		return NULL; //Error allocating memory

//Calculate average of each columns
	for (i = 0; i < dataMatrix->columns; i++) {
		average[i] = 0;
		number = 0;
		for (j = 0; j < dataMatrix->lines; j++)
			if (dataMatrix->data[j][i] == 0 || dataMatrix->data[j][i] == 1) {
				average[i] += dataMatrix->data[j][i];
				number++;
			}
		average[i] /= number;
	}

	return average;
}

int getColumns(FILE* source) { //Get number of columns in a space separated text file
	int columns = 0, //Number of columns (return value)
			position; //Position of cursor
	char tmp; //Last read character

	position = ftell(source); //Get position of cursor

	fseek(source, 0, SEEK_SET); //Set cursor to beginning of file

	do {
		tmp = fgetc(source);
		if (tmp == ' ' || tmp == '\n')
			columns++;
	} while (tmp != '\n');

	fseek(source, position, SEEK_SET); //Set cursor back to where it was

	return columns;
}

int getLines(FILE* source) { //Get number of lines in a text file
	int lines = 0, //Number of lines (return value)
			position; //Position of cursor
	char tmp; //Last read character

	position = ftell(source); //Get position of cursor

	fseek(source, 0, SEEK_SET); //Set cursor to beginning of file

	do {
		tmp = fgetc(source);
		if (tmp == '\n')
			lines++;
	} while (tmp != EOF);

	fseek(source, position, SEEK_SET); //Set cursor back to where it was

	return lines;
}

int getLineLength(FILE* source) { //Get length of the longest line in a text file
	int length = 0, //Length of momentary line
			position, //Position of cursor
			i, //Counting variable
			maxLength = 0; //Length of longest line (return value)
	char tmp; //Last read character
	int lines = getLines(source); //Number of lines in file

	position = ftell(source); //Get cursor position

	fseek(source, 0, SEEK_SET); //Ste cursor to beginning of file

	for (i = 0; i < lines; i++) {
		tmp = fgetc(source);
		length = 0;
		while (tmp != '\n') {
			length++;
			tmp = fgetc(source);
		}

		if (length > maxLength)
			maxLength = length;
	}

	fseek(source, position, SEEK_SET); //Set cursor back to where it was

	return maxLength;
}

int printSubject(FILE* fp, Subject subject) { //Print all information about a subject to a stream
	fprintf(fp,
			"Picture: %s\nIndex: %d\nScore: %f\nRank: %d\nAge: %d\nGlasses: %d\nEthnicity: %d\n\n",
			subject.image, subject.number, subject.score, subject.rank,
			subject.age, subject.glasses, subject.ethnicity);

	return 1;
}

int showFile(char *file, int created) { //Open file in preferred application for file type
	char command[STR_LEN];

	sprintf(command, "xdg-open %s%s", created ? createdDataPath : dataPath,
			file);
	system(command);

	return 0;
}

int downloadFile(char *file, char *target) { //Download file specified in URL to target
	char command[STR_LEN];

	sprintf(command, "wget %s%s -O %s%s", urlPref, file, createdDataPath,
			target);
	system(command);

	return 0;
}
