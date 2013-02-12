#include <unistd.h>

#define IN 0
#define OUT 1
#define ERR 2
#define MAXBUFFER 256

enum tokenType
{
        IDENTIFIER,
        PIPE,
        REDIRECT_OUT_OVERWRITE,
        REDIRECT_OUT_APPEND,
        REDIRECT_IN,
        REDIRECT_ERROR
};

typedef enum tokenType TOKENTYPE;

struct token
{
        char* value;
        TOKENTYPE type;
        struct token* next;
};

typedef struct token TOKEN;

enum state
{
        DEFAULT,
	COMMENT,
        SAW_PIPE,
        SAW_AMPERSAND,
        SAW_LESS_THAN,
        SAW_GREATER_THAN,
	IN_IDENTIFIER,
        ERROR
};

typedef enum state STATE;

enum redirect_type
{
	RDCT_INPUT,
	RDCT_OUT_OVERWRITE,
	RDCT_OUT_APPEND,
	RDCT_ERROR
};

struct redirection
{
	enum redirect_type type;
	char* filename;
	struct redirection* next;
};

typedef struct redirection REDIRECTION;

struct process
{
	pid_t  pid;
	char** args;
	struct process* next;
	struct redirection* redirections;
};

typedef struct process PROCESS;
