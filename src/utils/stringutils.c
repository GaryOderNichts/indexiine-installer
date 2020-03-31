#include "stringutils.h"
#include <stdlib.h>
#include <string.h>

char* stringReplaceAll(const char *search, const char *replace, char *string) 
{
	char* searchStart = strstr(string, search);
	while (searchStart != NULL)
	{
		char* tempString = (char*) malloc(strlen(string) * sizeof(char));
		if(tempString == NULL) {
			return NULL;
		}

		strcpy(tempString, string);

		int len = searchStart - string;
		string[len] = '\0';

		strcat(string, replace);

		len += strlen(search);
		strcat(string, (char*)tempString+len);

		free(tempString);

		searchStart = strstr(string, search);
	}
	
	return string;
}

char* stringReplace(char *search, char *replace, char *string)
{
	char* searchStart = strstr(string, search);
	if(searchStart == NULL) {
		return string;
	}

	char* tempString = (char*) malloc(strlen(string) * sizeof(char));
	if(tempString == NULL) {
		return NULL;
	}

	strcpy(tempString, string);

	int len = searchStart - string;
	string[len] = '\0';

	strcat(string, replace);

	len += strlen(search);
	strcat(string, (char*)tempString+len);

	free(tempString);
	
	return string;
}