//
// Shell Project
//
#include "shell.h"

//
// Built-ins
//
void xsetenv(char* name, char* value) {
	int success = setenv(name, value, 1);
	if (success == 0) printf("Variable %s set to %s. \n", name, value);
	else printf(KRED "Error, could not set %s as %s. \n" RESET, name, value);
}

void xprintenv() {
	int i = 0;
	while (environ[i]) {
		printf("%s \n", environ[i++]);
	}
}

void xunsetenv(char* name) {
	int success = unsetenv(name);
	if (success != 0) printf(KRED "Error, could not remove %s. \n" RESET, name);
}

void xcd(char* path) {
	if (path == NULL) {
		char* home = getenv("HOME");
		chdir(home);
	}
	else {
		if (chdir(path) == -1) {
			printf(KRED "Error, %s is not a valid directory. \n" RESET, path);
		}
	}
}
	
char* xgetalias(char* name) {
	char* temp = llGetAlias(aliasTable, name);
	//if (temp == NULL) printf(KRED "Error, %s was not found or is circular. \n" RESET, name);
	return temp;
}

void xsetalias(char* name, char* value) {
	printf("Alias %s set to %s. \n", name, value);
	llPushAlias(aliasTable, name, value);
}

void xprintalias() {
	llPrint(aliasTable);
}

void xunalias(char* name) {
	printf("Alias %s removed. \n", name);
	llRemove(aliasTable, name);
}

void xbye() {
	printf(KGRN "Exiting [shellX] \n" RESET);
	llFree(aliasTable);
	chainReset(chainTable);
	free(chainTable);
	exit(0);
}

//
// Externals
//
void xexecute(ll* list) {
	/*if (!Executable()) {
		//use access() system call with X_OK
		printf("Command not Found");
		return;
	}
	
	/* Check io file existence in case of io-redirection.
	if (check_in_file() == SYSERR) {
		printf("Cann't read from : %s", srcf);
		return;
	}

	if (check_out_file() == SYSERR) {
		printf("Cann't write to : %s", distf);
		return;
	}*/
	
	//Build up the pipeline (create and set up pipe end points (using pipe, dup) 
	//Process background*/

	//printf("Executing %s \n", list->start->name);

	// See if command exists
	char* path = xpathlookup(list->start->name);

	if (path != NULL) {
		// matched command from path
		list->start->name = path;
	}
	else {
		// invalid command, exit
		printf(KRED "Error, could not find command %s. \n" RESET, list->start->name);
		return;
	}

	// Time to execute
	int status;
	pid_t pid = fork();

	if (pid == 0) {
		xexecutecommand(list);
		exit(0); // if command fails
	}
	else {
		llFree(list); // no longer needed
		waitpid(pid, &status, 0);
	}
}

char* xpathlookup(char* command) {
	// command already absolute, works as given
	if (access(command, X_OK) == 0) return command;

	// look if command is in one of the paths
	char* path = strdup(getenv("PATH"));
	char* parsedPath = strtok(path, ":");

	while (parsedPath != NULL) {
		char* result = malloc(strlen(command) + strlen(parsedPath) + 2); // +1 for null, +1 for slash
		strcpy(result, parsedPath);
    	strcat(result, "/");
    	strcat(result, command);

		// found the winning path!
		if (access(result, X_OK) == 0) {
			return result;
		}

		// next token
		free(result);
		result = NULL;
		parsedPath = strtok(NULL, ":");
	}

	// not found
	return NULL;
}

void xexecutecommand(ll* list) {
	// http://geoffgarside.co.uk/2009/08/28/using-execve-for-the-first-time/
	node* current = list->start;

	// Parse list into argv
	char** envp = {NULL};
	char** argv = calloc(list->count + 1, sizeof(char*));
	argv[list->count] = NULL; // NULL terminator

	int i;
	// no while loop, we need an index for argv
	for (i = 0; i < list->count; i++) {
		if (strcmp(current->name, "&") == 0) {
			// needs to be ran in the background
		}
		else {
			argv[i] = current->name;
			current = current->next;
		}
	}

	execve(argv[0], argv, envp); // or execv
}

