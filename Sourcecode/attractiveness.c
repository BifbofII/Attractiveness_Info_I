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
#include<sys/types.h>
#include<sys/stat.h>
#if OS==1
#include<direct.h>
#endif

const char urlPref[] =
		"http://mraetsch.rt-lions.de/Attractiveness_rel_2.0/olympicstomato/"; //URL prefix
char dataPath[STR_LEN] = ""; //Path to source data
char createdDataPath[STR_LEN] = ""; //Path to the output folder
char systemCommands[NUM_CMD][STR_LEN]; //Array of commands to execute on the system

int setup() { //Set global variables according to operating system
	int i, j, //Counting variables
			good, //Boolean if command is executable
			numberPath = 0; //Number of entries in PATH
	char tmpString[STR_LEN], //String for temporary use as path storage
			dividingChar = ':'; //Char that is used to divide PATH entries
	const char* PATH = getenv("PATH"); //Get PATH environment variable
	if (!PATH)
		return ER_PATH; //Unable to get PATH
	char** paths;

	//Add slash at the end of path to data source if missing
	if (dataPath[strlen(dataPath) - 1] != '/') {
		dataPath[strlen(dataPath) + 1] = '\0';
		dataPath[strlen(dataPath)] = '/';
	}

	//Add slash at the end of output path if missing
	if (createdDataPath[strlen(createdDataPath) - 1] != '/') {
		createdDataPath[strlen(createdDataPath) + 1] = '\0';
		createdDataPath[strlen(createdDataPath)] = '/';
	}

	//Set and check commands depending on operating system
	switch (OS) {
	case 1: //Windows
		sprintf(systemCommands[download], "bitsadmin");
		sprintf(systemCommands[downloadSource], "/transfer \"JobName\"");
		strcpy(systemCommands[downloadTarget], "");
		sprintf(systemCommands[openStandard], "start");
		sprintf(systemCommands[gnuplot], "gnuplot");

		makeWindowsPath(dataPath);
		makeWindowsPath(createdDataPath);

		break;

	case 2: //Linux
		sprintf(systemCommands[download], "wget");
		strcpy(systemCommands[downloadSource], "");
		sprintf(systemCommands[downloadTarget], "-O");
		sprintf(systemCommands[openStandard], "xdg-open");
		sprintf(systemCommands[gnuplot], "gnuplot");

		break;

	case 3: //OS X
		sprintf(systemCommands[download], "curl");
		strcpy(systemCommands[downloadSource], "");
		sprintf(systemCommands[downloadTarget], "-o");
		sprintf(systemCommands[openStandard], "open");
		sprintf(systemCommands[gnuplot], "gnuplot");

		break;

	default:
		return ER_OS;
	}

	if (OS == 1) //If Windows, change dividing char to ';'
		dividingChar = ';';

	//Count entries of PATH
	char* positionChar = strchr(PATH, dividingChar);
	while (positionChar != NULL) {
		numberPath++;
		positionChar = strchr(positionChar + 1, dividingChar);
	}
	numberPath++;

	//Allocate memory for paths array
	paths = (char**) malloc(sizeof(char*) * numberPath);
	if (paths == NULL)
		return ER_MEM; //Error allocating memory

	paths[0] = (char*) malloc(sizeof(char) * numberPath * STR_LEN);
	if (paths[0] == NULL)
		return ER_MEM; //Error allocating memory
	for (i = 0; i < numberPath; i++)
		paths[i] = paths[0] + i * STR_LEN;

	j = 0;
	for (i = 0; i < numberPath; i++, j++) {
		paths[i][0] = '\0';
		while (PATH[j] != dividingChar && PATH[j] != '\0') {
			sprintf(paths[i], "%s%c", paths[i], PATH[j]);
			j++;
		}

		//Add slashes to end of path if not there
		if (paths[i][strlen(paths[i]) - 1] != '/') {
			paths[i][strlen(paths[i]) + 1] = '\0';
			paths[i][strlen(paths[i])] = '/';
		}
	}

#if OS == 1
	//Check whether commands can be executed under windows
	struct _stat tmpStat;
	for (i=0; i < NUM_CMD; i++) {
		good = 0;

		if (i == downloadTarget || i == downloadSource || i == openStandard) {
			continue;
		}

		for (j=0; j < numberPath; j++) {
			sprintf(tmpString, "%s%s.exe", paths[j], systemCommands[i]);
			makeWindowsPath(tmpString);
			if (!_stat(tmpString, &tmpStat)) {
				if (tmpStat.st_mode & S_IXUSR) {
					good = 1;
				}
			}
		}
		if (!good) {
			return i + 1;
		}
	}
#else
	//Check whether commands can be executed under unix
	struct stat tmpStat;
	for (i = 0; i < NUM_CMD; i++) {
		good = 0;

		if (i == downloadTarget || i == downloadSource) {
			continue;
		}

		for (j = 0; j < numberPath; j++) {
			sprintf(tmpString, "%s%s", paths[j], systemCommands[i]);
			if (!stat(tmpString, &tmpStat)) {
				if (tmpStat.st_mode & S_IEXEC) {
					good = 1;
				}
			}
		}
		if (!good) {
			return i + 1;
		}
	}
#endif

	return 0;
}

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
			data[i][strlen(data[i]) - (OS == 1 ? 1 : 2)] = '\0';
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

	if (OS == 1)
		makeWindowsPath(destiny);

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

	fseek(source, 0, SEEK_SET); //Set cursor to beginning of file

	tmp = fgetc(source);

	for (i = 0; i < lines; i++) {
		length = 0;
		while (tmp != '\n' && tmp != '\r') {
			length++;
			tmp = fgetc(source);
		}

		tmp = fgetc(source);
		if (tmp == '\n')
			tmp = fgetc(source);

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

	sprintf(command, "%s %s%s", systemCommands[openStandard],
			created ? createdDataPath : dataPath, file);

	if (OS == 1) //If OS is Windows, change path syntax
		makeWindowsPath(command);

	system(command);

	return 0;
}

