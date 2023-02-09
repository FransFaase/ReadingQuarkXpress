#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define TRUE  1
#define FALSE 0

#define NAME  1
#define INTER 2
#define IMPL  3

void process_file(int mode, FILE *fin, FILE *fout, char *fn)
{
	fseek(fin, 0L, SEEK_SET);
	
	char line[1000];
	long line_nr = 1;
	char class_name[100];
	int in_class = 0;
	long nest = 0;
	int in_method_head = 0;

#define START_LINE if(!last_closed)fprintf(fout, "\n");last_closed=FALSE;\
         if(prev_skipped&&!printed)fprintf(fout, "#line %ld \"%s\"\n", line_nr, fn); printed=TRUE;
#define CLOSED last_closed=TRUE;
	bool prev_skipped = TRUE;
	bool last_closed = FALSE;

	fgets(line, 999, fin);
	while (!feof(fin))
	{
		bool printed = FALSE;
		
		if (line[strlen(line)-1] == '\n')
			line[strlen(line)-1] = '\0';
			
		long i;
		for (i = 0; line[i] == ' ' || line[i] == '\t'; i++);
		if (line[i] == '{')
			nest++;
			
		if (nest == 0)
		{	char *kind = NULL;
			char *ch = NULL;
			if (!strncmp(line, "class", 5))
			{	kind = "class";
				ch = line + 6;
			}
			else if (!strncmp(line, "struct", 6))
			{
				kind = "struct";
				ch = line + 7;
			}
			
			if (kind != NULL)
			{
				in_class = TRUE;	

				int j;
				for (j = 0; *ch != ';' && *ch != '\0' && *ch != ' '; j++, ch++)
					class_name[j] = *ch;
				class_name[j] = '\0';

				if (mode == NAME)
				{	START_LINE
					fprintf(fout, "%s %s;\n", kind, class_name);
					CLOSED
				}
				else if (mode == INTER)
				{	fprintf(fout, "\n");
					CLOSED
				}
				else if (mode == IMPL)
				{	fprintf(fout, "\n// Methods for the class %s\n\n", class_name);
					CLOSED
				}
			}
			
			if ((mode == IMPL && !in_class) || (mode == INTER && in_class))
			{
				START_LINE
				fprintf(fout, "%s\n", line);
				CLOSED
			}
		}
		else if (!in_class)
		{
			if (mode == IMPL)
			{	START_LINE
				fprintf(fout, "%s\n", line);
				CLOSED
			}
		}
		else if (nest == 1)
		{
			if (in_method_head)
			{
				if (mode != NAME)
				{
					START_LINE
					fprintf(fout, "%s", line);
				}
			}
			else
			{
				char *ch = strstr(line, "(");
				if (ch == NULL)
				{
					if (mode == INTER)
					{	START_LINE
						fprintf(fout, "%s\n", line);
						CLOSED
					}
				}
				else
				{
					in_method_head = TRUE;
					if (mode == INTER)
					{
						START_LINE
						fprintf(fout, "%s", line);
					}
					else if (mode == IMPL)
					{
						while (ch > line && *ch != '\t' && *ch != ' ' && *ch != '*')
							ch--;
						if (*ch == '\t' || *ch == ' ' || *ch == '*')
							ch++;

						START_LINE						
						char *s = line;
						while (s < line + i)
							fprintf(fout, "%c", *s++);
						
						if (!strncmp(s, "static ", 7))
							s += 7;
						while (s < ch)
							fprintf(fout, "%c", *s++);
						fprintf(fout, "%s::%s", class_name, ch);
					}	
				}
			}
		}
		else
		{
			if (in_method_head)
			{
				if (mode == INTER)
					fprintf(fout, ";");
				in_method_head = FALSE;
			}		
			if (mode == IMPL)
			{
				START_LINE
				fprintf(fout, "%s", line);
			}
		}		
		
		
		if (line[i] == '}')
		{	nest--;
			
			if (nest == 0)
				in_class = FALSE;
		}

		prev_skipped = !printed;
		fgets(line, 999, fin);
		line_nr++;
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2)
		return 0;
	
	char fnin[100];
	sprintf(fnin, "%s.cls", argv[1]);
	FILE *fin = fopen(fnin, "r");
	if (fin == NULL)
		return 0;
	
	char fnout[100];
	sprintf(fnout, "%s.cpp", argv[1]);
	FILE *fout = fopen(fnout, "w");
	if (fout == NULL)
		return 0;
	
	fprintf(fout, "\n// Classes defined in this file\n\n");
	process_file(NAME, fin, fout, fnin);
	fprintf(fout, "\n// Interfaces for the classes\n\n");
	process_file(INTER, fin, fout, fnin);
	fprintf(fout, "\n// Implementation of the class methodes\n\n");
	process_file(IMPL, fin, fout, fnin);
	
	fclose(fin);
	fclose(fout);
		
	return 0;
}