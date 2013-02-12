#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/wait.h> 
#include <sys/types.h> 
#include <signal.h> 
#include <ctype.h> 
#include <errno.h> 
#include <glob.h>
#include <fcntl.h>
#include "hsh.h"

void prompt(char* prompt,char* buffer,int size);
TOKEN * scanner(char* string);
int addtoken(TOKEN ** token,TOKEN ** list);
int isOperator(char ch);
void showTokens(TOKEN * t);
void freeTokens(TOKEN ** t);

PROCESS* buildJob(TOKEN * t);
int countTokens(TOKEN * t);
char** tokens2array(TOKEN * t);
TOKEN * duptoken(TOKEN * t);
int countArgs(char** args);
int addprocess(PROCESS** process,PROCESS** job);
int addredirection(REDIRECTION** redirection,REDIRECTION** list);

int show_processes(PROCESS* process);
int free_processes(PROCESS* process);
int free_redirections(REDIRECTION* redirection);
int free_args(char** args);

int run_processes(PROCESS* process);
int setup_redirections(REDIRECTION* redirection);
int wait_processes(PROCESS* process);

int changedirectory(char** args);
int printworkingdirectory();

TOKEN * globToken(TOKEN* token);

int main()
{
	char cmdline[MAXBUFFER]; 
	TOKEN * tokens = NULL;
	PROCESS* job = NULL;
	int done = 0; 

	while (!done)
	{
		prompt(" ->> ",cmdline,MAXBUFFER);
		tokens = scanner(cmdline);
		if(tokens)
		{
			/* showTokens(tokens); */
			if(!strcmp(tokens->value,"exit"))
			{
				done = 1;
			}
			else
			{
				job = buildJob(tokens);
				if(job)
				{
					/* show_processes(job); */
					run_processes(job);
					wait_processes(job);
					free_processes(job);
					job = NULL;
				}
			}
			freeTokens(&tokens);
		}
	}
	
	return 0;
}

void prompt(char* prompt,char* buffer,int size)
{
	char pstring[MAXBUFFER]; 

	getcwd(pstring,MAXBUFFER); 
	strcat(pstring,prompt); 
	printf("%s",pstring); 
	fgets(buffer,size, stdin); 
	buffer[strlen(buffer)-1] = '\0'; 
}

TOKEN * scanner(char* string)
{
        TOKEN * tokens = NULL;
	TOKEN * token = NULL;
        char* strptr = string;
	char* lookahead = NULL;
	char* start_identifier = NULL;
	int size = 0;
        STATE state = DEFAULT;


        while(*strptr)
        {
                switch (state)
                {
                        case DEFAULT:
				if(isspace(*strptr)) while(isspace(*strptr)) strptr++;
				else if(*strptr == '#')	state = COMMENT;
				else if(isOperator(*strptr))
				{
					if(*strptr == '|') state = SAW_PIPE;
					else if(*strptr == '&')	state = SAW_AMPERSAND;
					else if(*strptr == '<') state = SAW_LESS_THAN;
					else if(*strptr == '>') state = SAW_GREATER_THAN;
				}
				else if(!isOperator(*strptr)) state = IN_IDENTIFIER;
                                break;
			case COMMENT:
				while((*strptr != '\n') && (*strptr)) strptr++; 
				break;
                        case SAW_PIPE:
				token = (TOKEN *) malloc(sizeof(TOKEN));
				token->value = (char*) malloc(sizeof(char)*2);
				strcpy(token->value,"|");
				token->type = PIPE;
				token->next = NULL;
				addtoken(&token,&tokens);
				strptr++;
				state = DEFAULT;
                                break;
                        case SAW_AMPERSAND:
				lookahead = strptr;
				lookahead++;
				if(*lookahead=='>')
				{
					token = (TOKEN *) malloc(sizeof(TOKEN));
					token->value = (char*) malloc(sizeof(char)*3);
					strcpy(token->value,"&>");
					token->type = REDIRECT_ERROR;
					token->next = NULL;
					addtoken(&token,&tokens);
					strptr++;
					strptr++;
					state = DEFAULT;	
				}
				else
				{
					strptr++;
					state = ERROR;
				}
                                break;
                        case SAW_LESS_THAN:
				token = (TOKEN *) malloc(sizeof(TOKEN));
				token->value = (char*) malloc(sizeof(char)*2);
				strcpy(token->value,"<");
				token->type = REDIRECT_IN;
				token->next = NULL;
				addtoken(&token,&tokens);
				strptr++;
				state = DEFAULT;
				break;
                        case SAW_GREATER_THAN:
				lookahead = strptr;
				lookahead++;
				if(*lookahead=='>')
				{
					token = (TOKEN *) malloc(sizeof(TOKEN));
					token->value = (char*) malloc(sizeof(char)*3);
					strcpy(token->value,">>");
					token->type = REDIRECT_OUT_APPEND;
					token->next = NULL;
					addtoken(&token,&tokens);
					strptr++;
					strptr++;
					state = DEFAULT;	
				}
				else
				{
					token = (TOKEN *) malloc(sizeof(TOKEN));
					token->value = (char*) malloc(sizeof(char)*2);
					strcpy(token->value,">");
					token->type = REDIRECT_OUT_OVERWRITE;
					token->next = NULL;
					addtoken(&token,&tokens);
					strptr++;
					state = DEFAULT;
				}
                                break;
			case IN_IDENTIFIER:
				start_identifier = strptr;
				size = 0;
				while(!(isOperator(*strptr) || isspace(*strptr) || !*strptr))
				{
					size++;
					strptr++;
				}
				token = (TOKEN *) malloc(sizeof(TOKEN));
				token->value = (char*) malloc(sizeof(char)*(size+1));
				strncpy(token->value,start_identifier,size);
				token->value[size]='\0';
				token->type = IDENTIFIER;
				token->next = NULL;
				token = globToken(token);
				addtoken(&token,&tokens);
				state = DEFAULT;
				break;
                        case ERROR:

                                break;
                }
        }

        return tokens;
}