int downloadFile(char *file, char *target) { //Download file specified in URL to target
	char command[STR_LEN];
	char targetPath[STR_LEN];

	sprintf(targetPath, "%s%s", createdDataPath, target);

	if (OS == 1) //If OS is Windows, change path syntax
		makeWindowsPath(targetPath);

	sprintf(command, "%s %s %s%s %s %s", systemCommands[download],
			systemCommands[downloadSource], urlPref, file,
			systemCommands[downloadTarget], targetPath);

	system(command);

	return 0;
}

char* makeWindowsPath(char* path) { //Change unix syntax path to Windows syntax path
#if OS == 1
int i; //Counting variable
char cd[STR_LEN];//Current directory

_getcwd(cd, STR_LEN);

if (path[0] == '.') {
	sprintf(cd, "%s/%s", cd, path);
	sprintf(path, "%s", cd);
}

for (i = 0; i < strlen(path); i++)
if (path[i] == '/')
path[i] = '\\';
#endif
	return path;
}

int makeDirectory(char* path) { //Make a directory
#if OS == 1
makeWindowsPath(path);
_mkdir(path);
#else
	mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

	return 0;
}

int plotGUI(int type, char* title, char* label, char* source, int numberLines,
		float* lines) { //Plot values to GUI
	char types[][STR_LEN] = { "linespoint", "boxes" }, //Plot types
			path[STR_LEN]; //Path variable for input
	int i, j; //Counting variables
	FILE* gnuplotPipe = popen("gnuplot -persistent", "w");
	if (gnuplotPipe == NULL)
		return ER_PIPE; //Error opening Pipe

	sprintf(path, "%s%s", createdDataPath, source);
	if (OS == 1) {
		makeWindowsPath(path);
		for (i = 0; i < strlen(path); i++) //Double all backslashes in path
			if (path[i] == '\\' && path[i + 1] != '\\') {
				path[strlen(path) + 1] = '\0';
				for (j = strlen(path); j > i; j--)
					path[j] = path[j - 1];
				i++;
			}
	}

	fprintf(gnuplotPipe, "set yrange [0:]\n");

	for (i = 0; i < numberLines; i++) { //Add vertical lines
		fprintf(gnuplotPipe, "set arrow from %f, graph 0 to %f, graph 1 nohead",
				(double) lines[i], (double) lines[i]);
		if (i % 2) //Make each second line red
			fprintf(gnuplotPipe, " lc rgb 'red'\n");
		else
			fprintf(gnuplotPipe, " lc rgb 'black'\n");
	}

	fprintf(gnuplotPipe, "set title \"%s\"\n", title);
	fprintf(gnuplotPipe, "plot '%s' with %s title \"%s\"\n", path, types[type],
			label);

	pclose(gnuplotPipe);

	return 0;
}

