#include <stdio.h>  // for perror
#include <stdlib.h>   // for exit
#include <unistd.h>   // for execv
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include<fcntl.h>
#include<errno.h>
#include <string.h>
#define SIGTSTP 20 
#include<sys/wait.h>



int signalCheck; // checks for forground and background entry
int curPid; // holds the current background pid
char *checkIn; // checks to see if the current input has a & at the end for background processes
int curState; // holds the current state

// used for different execvp's

int ls(char** pathName){
// this is for any cmd that doesn't contain the word "kill", this was easiest to use as the kill cmd
// needs to use differnet parameters to be executed.
  if(!strstr(pathName[0], "kill")){
    execvp(pathName[0], pathName); 
    
// this next one is for the pkill command as the script pkill sleep doesn't contain
// a signal, it pulls the curPid and then uses it to kill the cmd
// unfortunatly if you were to run pkill with multiple sleep cmds running in the background it would only 
// kill the first one, I wasn't sure how to correctly implement this.
  }else if(strstr(pathName[0], "pkill")){
    char spid[40];
    int kl = snprintf(spid, 20, "%d", curPid);
    char *argv[] = {"/bin/kill", "-2", spid, NULL};  
    printf("Killed off proccess %d: with termination status 2\n", curPid); 
    fflush(stdout);                   
    execvp(argv[0], argv);
    
// this last one checks that the kill signal is not equal to 20 as I have a seperate way of handling that
// this and the one above takes in the curPid (I tried to get the passed in pid but had some weird errors)
// so it converts the curPid into a string and puts it into an argv array and then passes it into the execvp
// function to run the cmd in the consle.
  } else {
  if(!(strncmp(pathName[1], "-20",3) == 0)){
      char spid[40];
      int kl = snprintf(spid, 20, "%d", curPid);
      printf("Killed off proccess %d: with termination status %s\n", curPid, pathName[1]);
      fflush(stdout);
      char *argv[] = {"/bin/kill", pathName[1], spid, NULL};                      
      execvp(argv[0], argv); 
    }
  } 
}

// this function checks for an occurance of "$$" and then replaces it with the current pid.
char* dolladolla(char* userInput){
    if (strstr(userInput, "$$")){
    // use strcspn to find the index of "$$"
      userInput[strcspn(userInput, "$$")] = 0;
      char holder[100];
      // puts it into a holder string and then puts the string together with the getPid().
      int can = snprintf(holder, 100, "%s", userInput);
      int cu = snprintf(userInput, 100, "%s%d", holder, getpid());
    }
}

// this puts all the arguments into an array and then passes back that array.
// uses malloc to save space on the heap, as only making this in this function will only
// work on the stack.
char** args(char* input){
  char ** array = malloc(512 * sizeof(char*));
  char* word;
  int i = 0;
  // checks for the first occurance of a space and saves the stuff before into word
  word = strtok (input," ");
  
  // while word is not null, >, <, newline, or & with newline put the words into the malloc array
  while (word != NULL && (strncmp(word, ">",1) != 0) && (strncmp(word, "<",1) != 0) && (strncmp(word, "\n",1) != 0) && (strncmp(word, "&\n",1) != 0)){
    array[i] = malloc(40 * sizeof(char));
    if(i == 0){
      int a = snprintf(array[0], 100, "/bin/%s", word);
    } else {
      // this checks for the words -SIGINT and -SIGTSTP as they don't seem to work in execvp
      // research showed that they were equal to -2 and -20 respectivly. 
      if(strncmp(word, "-SIGINT", 7) ==0 ){
        int ka = snprintf(array[i], 100, "-2");
      } else if(strncmp(word, "-SIGTSTP", 8) == 0) {
        int la = snprintf(array[i], 100, "-20");
      }else{
        int can = snprintf(array[i], 50, "%s", word);
      }
    } 
    word = strtok (NULL, " ");
    i = i + 1;
  }
  return array;
}

