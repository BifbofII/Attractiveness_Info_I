/******************************************************************************************
 * Name: main.c
 ******************************************************************************************
 * Author: Christoph Jabs
 ******************************************************************************************
 * Return values:
 * 0: All went well
 * -1: Unknown error
 * 100: Error when allocating memory
 * 101: Error when opening file
 * 102: Error when opening Pipe
 * 103: No annotation for matrikel number
 * 104: Unknown operating system
 * 105: Command not found
 * 106: Could not get PATH
 ******************************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include "attractiveness.h"

int main(int argc, char*argv[]) {
	int i, j; //Counting variables
	int userMatrNr = 0, //Matrikelnumber of the user
			userNumber, //Index of the users annotation
			diversePlus = 0, //Number of times user rated higher than average
			diverseMinus = 0, //Number of times user rated lower than average
			highDivPlus, //Index of picture rated much better than average
			highDivMinus = -1, //Index of picture rated much worse than average
			gui = 0, //GUI activated or not
			download = 0, //Download ranked pictures or not
			setupValue; //Return value of setup function
	float avgScore = 0, //Global average score
			varScore = 0; //Global variance in score
	const float presicionScore = 0.01; //Value of resolution for Score-Number
	int* scoreNumber = NULL; //Array for number of subjects with score i*presicionScore
	int* matrNr = NULL; //Array of matrikelnumbers of the annotations
	int* userData = NULL; //Array of users first annotation
	char tmpString[STR_LEN]; //String for different temporary use
	Subject* subjects = NULL; //Array of all subjects
	Subject** subjectsSorted = NULL; //All subjects sorted by score
	FILE *fpDataMatrix = NULL, //datamatrix_v6.txt file
			*fpImages = NULL, //images.txt file
			*fpGlasses = NULL, //glasses.txt file
			*fpEthnicity = NULL, //ethnicity.txt file
			*fpAges = NULL, //ages.txt file
			*fp = NULL, //File pointer for different temporary use
			*gnuplotPipe; //Pipe to gnuplot
	IntMatrix dataMatrix; //Interger matrix of all data
	StringArray annotations; //String array of all user names
	Characteristic *age = NULL, //All ages with average and variance in score
			*glasses = NULL, //Glasses with average and variance in score
			*ethnicity = NULL; //Ethnicity with average and variance in score

//Command line arguments
	if (argc > 1)
		for (i = 1; i < argc; i++) {
			//Data path from command line
			if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "-P"))
				strcpy(dataPath, argv[i + 1]);
			//Matrikelnumber from command line
			else if (!strcmp(argv[i], "-n") || !strcmp(argv[i], "-N")) {
				for (j = 0; j < strlen(argv[i + 1]); j++)
					if (argv[i + 1][j] > 47 && argv[i + 1][j] < 58)
						userMatrNr = userMatrNr * 10 + argv[i + 1][j] - 48;
				//GUI yes or no from command line
			} else if (!strcmp(argv[i], "-g") || !strcmp(argv[i], "-G"))
				gui = 1;
			//Download yes or no from command line
			else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "-D"))
				download = 1;
			//Output path from command line
			else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "-O"))
				strcpy(createdDataPath, argv[i + 1]);

		}

	if (!strcmp(dataPath, "")) {
		//Get data path
		printf("Please enter the path to the data folder: ");
		scanf("%s", dataPath);
	}

	if (!userMatrNr) {
		//Read Matrikel-Number
		printf("Please enter your matrikel number: ");
		scanf("%d", &userMatrNr);
	}

	if (!strcmp(createdDataPath, "")) {
		//Set Path for created Data
		strcpy(createdDataPath, dataPath);
		strcat(createdDataPath, "Results/");
	}

//Set global variables according to operating system
	setupValue = setup();
	if (setupValue == ER_OS)
		return ER_OS;
	else if (setupValue == ER_PATH)
		return ER_PATH;
	else if (setupValue){
		printf("%s: command not found or not executable\n", systemCommands[setupValue - 1]);
		return ER_CMD;
	}

//Create directory for created Data
	makeDirectory(createdDataPath);

//Read subject information
	fpDataMatrix = openFile("datamatrix_v6", "r");
	fpImages = openFile("images", "r");
	fpGlasses = openFile("glasses", "r");
	fpEthnicity = openFile("ethnicity", "r");
	fpAges = openFile("ages", "r");

	if (fpDataMatrix == NULL || fpImages == NULL || fpGlasses == NULL
			|| fpEthnicity == NULL || fpAges == NULL)
		return ER_FILE; //Error opening files

	subjects = readSubjects(fpDataMatrix, fpImages, fpGlasses, fpEthnicity,
			fpAges);
	if (subjects == NULL)
		return ER_MEM; //Error allocating memory

//Read datamatrix
	dataMatrix = readIntMatrix(fpDataMatrix);
	if (dataMatrix.data == NULL)
		return ER_MEM; //Error allocating memory

	fclose(fpDataMatrix);
	fclose(fpImages);
	fclose(fpGlasses);
	fclose(fpEthnicity);
	fclose(fpAges);

//Read user names
	fp = openFile("annos_v6", "r");
	if (fp == NULL)
		return ER_FILE; //Error opening file

	annotations = readStringArray(fp);
	if (annotations.data == NULL)
		return ER_MEM; //Error allocating memory

	fclose(fp);

//Write matrikelnumber array
	matrNr = (int*) malloc(sizeof(int) * annotations.lines);
	if (matrNr == NULL)
		return ER_MEM; //Error allocating memory

	for (i = 0; i < annotations.lines; i++) {
		matrNr[i] = 0;
		for (j = 5; j >= 0; j--)
			matrNr[i] += (annotations.data[i][11 - j] - 48) * pow(10, j);
	}

//Get user annotation index
	for (i = 0; i < annotations.lines; i++)
		if (userMatrNr == matrNr[i])
			break;

	if (i == annotations.lines) {
		printf("There was no annotation found for your matrikel number");
		return ER_MNUM;
	} else {
		userNumber = i;
		printf("Your user name is %s\n",
				annotations.data[userNumber]);
	}

//Get user data
	userData = (int*) malloc(sizeof(int) * subjects[0].numberInstances);
	if (userData == NULL)
		return ER_MEM; //Error allocating memory

	for (i = 0; i < subjects[0].numberInstances; i++)
		userData[i] = dataMatrix.data[userNumber][i];

//Write user data to file
	fp = openFile(annotations.data[userNumber], "w");
	if (fp == NULL)
		return ER_FILE; //Error opening file

	for (i = 0; i < subjects[0].numberInstances; i++) {
		fprintf(fp, "%d", userData[i]);
		if (i == subjects[0].numberInstances - 1)
			fputc('\n', fp);
		else
			fputc(' ', fp);
	}
	fclose(fp);

//Bubblesort subjects
	subjectsSorted = malloc(sizeof(Subject*) * subjects[0].numberInstances);
	if (subjectsSorted == NULL)
		return ER_MEM; //Error allocating memory

	for (i = 0; i < subjects[0].numberInstances; i++)
		subjectsSorted[i] = &subjects[i];

	for (i = subjects[0].numberInstances; i > 1; i--)
		for (j = 0; j < i - 1; j++)
			if (subjectsSorted[j]->score < subjectsSorted[j + 1]->score) {
				Subject* tmp = subjectsSorted[j];
				subjectsSorted[j] = subjectsSorted[j + 1];
				subjectsSorted[j + 1] = tmp;
			}

//Write ranks to subjects array
	for (i = 0; i < subjects[0].numberInstances; i++)
		subjects[subjectsSorted[i]->number].rank = i + 1;

//Write scores to file
	fp = openFile("Scores", "w");
	if (fp == NULL)
		return ER_FILE; //Error opening file

	for (i = 0; i < subjects[0].numberInstances; i++)
		fprintf(fp, "%.6f\n", subjects[i].score);
	fclose(fp);

//Get number per score
	scoreNumber = (int*) malloc(sizeof(int) * (1.0 / presicionScore + 1));
	if (scoreNumber == NULL)
		return ER_MEM; //Error allocating memory

	for (i = 0; i < (int) 1.0 / presicionScore + 1; i++) {
		scoreNumber[i] = 0;
		for (j = 0; j < subjects[0].numberInstances; j++)
			if (subjects[j].score >= (i * presicionScore - 0.5 * presicionScore)
					&& subjects[j].score
							< (i * presicionScore + 0.5 * presicionScore))
				scoreNumber[i]++;
	}

//Write number per score to file
	fp = openFile("ScoreNumber", "w");
	if (fp == NULL)
		return ER_FILE; //Error opening file

	for (i = 0; i < (int) 1.0 / presicionScore + 1; i++)
		fprintf(fp, "%f %d\n", i * presicionScore, scoreNumber[i]);
	fclose(fp);

//Write highest and lowest rated picture
	fp = openFile("MissInf_MissDissInf", "w");
	if (fp == NULL)
		return ER_FILE; //Error opening file

	fprintf(fp, "Miss Informatics: %s%s\n", urlPref, subjectsSorted[0]->image);
	printSubject(fp, *(subjectsSorted[0]));
	fprintf(fp, "Miss Diss-Informatics: %s%s", urlPref,
			subjectsSorted[subjects[0].numberInstances - 1]->image);
	printSubject(fp, *(subjectsSorted[subjects[0].numberInstances - 1]));
	fclose(fp);

//Download highest and lowest rated picture
	downloadFile(subjectsSorted[0]->image, "MissInformatics.jpg");
	downloadFile(subjectsSorted[subjects[0].numberInstances - 1]->image,
			"MissDissInformatics.jpg");

//Show highest and lowest rated picture
	if (gui) {
		showFile("MissInformatics.jpg", 1);
		printf(
				"The highest rated picture is shown\n(Press <Enter> to continue...)\n");
		getchar();

		showFile("MissDissInformatics.jpg", 1);
		printf(
				"The lowest rated picture is shown\n(Press <Enter> to continue...)\n");
		getchar();
	}

//Calculate average Score
	for (i = 0; i < subjects[0].numberInstances; i++)
		avgScore += subjects[i].score;
	avgScore /= subjects[0].numberInstances;

//Calculate variance
	for (i = 0; i < subjects[0].numberInstances; i++)
		varScore += pow(subjects[i].score - avgScore, 2);
	varScore /= subjects[0].numberInstances - 1;
	varScore = sqrt(varScore);

//Write average and variance in score to file
	fp = openFile("ScoreAvgVar", "w");
	if (fp == NULL)
		return ER_FILE; //Error opening file

	fprintf(fp, "Average: %f\nVariance: %f", avgScore, varScore);
	fclose(fp);

//Count diversities
	for (i = 0; i < subjects[0].numberInstances; i++) {
		if (subjectsSorted[i]->score > 0.5) {
			if (userData[subjectsSorted[i]->number] == 0) {
				diverseMinus++;
				if (highDivMinus == -1)
					highDivMinus = subjectsSorted[i]->number;
			}
		} else if (subjectsSorted[i]->score < 0.5) {
			if (userData[subjectsSorted[i]->number] == 1) {
				diversePlus++;
				highDivPlus = subjectsSorted[i]->number;
			}
		}
	}

//Write diversities to file
	strcpy(tmpString, "Diversities_");
	strcat(tmpString, annotations.data[userNumber]);

	fp = openFile(tmpString, "w");
	if (fp == NULL)
		return ER_FILE; //Error opening file

	fprintf(fp,
			"diversePlus: %d\ndiverseMinus: %d\n\nMiss Diversity Plus: %s%s",
			diversePlus, diverseMinus, urlPref,
			diversePlus ? "" : subjects[highDivPlus].image);
	if (diversePlus)
		printSubject(fp, subjects[highDivPlus]);
	fprintf(fp, "Miss Diversity Minus: %s%s", urlPref,
			diverseMinus ? "" : subjects[highDivMinus].image);
	if (diverseMinus)
		printSubject(fp, subjects[highDivMinus]);
	fclose(fp);

//Download highest diverse pictures
	strcpy(tmpString, "MissDiversityPlus_");
	strcat(tmpString, annotations.data[userNumber]);
	strcat(tmpString, ".jpg");
	downloadFile(subjects[highDivPlus].image, tmpString);

	strcpy(tmpString, "MissDiversityMinus_");
	strcat(tmpString, annotations.data[userNumber]);
	strcat(tmpString, ".jpg");
	downloadFile(subjects[highDivMinus].image, tmpString);

//Show highest diverse pictures
	if (gui) {
		strcpy(tmpString, "MissDiversityPlus_");
		strcat(tmpString, annotations.data[userNumber]);
		strcat(tmpString, ".jpg");
		showFile(tmpString, 1);
		printf(
				"The picture you rated way higher than the average is shown\n(Press <Enter> to continue...)\n");
		getchar();

		strcpy(tmpString, "MissDiversityMinus_");
		strcat(tmpString, annotations.data[userNumber]);
		strcat(tmpString, ".jpg");
		showFile(tmpString, 1);
		printf(
				"The picture you rated way lower than the average is shown\n(Press <Enter> to continue...)\n");
		getchar();
	}

//Get evaluations of age, glasses and ethnicity
	age = evaluateCharacteristic(subjects, subjects[0].numberInstances, 'a');
	glasses = evaluateCharacteristic(subjects, subjects[0].numberInstances,
			'g');
	ethnicity = evaluateCharacteristic(subjects, subjects[0].numberInstances,
			'e');

	if (age == NULL || glasses == NULL || ethnicity == NULL)
		return ER_MEM; //Error allocating memory

//Write characteristics to file
	fp = openFile("AgeScore", "w");
	if (fp == NULL)
		return ER_FILE; //Error opening file

	for (i = 0; i < age[0].numberInstances; i++)
		fprintf(fp, "%d %.6f\n", age[i].value, age[i].avgScore);
	fclose(fp);

	fp = openFile("GlassesScore", "w");
	if (fp == NULL)
		return ER_FILE; //Error opening file

	for (i = 0; i < glasses[0].numberInstances; i++)
		fprintf(fp, "%d %.6f\n", glasses[i].value, glasses[i].avgScore);
	fclose(fp);

	fp = openFile("EthnicityScore", "w");
	if (fp == NULL)
		return ER_FILE; //Error opening file

	for (i = 0; i < ethnicity[0].numberInstances; i++)
		fprintf(fp, "%d %.6f\n", ethnicity[i].value, ethnicity[i].avgScore);
	fclose(fp);

//Plotting
	gnuplotPipe = popen("gnuplot -persistent", "w");
	if (gnuplotPipe == NULL)
		return ER_PIPE; //Error opening Pipe

	fprintf(gnuplotPipe, "set yrange [0:]\n");

	if (gui) {
		//Plot Age-Score GUI
		fprintf(gnuplotPipe, "set title \"Age-Score\"\n");
		fprintf(gnuplotPipe, "plot '%sAgeScore.txt' with linespoints\n",
				createdDataPath);
		fflush(gnuplotPipe);
		printf(
				"The correlation between age and score is shown\n(Press <Enter> to continue...)\n");
		getchar();

		//Plot Glasses-Score GUI
		fprintf(gnuplotPipe, "set title \"Glasses-Score\"\n");
		fprintf(gnuplotPipe, "plot '%sGlassesScore.txt' with boxes\n",
				createdDataPath);
		fflush(gnuplotPipe);
		printf(
				"The correlation between glasses and score is shown\n(Press <Enter> to continue...)\n");
		getchar();

		//Plot Ethnicity-Score GUI
		fprintf(gnuplotPipe, "set title \"Ethnicity-Score\"\n");
		fprintf(gnuplotPipe, "plot '%sEthnicityScore.txt' with boxes\n",
				createdDataPath);
		fflush(gnuplotPipe);
		printf(
				"The correlation between ethnicity and score is shown\n(Press <Enter> to continue...)\n");
		getchar();

		//Plot Score-Number GUI
		fprintf(gnuplotPipe, "set title \"Score-Number\"\n");
		fprintf(gnuplotPipe,
				"set arrow 1 from %f, graph 0 to %f, graph 1 nohead lc rgb 'black'\n",
				avgScore, avgScore); //Line for average Score
		fprintf(gnuplotPipe,
				"set arrow 2 from %f, graph 0 to %f, graph 1 nohead lc rgb 'red'\n",
				avgScore + varScore, avgScore + varScore); //Line for variance score plus
		fprintf(gnuplotPipe,
				"set arrow 3 from %f, graph 0 to %f, graph 1 nohead lc rgb 'red'\n",
				avgScore - varScore, avgScore - varScore); //Line for variance score minus
		fprintf(gnuplotPipe, "plot '%sScoreNumber.txt' with linespoints, \n",
				createdDataPath);
		fflush(gnuplotPipe);
		printf(
				"The correlation between score and number of subjects is shwon\n(Press <Enter> to continue...)\n");
		getchar();
	}

	fprintf(gnuplotPipe, "set term png\n");
	fprintf(gnuplotPipe, "unset arrow 1\nunset arrow 2\nunset arrow 3\n");

//Save Age-Score Plot
	fprintf(gnuplotPipe, "set output \"%sAgeScore.png\"\n", createdDataPath);
	fprintf(gnuplotPipe, "set title \"Age-Score\"\n");
	fprintf(gnuplotPipe, "plot '%sAgeScore.txt' with linespoints\n",
			createdDataPath);
	fflush(gnuplotPipe);

//Save Glasses-Score Plot
	fprintf(gnuplotPipe, "set output \"%sGlassesScore.png\"\n",
			createdDataPath);
	fprintf(gnuplotPipe, "set title \"Glasses-Score\"\n");
	fprintf(gnuplotPipe, "plot '%sGlassesScore.txt' with boxes\n",
			createdDataPath);
	fflush(gnuplotPipe);

//Save Ethnicity-Score Plot
	fprintf(gnuplotPipe, "set output \"%sEthnicityScore.png\"\n",
			createdDataPath);
	fprintf(gnuplotPipe, "set title \"Ethnicity-Score\"\n");
	fprintf(gnuplotPipe, "plot '%sEthnicityScore.txt' with boxes\n",
			createdDataPath);
	fflush(gnuplotPipe);

//Save Score-Number
	fprintf(gnuplotPipe, "set output \"%sScoreNumber.png\"\n", createdDataPath);
	fprintf(gnuplotPipe, "set title \"Score-Number\"\n");
	fprintf(gnuplotPipe,
			"set arrow 1 from %f, graph 0 to %f, graph 1 nohead lc rgb 'black'\n",
			avgScore, avgScore); //Line for average score
	fprintf(gnuplotPipe,
			"set arrow 2 from %f, graph 0 to %f, graph 1 nohead lc rgb 'red'\n",
			avgScore + varScore, avgScore + varScore); //Line for variance score plus
	fprintf(gnuplotPipe,
			"set arrow 3 from %f, graph 0 to %f, graph 1 nohead lc rgb 'red'\n",
			avgScore - varScore, avgScore - varScore); //Line for variance score minus
	fprintf(gnuplotPipe, "plot '%sScoreNumber.txt' with linespoints\n",
			createdDataPath);
	fflush(gnuplotPipe);

	pclose(gnuplotPipe);

//Download all images sorted by rank
	if (download) {
		sprintf(tmpString, "%s/PicturesSorted", createdDataPath);
		makeDirectory(tmpString);

		for (i = 0; i < subjects[0].numberInstances; i++) {
			sprintf(tmpString, "PicturesSorted/%04d_%f.jpg",
					subjectsSorted[i]->rank, subjectsSorted[i]->score);

			downloadFile(subjectsSorted[i]->image, tmpString);
		}
	}

	return 0;
}