int addtoken(TOKEN ** token,TOKEN ** list)
{
	if(*list)
	{
		addtoken(token,&((*list)->next));
	}
	else
	{
		*list = *token;
	}
	return 0;
}

int isOperator(char ch)
{
	int result;
	
	switch (ch)
	{
		case  '>':
		case  '<': 
		case  '&': 
		case  '|': 
			result = 1;
			break;
		default:
			result = 0;
	}

	return result; 
}

void showTokens(TOKEN * t)
{
	if(t)
	{
		printf("\t%15s\t%d\n",t->value,t->type);	
		showTokens(t->next);
	}
}

void freeTokens(TOKEN ** t)
{
	if(*t)
	{
		if(&((*t)->next)) freeTokens(&((*t)->next));
		if((*t)->value) free((*t)->value);
		free(*t);
		*t = NULL;
	}
}


PROCESS* buildJob(TOKEN * t)
{
	PROCESS* job = NULL;
	PROCESS* process = NULL;
	TOKEN * token = NULL;
	TOKEN * args = NULL;
	REDIRECTION* redirection = NULL;
	int error = 0;

	while(t && !error)
	{
		if(process)
		{
			if(t->type == IDENTIFIER)
			{
				token = duptoken(t);
				addtoken(&token,&args);
				token = NULL;
			}
			else if(t->type == PIPE)
			{
				process->args  = tokens2array(args);
				process->next = NULL;
				addprocess(&process,&job);
				freeTokens(&args);
				process = NULL;
			}
			else
			{
				redirection = (REDIRECTION*) malloc(sizeof(REDIRECTION)); 
				switch(t->type)
				{
					case REDIRECT_OUT_OVERWRITE:
						redirection->type = RDCT_OUT_OVERWRITE;
						break;
					case REDIRECT_OUT_APPEND:
						redirection->type = RDCT_OUT_APPEND;
						break;
					case REDIRECT_IN:
						redirection->type = RDCT_INPUT;
						break;
					case REDIRECT_ERROR:
						redirection->type = RDCT_ERROR;
						break;
					default:
						error = 1;
				}		
				if(t->next == NULL)
				{
					fprintf(stderr,"hsh: syntax error expected token missing\n"); 
					error = 1;
				}
				else
				{
					t = t->next;
					if(t->type == IDENTIFIER)
					{
						if(t->value)
						{
							redirection->filename = (char*) malloc(sizeof(char)*(strlen(t->value)+1));
							strcpy(redirection->filename,t->value);
							redirection->next = NULL;
							addredirection(&redirection,&(process->redirections));	
						}
						else
						{
							error = 1;
						}
					}
					else
					{
						fprintf(stderr,"hsh: syntax error invalid token after redirection\n");
						error = 1;
					}
				}	
			}
		}
		else
		{
			if(t->type == IDENTIFIER)
			{
				process = (PROCESS*) malloc(sizeof(PROCESS));
				process->redirections = NULL;
				token = duptoken(t);
				addtoken(&token,&args);
				token = NULL;
			}
			else
			{
				fprintf(stderr,"hsh: syntax error unexpected token\n"); 
				error = 1;
			}

		}
		t = t->next;
	}
	if(process)
	{
		process->args = tokens2array(args);
		process->next = NULL;
		addprocess(&process,&job);
		freeTokens(&args);
		process = NULL;
	}

	if(error)
	{
		if(job)
		{
			free_processes(job);
		}
		job = NULL;
	}

	return job;
}