// this function checks for the > and <, and then uses them to read in from a file or print to a file.
void inputOrOutput(char* userInput){
  char* copy [258];
  userInput[strcspn(userInput, "\n")] = 0;
  int ca = snprintf(copy, 258, "%s", userInput);
  char* input = strstr(userInput, " < ");
  char* output = strstr(userInput, " > ");
  char ** theseargs = args(copy);
  
  // checks if an occurance of < has been found in the userInput.
  if(input){
    userInput = userInput + 5;
    char word[100];
    int x = snprintf(word, 80, "%s", strtok(userInput," "));
      dolladolla(userInput);
      int input_file = open(userInput, O_RDONLY); 
      
      if(input_file > 0){
        int stdin = dup(0);
        dup2(input_file,0); 
        
          // checks if an occurance of > has been found in the userInput which will output the input into a document.
        if( output ){
          userInput = userInput + (strlen(userInput) - strlen(output)) + 3;
          // open file with only writing allowed.
          FILE * newFile = fopen(userInput, "w");
          // get the file number for dup2.
          int fd = fileno(newFile);
          dup2(fd, 1);   // make stdout go to file
          dup2(fd, 2);   // make stderr go to file
          // send the arguments to the execvp function ls
          ls(theseargs);
          // close all files when finished
          close(fd);  
          fclose(newFile);
          close(input_file);
          
          // if no output file then skip opening the other file and just go straight to sending the 
          // arguments to ls.
        }else{
          ls(theseargs); // now execute
          dup2(stdin, 0);
          close(input_file);
        }
        
        // catch errors.
      } else {
        printf("File does not exist!\n");
        fflush(stdout);
      }
        // checks if an occurance of > has been found in the userInput, this is for only output situations
  } else if(output){
      userInput = userInput + (strlen(userInput) - strlen(output)) + 3;
      // checks last time for occurence of $$.
      dolladolla(userInput);
      FILE * newFile = fopen(userInput, "w");
      int fd = fileno(newFile);
      dup2(fd, 1);   // make stdout go to file
      dup2(fd, 2);   // make stderr go to file
      ls(theseargs);
      close(fd);  
      fclose(newFile);
  } else {
    ls(theseargs);
  } 
}

// this is for the signal SIGCHLD, it checks if the input has an & and then newline
// if it does then it will print the held curPid and the exit status.
void catch()
{
  if(strstr(checkIn, "&\n")){
    int st = 0;
    // get exit status
    waitpid(curPid, &st, 0);
    printf("\nbackground pid %d is done: exit value %d\n:", curPid, st );
    fflush(stdout);
  }
}
    



