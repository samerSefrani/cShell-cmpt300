#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>



#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define nullptr ((void*)0)



//Global stuff
typedef struct {
    char name[1000];
    char time[64];
    int value;
} Command;

typedef struct {
    char name[1000];
    char value[1000];
} EnvVar;

int consoleColor = 0;
Command commandLogs[1000];
EnvVar envVars[1000];
int numOfCommand = 0;
int numOfCommandArgs = 0;




//To print string in a specific color
void print(char *str, int colorCode){
    if(colorCode ==1){
        printf( ANSI_COLOR_BLUE);
    }else if(colorCode ==2){
        printf( ANSI_COLOR_RED);
    }else if(colorCode ==3){
        printf( ANSI_COLOR_GREEN);
    }
    printf("%s", str);
    printf(ANSI_COLOR_RESET);
}


void executeNonBuiltInCommands(char *command, int consoleColor){

    //Refrenced: http://www.microhowto.info/howto/capture_the_output_of_a_child_process_in_c.html

    int filedes[2];
    if (pipe(filedes) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) {    //fork faild
        perror("fork");
        exit(1);
    }else if (pid == 0) {   //Child
        //while ((dup2(filedes[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}    //we dont need dup2 0_o
        close(filedes[1]);
        close(filedes[0]);
        execlp(command, command, (char*)0);
        //perror("execlp");
        print("Missing keyword or command, or permission problem.\n", consoleColor);
        exit(1);
    }
    close(filedes[1]);


    //Read the output of the child proccess from the pipe
    char buffer[1000];
    while (1) {
        ssize_t count = read(filedes[0], buffer, sizeof(buffer));
        if (count == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("read");
                exit(1);

            }
        } else if (count == 0) {
            break;
        } else {
            print(buffer, consoleColor);
        }
    }
    close(filedes[0]);
    wait(NULL);
    return;
}


//djb2 Algorithm Refrenced: https://stackoverflow.com/questions/7666509/hash-function-for-string
unsigned long hash(char *str){
    unsigned long hash = 999;
    int c;
    while (((c) = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}


void parseThenExecute(char *userInput){

    //parse user inputs into tokens
    // Refrenced: http://www.cplusplus.com/reference/cstring/strtok/
    char * oneWord;
    oneWord = strtok (userInput," ");
    if (!oneWord) return; // if no input and user presses enter -> do nothing
    if (oneWord) strcpy(commandLogs[numOfCommand].name, oneWord);


    //Refrenced: https://stackoverflow.com/questions/1442116/how-to-get-the-date-and-time-values-in-a-c-program
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char dateOfCommand[64];
    strftime(dateOfCommand, sizeof(dateOfCommand), "%c", tm);
    strcpy(commandLogs[numOfCommand].time, dateOfCommand);


    //check for "exit" command
    if(strstr(oneWord, "exit")) {
        print("Bye!\n", consoleColor);
        exit(0);
    }

    //check for "$" command (env var stuff)
    if(strstr(oneWord, "$")) {
        oneWord = strtok (userInput," $=");
        int index = hash(oneWord ) % 1000;
        strcpy (envVars[index].name, oneWord);
        //Use the envVar as an index by hashing it, then calculate the modulo by setting the max number as 1000
        while (oneWord)
        {
            oneWord = strtok (NULL, "$ =");
            if (oneWord) strcpy (envVars[index].value, oneWord);
        }
        return;
    }

    //check for "print" command
    if(strstr(oneWord, "print")) {
        oneWord = strtok (NULL, " "); //to skip printing "print" in the results
        if(!oneWord) return;

        while (oneWord != NULL) //normal print mode
        {
            if(strstr(oneWord, "$")) { //env vars print mode
                char *wordWithSign = oneWord;
                oneWord = strtok (wordWithSign," $");
                if (!oneWord) return;
                int index = hash(oneWord) % 1000;

                print(envVars[index].value, consoleColor);
                oneWord = strtok (NULL, " ");
                continue;
            }
            numOfCommandArgs++;
            char oneWordWithSpace[1000];
            strcpy (oneWordWithSpace, oneWord);
            strcat(oneWordWithSpace, " ");
            print(oneWordWithSpace, consoleColor);
            oneWord = strtok (NULL, " ");
        }
        print("\n", consoleColor);
        return;   //bcz if we get here this means oneWord is empty and we should not move to the next lines...
    }

    //check for "log" command
    if(strstr(oneWord, "log")) {
        for(int i = 0; i <= numOfCommand; i++){

            //Print date
            char appendNewLineToDate[1000];
            strcpy (appendNewLineToDate, commandLogs[i].time);
            strcat(appendNewLineToDate, "\n");
            print(appendNewLineToDate, consoleColor);
            

            //Print name of command and value(number of args)
            //First do this (name + " " + value)
            char appendSpaceToCommand[1000];
            char copyCommandValue[1000];
            strcpy (appendSpaceToCommand, commandLogs[i].name);
            strcat(appendSpaceToCommand, " ");

            //convert "value" from Int to String
            //Refrenced: https://stackoverflow.com/questions/8257714/how-to-convert-an-int-to-string-in-c/23840699#:~:text=You%20can%20use%20itoa(),to%20convert%20any%20value%20beforehand.
            sprintf( copyCommandValue, "%d", commandLogs[i].value);
            strcat(copyCommandValue, "\n");

            strcat(appendSpaceToCommand, copyCommandValue);
            print(appendSpaceToCommand, consoleColor);

        }
        return;   //bcz if we get here this means oneWord is empty and we should not move to the next lines...
    }

    //check for "theme x" command
    if(strstr(oneWord, "theme")){
        numOfCommandArgs++;
        int tempOldColor = consoleColor;
        while (oneWord)
        {
            //Refrenced: https://www.theurbanpenguin.com/4184-2/
            if(strstr(oneWord, "blue")){
                consoleColor = 1;
            }else if(strstr(oneWord, "red")){
                consoleColor = 2;
            }else if(strstr(oneWord, "green")){
                consoleColor = 3;
            }
            oneWord = strtok (NULL, " ,.-");
        }

        //if the color was not updated this means -> the requested color is not valid
        if(consoleColor == tempOldColor){
            print("Unsupported theme! \n", consoleColor);
        }

        return;
    }
    executeNonBuiltInCommands(oneWord, consoleColor);

}


void interactiveMode(){

    while(1){
        numOfCommandArgs = 0;
        char userInput [256];
        print("cshell$ ", consoleColor);
        //gets (userInput);   //one line
        fgets(userInput, sizeof userInput, stdin);
        userInput[strcspn(userInput, "\r\n")] = 0;

        parseThenExecute(userInput);

        commandLogs[numOfCommand].value = numOfCommandArgs;
        ++numOfCommand;
    }
}


void scriptMode(char *fileName){

    FILE* file = fopen(fileName, "r");
    if(!file){
        perror("File Name");
        return;
    }

    char oneLine[1000];

    while (fgets(oneLine, sizeof(oneLine), file)){
        numOfCommandArgs = 0;
        oneLine[strcspn(oneLine, "\r\n")] = 0;  //fixes the stupid bug
        parseThenExecute(oneLine);
        commandLogs[numOfCommand].value = numOfCommandArgs;
        ++numOfCommand;
    }
    print("Bye!", consoleColor);
    print("\n", consoleColor);
    
    fclose(file);

    return;
}


int main(int argc, char **argv)
{
    argc > 1 ? scriptMode(argv[1]) : interactiveMode();

    return 0;
}