//
// shellX code
//
void min() {
	printf(KRED "Missing required arguments. \n" RESET);
	return;
}

void ignore(int number) {
	if (number < 2) printf(KYEL "Ignoring last additional argument. \n" RESET);
	else printf(KYEL "Ignoring last %d additional arguments. \n" RESET, number);
}

void xshell(ll* list) {
	if (list == NULL || list->start == NULL) return;
	char* command = list->start->name;
	int count = list->count;

	// check if built in or command
	if (strcmp(command, "setenv") == 0) {
		if (count < 3) {min(); return;}
		else if (count > 3) ignore(count - 3);

		llPop(list); // ignore command name
		xsetenv(llPop(list), llPop(list));
	}

	else if (strcmp(command, "printenv") == 0) {
		if (count > 1) ignore(count - 1);
		xprintenv();	
	}

	else if (strcmp(command, "unsetenv") == 0) {
		if (count < 2) {min(); return;}
		else if (count > 2) ignore(count - 2);

		llPop(list); // ignore command name
		xunalias(llPop(list));
	}

	else if (strcmp(command, "cd") == 0) {
		if (count > 2) ignore(count - 2);
		
		llPop(list); // ignore command name
		xcd(llPop(list));
	}

	else if (strcmp(command, "alias") == 0) {
		if (count > 3) ignore(count - 3);

		llPop(list); // ignore command name
		char* name = llPop(list);
		char* value = llPop(list);

		if (name != NULL) {
			if (value != NULL) {
				xsetalias(name, value);
			}
			else {
				xgetalias(name);
			}
		}
		else {
			xprintalias();
		}
	}

	else if (strcmp(command, "unalias") == 0) {
		if (count > 1) ignore(count - 1);
		
		llPop(list); // ignore command name
		xunalias(llPop(list));
	}

	else if (strcmp(command, "bye") == 0) {
		xbye();
	}

	else {
		// try executing
		xexecute(list);
	}
}

void shell_init() {
	// init storage
	aliasTable = llCreate(0);
	chainTable = chainCreate(0);
	chainBuffer = NULL;

    llPush(aliasTable, "a", "c");
	llPush(aliasTable, "b", "d");
	llPush(aliasTable, "c", "a");
	llPush(aliasTable, "d", "dfinal");
	llPush(aliasTable, "q", "pwd");
	llPush(aliasTable, "w", "ls");
	
	ll* a = llCreate(1);
	llPush(a, "0", NULL);
	llPush(a, "1", NULL);
	llPush(a, "2", NULL);
	
	ll* b = llCreate(1);
	llPush(a, "b", NULL);
	llPush(a, "c", NULL);
	llPush(a, "d", NULL);
	


	/* get environment variables --NOT USED--
	PATH = getenv("PATH");
	HOME = getenv("HOME");*/
    
	// disable anything that can kill your shell. 
	/*signal(SIGINT, SIG_IGN);  // Ctrl-C
    signal(SIGQUIT, SIG_IGN); // Ctrl-backslash
    signal(SIGTSTP, SIG_IGN); // Ctrl-Z*/
}

int recover_from_errors() {
	/*printf(KRED "An exception has occured. \nRecovering [%s]... ", yytext);

	int x = yylex();
	while (x != 0 && x != 59) {
	    printf("[%s] %d ... ", yytext, x);
	}
	printf("\n" RESET);*/
	
	printf(KRED "An exception has occured. Recovering... \n" RESET);
}

int main(int argc, char *argv[]) {
	shell_init();
	printf(KGRN "Launching [shellX] \n" RESET);

	while (1) {
		// print current directory
		char* pwd = get_current_dir_name();
		printf(KMAG "%s> " RESET, pwd);
	
		// process input
		if (yyparse() == 1) recover_from_errors();

		// process chain
		chainPrint(chainTable);
		
		ll* command = chainPop(chainTable);
		while (command != NULL) {
			xshell(command);
			command = chainPop(chainTable);
		}

		printf("clear the table \n");
		// clear commands table
		chainReset(chainTable);
	}
}