int main(){
  int childStatus;
  char* userInput[2048];
  // initialize values to 0.
  signalCheck = 0;
  curPid = 0;
  curState = 0;
  
  while(!(strncmp(userInput, "exit", 4) == 0)){
    printf(": ");
    checkIn = userInput;
    // read in user input.
    fgets(userInput, 2048, stdin);
    userInput[strcspn(userInput, "\n")] = 0;
    dolladolla(userInput);
    
    // uses signal to catch the result of a background process
    if(strstr(userInput, "&\n") && signalCheck == 0){
        signal(SIGCHLD, catch);
    }
    
    // execute these next cmds before forking, because its the parent that needs to 
    // change the directory, make a directory, or get the status.
    // Doing it otherwise would result in only the child being in a different directory.
    
    // in case of cmd cd
    if(strncmp(userInput, "cd", 2) == 0){
      char* newDir = userInput;
      char* directName[40];
      newDir[strcspn(newDir, "\n")] = 0;
      
      // checks if it's only "cd ", if it is then it gets the environment variable HOME
      // else it will get the new directory (moving the pointer up 3 to account for "cd "
      // then changes the directory.
      if( strlen(newDir) <= 3){
        chdir(getenv("HOME"));
      } else {
        newDir = newDir + 3;
        int cu = snprintf(directName, 100, "%s", newDir);
        chdir(directName);
      }
      
    // in case of cmd mkdir
    }else if(strncmp(userInput, "mkdir", 5) == 0){
      char* makDir = userInput;
      char* directName[40];
      makDir[strcspn(makDir, "\n")] = 0;
      makDir = makDir + 6;
      // moves pointer up 6 spaces and then makes a directory using the name, it also checks for 
      // occurence of $$.
      dolladolla(makDir);
      int cx = snprintf(directName, 100, "%s", makDir);
      // make directory with mode 0750.
      mkdir(directName, 0750);
            
    // in case of cmd status        
    }else if(strncmp(userInput, "status", 6) == 0){
      // uses curState to get the current state.
      printf("Exit status is: %d\n", curState);
      fflush(stdout);
      
    // this will excute all the other cmds
    }else {
      // create a child process.
      pid_t spawnPid = fork();
      int status;
    
      switch(spawnPid){
        case -1:
          // error in fork
          perror("fork()\n");
          exit(1);
          break;
          
        case 0:
          // In the child process
          // just exits the cmd
          if (strncmp(userInput, "exit", 4) == 0){
            exit(2);
            break;
            
          // checks for a comment or blank and just exits.
          } else if(strncmp(userInput, "#", 1) == 0 || strncmp(userInput, "", 5) == 0 ){
            exit(2);
            break;
            
          // prints the current working directory. 
          } else if(strncmp(userInput, "pwd", 3) == 0 ){
            char cwd[256];
            printf("%s\n", getcwd(cwd, sizeof(cwd)));
            exit(2);
            break;
            
          // prints out the contents of a file.  
          } else if(strncmp(userInput, "cat", 3) == 0 ) {
            char* fileName = userInput;
            fileName[strcspn(fileName, "\n")] = 0;
            fileName = fileName + 4;
            dolladolla(fileName);
            FILE *reading = fopen(fileName, "r");
            char word;
            word = fgetc(reading);
            // get word character by character.
            
            while (word != EOF){
               printf("%c", word);
               word = fgetc(reading);
            }
            fclose(reading);
            printf("\n");
            exit(2);
            break;
            
          // checks for echo cmd. if echo it just prints out the rest of the userInput.
          } else if(strncmp(userInput, "echo", 4) == 0 ){
            char* echoed = userInput;
            echoed[strcspn(echoed, "\n")] = 0;
            dolladolla(echoed);
            echoed = echoed + 4;
            printf("%s\n", echoed);
            exit(2);
            break;
            
          // catch all the rest and if the input has a space in it (multiple words)
          // if it does then it passes the input into the inputOrOutput function.  
          } else {
            if(strchr(userInput, ' ')){
              inputOrOutput(userInput);
              exit(2);
              break;
              
            // if it doesn't have any spaces it just sends the cmd directly to execv   
            } else {
              char path [100];
              int a = snprintf(path, 100, "/bin/%s", strtok(userInput,"\n"));
              char *newargv[] = { path, NULL };
              execv(path, newargv);
              fflush(stdout);
              perror("execve");
              exit(2);
              break;
            }

          }
          
        default:
          // In the parent process
          // Wait for child's termination
          
          // checks for exit cmd and exits the program.
          if(strncmp(userInput, "exit", 4) == 0){
            return EXIT_SUCCESS;
          }
          
          // this is for moving between the foreground and background,
          // uses the signalCheck to see if it already is in the foreground only mode
          // will switch to the opposite.
          if( strstr(userInput, "-SIGTSTP") && signalCheck == 0){
            signalCheck = 1;
            printf("Entering foreground-only mode (& is now ignored)\n");
            fflush(stdout);
          } else if(strstr(userInput, "-SIGTSTP")) {
            signalCheck = 0;
            printf("Exiting foreground-only mode\n");
            fflush(stdout);
          }
          
          // this catches all the children, if it has an &\n (& is the last character before newline), 
          // then it uses WNOHANG to not wait for result of child. it also checks for signalCheck to ensure
          // that while in foreground only mode it doesn't run it in the background.
          if(strstr(userInput, "&\n") && signalCheck == 0){
            printf("background pid is %d\n", spawnPid);     
            curPid = spawnPid;        
            spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
            
          // catches all other children using waitpid to wait on childs exit status.
          // then sets curState to the exited status of the child.
          }else{
            spawnPid = waitpid(spawnPid, &childStatus, 0);
            fflush(stdout);
            curState = WEXITSTATUS(childStatus);
          } 
          }
        }
    }
}