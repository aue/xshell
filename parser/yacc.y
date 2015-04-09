%{
#include "shell.h"
extern int yyterminate;

void yyerror(const char* str) {
	fprintf(stderr, KRED "Error, %s \n" RESET, str);
	// unusable command chain
	chainReset(chainTable);
}

int yywrap() {
	return 1;
}

%}
%error-verbose
%union {
	int intvar;
	char* strval;
	void* linkedlist;
}
%token STDERROUT
%token <strval> VAR
%token <strval> STRINGLITERAL
%type <linkedlist> arguments
%type <strval> ignore
%type <strval> argument
%%

commands: 
	/* blank */
	| commands command;

command:
	arguments {
		chainPush(chainTable, $1);
		YYACCEPT;
	};

arguments:
	argument {
		// First word
		ll* list = llCreate(1);
		
		// check first word for alias
		char* alias = xgetalias($1);
		if (alias != NULL) {
			llPush(list, alias, NULL);
			// kick alias back out for reparsing
		}
		else llPush(list, $1, NULL);

		$$ = list;
	}
	| arguments '|' {
		printf("pipe \n");
		chainPush(chainTable, $1);
		$$ = llCreate(1);
	}
	| arguments ignore {
		// ignore
		$$ = $1;
	}
	| arguments argument {
		llPush($1, $2, NULL);
		$$ = $1;
	};

ignore:
	STDERROUT {
		printf("error stdout \n");
		chainTable->fileErrorOut = NULL;
	}
	| '&' {
		printf("external run plz \n");
		chainTable->background = 1;
	}
	| '<' VAR {
		printf("file in \n");
		chainTable->fileIn = $2;
	}
	| '>' VAR {
		printf("file out \n");
		chainTable->fileOut = $2;
	};
	
argument:
	'$' '{' VAR '}' {
		char* value = getenv($3);
		if (value == NULL) {
			yyerror("variable is not defined");
			YYERROR;
		}
		printf("Replacing %s with %s \n", $3, value);
		$$ = value;
	}
	| '~' VAR {
		$$ = $2;
		printf("user path lookup \n");
		// look for users homepath here
	}
	| '~' {
		$$ = getenv("HOME");
	}
	| '2' '>' VAR {
		printf("error out \n");
		$$ = $3;
	}
	| STRINGLITERAL {
		// need to parse inside for alias and env
		printf("Removing quotes \n");
		$$ = $1;
	}
	| VAR {
		$$ = $1;
	};
%%