int countTokens(TOKEN * t)
{
	if(t)
	{
		return 1 + countTokens(t->next);
	}
	else
	{
		return 0;
	}
}

char** tokens2array(TOKEN * t)
{
	char** array = NULL;
	int count = 0;
	int i = 0;
	int size = 0;

	count = countTokens(t);
	array = (char**) malloc(sizeof(char*)*(count+1));
	
	for(i=0; i < count; i++)
	{
		size = strlen(t->value);
		array[i] = (char*) malloc(sizeof(char)*(size+1));
		strcpy(array[i],t->value);
	
		t = t->next;
	}
	array[count] = NULL;

	return array;
}
	
TOKEN * duptoken(TOKEN * t)
{
	TOKEN * nt = NULL;

	nt = (TOKEN *) malloc(sizeof(TOKEN));
	nt->value = (char*) malloc(sizeof(char)*(strlen(t->value)+1));
	strcpy(nt->value,t->value);
	nt->type = t->type;
	nt->next = NULL;
	
	return nt;
}

int countArgs(char** args)
{
	int count = 0;
	char** argloc = args;

	while(*argloc)
	{
		count++;
		argloc++;
	}

	return count;
}

int addprocess(PROCESS** process,PROCESS** job)
{
	if(*job)
	{
		addprocess(process,&((*job)->next));
	}
	else
	{
		*job = *process;
	}
	return 0;
}

int addredirection(REDIRECTION** redirection,REDIRECTION** list)
{
	if(*list)
	{
		addredirection(redirection,&((*list)->next));
	}
	else
	{
		*list = *redirection;
	}
	return 0;
}

int show_processes(PROCESS* process)
{
	int i = 0;
	char** args;
	REDIRECTION* redirection;
	PROCESS* p = process;


	while(p)
	{
		args = p->args;

		printf("The values of the process structure is:\n");
		if(args)
		{
			printf("\tThe Arguments are:\n");
			printf("\t\tArg#\tValue\n");
			for(i=0;*args;i++)
			{
				printf("\t\t%d\t%s\n",i,*args); 
				args++;
			}
		}
		if(p->redirections)
		{
			printf("\tThis process has redirection(s)\n");
			redirection = p->redirections;
			while(redirection)
			{
				printf("\n\tThis redirection's values are:\n");
				printf("\t\tField\tValue\n");
				printf("\t\tType\t%d\n",redirection->type);
				printf("\t\tFilename\t%s\n",redirection->filename);
				redirection = redirection->next;	
			}
		}
		p = p->next;
	}

	return 0;
}

int free_processes(PROCESS* process)
{
	if(process)
	{
		if(process->next)
		{
			free_processes(process->next);
		}
		free_redirections(process->redirections);
		process->redirections = NULL;
		free_args(process->args);
		process->args = NULL;
		free(process);

	}
	return 0; 
}

int free_redirections(REDIRECTION* redirection)
{
	if(redirection)
	{
		if(redirection->next)
		{
			free_redirections(redirection->next);
		}
		if(redirection->filename)
		{
			free(redirection->filename);
		}
		free(redirection);

		redirection = NULL;
	}	
	return 0;
}