int plotPNG(int type, char*title, char* label, char* source, char* target,
		int numberLines, float* lines) { //Plot values to PNG
	char types[][STR_LEN] = { "linespoint", "boxes" }, //Plot types
			sourcePath[STR_LEN], targetPath[STR_LEN]; //Path Variables for input and output
	int i, j, //Counting variables
			fileExtention = 0; //Has the target the file ending .png?
	FILE* gnuplotPipe = popen("gnuplot -persistent", "w");
	if (gnuplotPipe == NULL)
		return ER_PIPE; //Error opening Pipe

	sprintf(sourcePath, "%s%s", createdDataPath, source);
	sprintf(targetPath, "%s%s", createdDataPath, target);

	if (targetPath[strlen(targetPath) - 4] == '.') //Check if targetPath has file extention appended
		if (targetPath[strlen(targetPath) - 3] == 'p'
				|| targetPath[strlen(targetPath) - 2] == 'P')
			if (targetPath[strlen(targetPath) - 2] == 'n'
					|| targetPath[strlen(targetPath) - 2] == 'N')
				if (targetPath[strlen(targetPath) - 1] == 'g'
						|| targetPath[strlen(targetPath) - 2] == 'G')
					fileExtention = 1;

	if (!fileExtention) //Set file extention if not appended
		strcat(targetPath, ".png");

	if (OS == 1) { //If OS is Windows
		makeWindowsPath(sourcePath);
		makeWindowsPath(targetPath);
		for (i = 0; i < strlen(sourcePath); i++) //Double all backslashes in sourcePath
			if (sourcePath[i] == '\\' && sourcePath[i + 1] != '\\') {
				sourcePath[strlen(sourcePath) + 1] = '\0';
				for (j = strlen(sourcePath); j > i; j--)
					sourcePath[j] = sourcePath[j - 1];
				i++;
			}
		for (i = 0; i < strlen(targetPath); i++) //Double all backslashes in targetPath
			if (targetPath[i] == '\\' && targetPath[i + 1] != '\\') {
				targetPath[strlen(targetPath) + 1] = '\0';
				for (j = strlen(targetPath); j > i; j--)
					targetPath[j] = targetPath[j - 1];
				i++;
			}
	}

	fprintf(gnuplotPipe, "set yrange [0:]\n");

	for (i = 0; i < numberLines; i++) { //Add vertical lines
		fprintf(gnuplotPipe, "set arrow from %f, graph 0 to %f, graph 1 nohead",
				(double) lines[i], (double) lines[i]);
		if (i % 2) //Make each second line red
			fprintf(gnuplotPipe, " lc rgb 'red'\n");
		else
			fprintf(gnuplotPipe, " lc rgb 'black'\n");
	}

	fprintf(gnuplotPipe, "set term png\n");
	fprintf(gnuplotPipe, "set output \"%s\"\n", targetPath);

	fprintf(gnuplotPipe, "set title \"%s\"\n", title);
	fprintf(gnuplotPipe, "plot '%s' with %s title \"%s\"\n", sourcePath,
			types[type], label);

	pclose(gnuplotPipe);

	return 0;
}