int free_args(char** args)
{
	int size;
	int count = 0;

	size = countArgs(args);

	for(count = 0; count < size; count++)
	{
		free(args[count]);
	}
	free(args);

	return 0;
}

int run_processes(PROCESS* process)
{
	int pipefds[2]; /* A pipe for ipc */
	int in=STDIN_FILENO;
	int out=STDOUT_FILENO;
	PROCESS* p = process;

	while(p)
	{
		if(p->next)
		{
			pipe(pipefds);
			out = pipefds[OUT];
		}
		else
		{
			out = STDOUT_FILENO;
		}

		if(!strcmp(*p->args,"cd"))
		{
			changedirectory(p->args);
			p->pid = 0;
		}
		else if(!strcmp(*p->args,"pwd"))
		{
			printworkingdirectory();
			p->pid = 0;
		}
		else
		{

			if((p->pid=fork())==0)
			{
				if(in != STDIN_FILENO) /* get RDCT_INPUT from the pipe */
				{
					dup2(in,STDIN_FILENO);
					close(in);
				}
				if(out != STDOUT_FILENO) /* send output to the pipe */
				{
					dup2(out,STDOUT_FILENO);
					close(out);
				}
				if(p->redirections) /* redirections trump the pipe */
				{
					setup_redirections(p->redirections);
				}

				execvp(*p->args,p->args);
				fprintf(stderr,"hsh: %s: command not found\n",*p->args);
			}
		}
		if(in != STDIN_FILENO) close(in);
		if(out != STDOUT_FILENO) close(out);
		if(p->next)
		{
			in = pipefds[IN];
		}
		p = p->next;	
	}
	
	return 0;
}

int setup_redirections(REDIRECTION* redirection)
{
	int mode;
	int fd;
	
	if(redirection->next)
	{
		setup_redirections(redirection->next);
	}

	switch (redirection->type)
	{
		case RDCT_INPUT:
			mode = O_RDONLY;
			break;
		case RDCT_OUT_APPEND:
			mode = O_RDWR | O_CREAT | O_APPEND;
			break;
		case RDCT_OUT_OVERWRITE:
		case RDCT_ERROR:
			mode = O_RDWR | O_CREAT | O_TRUNC;
			break;
	}

	fd = open(redirection->filename, mode, 0666);

	switch (redirection->type)
	{
		case RDCT_INPUT:
			dup2(fd,STDIN_FILENO);
			break;
		case RDCT_OUT_APPEND:
		case RDCT_OUT_OVERWRITE:
			dup2(fd,STDOUT_FILENO);
			break;
		case RDCT_ERROR:
			dup2(fd,STDERR_FILENO);
			break;
	}

	return 0;
}

int wait_processes(PROCESS* process)
{
	int status=0;

	if(process)
	{
		if(process->next)
		{
			status = wait_processes(process->next);
		}
		waitpid(process->pid,&status,0);
	}

	return status;
}

int changedirectory(char** args)
{
	int argsnum = 0;

	argsnum = countArgs(args);

	if(argsnum == 1)
	{
		chdir(getenv("HOME"));
	}
	else
	{
		if(chdir(args[1]))
		{
			printf("hsh: %s: No such file or directory\n",args[1]);
		}
	}
	
	return 0;
}

int printworkingdirectory()
{
	fprintf(stdout,"%s\n",getenv("PWD"));
	
	return 0;
}

TOKEN * globToken(TOKEN* token)
{
	glob_t result;
	int flags;
	int i = 0;
	TOKEN* newtoken = NULL;

	flags = GLOB_NOCHECK;

	glob(token->value,flags,NULL,&result);

	freeTokens(&token);
	token = NULL;
		
	for(i=0;i<result.gl_pathc;i++)
	{
		newtoken = (TOKEN *) malloc(sizeof(TOKEN));
		newtoken->value = (char*) malloc(sizeof(char)*(strlen(result.gl_pathv[i])+1));
		strcpy(newtoken->value,result.gl_pathv[i]);
		newtoken->type = IDENTIFIER;
		newtoken->next = NULL;
		addtoken(&newtoken,&token);
	}

	globfree(&result);
	return token;
}

