#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <pwd.h>
#include <limits.h>

# define PARSE_CONDITION(c) memcmp(input, c , strlen(c)) == 0
#define READ 0
#define WRITE 1

void console_log(const char* str){
    printf(">>>%s\n",str);
}
void error_log(const char* str){
    printf("ERROR: %s\n", str);
}


int login_func(char input[100], int pipe[2]){
    FILE* fd = fopen("../users.txt","r");
    if (fd == NULL){
        close(pipe[READ]);
        write(pipe[WRITE], "9:not found", strlen("9:not found"));
        return -1;
    }

    char line[100];
    bool found = false;
    while (fgets(line, sizeof(line), fd)){
        if (strcmp(input + strlen("login:"), line) == 0) {
            close(pipe[READ]);
            write(pipe[WRITE], "7:success", strlen("7:success"));

            found = true;
            break;
        }
    }
    fclose(fd);

    if (!found){
        close(pipe[READ]);
        write(pipe[WRITE], "6:failed", strlen("6:failed"));
        return 0;
    }

    return 1;
}

char* check_correct_size(char* msg){
    char* st = strtok(msg,":");
    char* end;
    int len = strtol(msg, &end, 10);
    if (len != strlen(st + strlen(st) + 1)){
        return NULL;
    }
    return st + strlen(st) + 1;
}

int mystat_func(char input[100], const char* fifo_name){
    struct stat st;
    char permisiuni[10]="---------";

    FILE* fd = fopen(fifo_name, "w");

    system("pwd");
    if(0 != stat(input + strlen("mystat "),&st) )
    {
        fprintf(fd, "6:failed");
        fclose(fd);
        return -1;
    }

    fprintf(fd, "\nUID: %ld", (long) st.st_uid);
    fprintf(fd, "\nGID: %ld", (long) st.st_gid);

    fprintf(fd,"\nType: ");
    switch( S_IFMT & st.st_mode )
    {
        case S_IFDIR : fprintf(fd,"Director"); break;
        case S_IFREG : fprintf(fd,"File"); break;
        case S_IFLNK : fprintf(fd,"Link"); break;
        case S_IFIFO : fprintf(fd,"Fifo"); break;
        case S_IFSOCK: fprintf(fd,"Socket"); break;
        default: fprintf(fd,"Unknown file type");
    }

    fprintf(fd, "\nSize: %lld kb",(long long)st.st_size/1024);

    if(S_IRUSR & st.st_mode) permisiuni[0]='r';
    if(S_IWUSR & st.st_mode) permisiuni[1]='w';
    if(S_IXUSR & st.st_mode) permisiuni[2]='x';
    if(S_IRGRP & st.st_mode) permisiuni[3]='r';
    if(S_IWGRP & st.st_mode) permisiuni[4]='w';
    if(S_IXGRP & st.st_mode) permisiuni[5]='x';
    if(S_IROTH & st.st_mode) permisiuni[6]='r';
    if(S_IWOTH & st.st_mode) permisiuni[7]='w';
    if(S_IXOTH & st.st_mode) permisiuni[8]='x';

    fprintf(fd, "\nPermissions: %s", permisiuni);

    fprintf(fd, "\n%s: %lld", "std_ino",st.st_ino);
    fprintf(fd, "\n%s: %lld", "std_dev",st.st_dev);
    fprintf(fd, "\n%s: %lld", "st_atim",st.st_atim);
    fprintf(fd, "\n%s: %lld", "st_ctim",st.st_ctim);
    fprintf(fd, "\n%s: %lld", "st_mtim",st.st_mtim);

    fseek(fd, 0L, SEEK_END);
    int sz = ftell(fd);
    fseek(fd, 0L, SEEK_SET);

    char temp_buffer[1000];
    fread(temp_buffer, 1, sz, fd);

    fseek(fd, 0L, SEEK_SET);
    fprintf(fd, "%d:", sz);
    fprintf(fd, "%s", temp_buffer);

    fclose(fd);
    return 1;
}

int main( )
{
    char input[100];

    while(true){
        char* s = fgets(input, sizeof(input), stdin);
        if (s == NULL)
            break;

        if (PARSE_CONDITION("quit")){
            break;
        }
        else if (PARSE_CONDITION("login:")){
            int return_val = 0;
            int pipe_d[2];

            pipe(pipe_d);
            pid_t pid = fork();
            if (pid == -1){
                error_log("Fork failed");
                return -1;
            }
            else if (pid != 0){
                wait(&return_val);

                char buff[20];
                int len_read = read(pipe_d[READ], buff, 100);
                buff[len_read] = '\0';

                char* message_start = check_correct_size(buff);
                if (!message_start){
                    error_log("Receive message not complete");
                    return -1;
                }

                if(memcmp(message_start,"not found", strlen("not found")) == 0){
                    console_log("Users db not found - login functionality not available");
                }
                else if (memcmp(message_start,"success", strlen("success")) == 0) {
                    console_log("User recognized");
                    console_log("You are now logged in");
                }
                else if (memcmp(message_start,"failed", strlen("failed")) == 0) {
                    console_log("User not found");
                }
                else {
                    error_log("Subprocess response not recognized");
                }
            }
            else {
                return login_func(input, pipe_d);
            }
        }
        else if (PARSE_CONDITION("mystat ")){
            char path[100] = "/home/first/Desktop/RiCa-One";
            if (getcwd(path, sizeof(path)) == NULL) {
                error_log("getcwd error");
                return -1;
            }

            char* fifo_name = "fisier.txt";
            strcat(path + strlen(path), "/");
            strcat(path + strlen(path), fifo_name);
            printf("%s\n",path);

            if (-1 == mkfifo(path, 0777)){
                error_log("Could not create fifo");
                perror("Descr:");
                return -1;
            }
            pid_t pid = fork();
            int return_val;

            if (pid == -1){
                error_log("Fork failed");
                return 0;
            }
            else if (pid != 0){
                wait(&return_val);

                char buffer[1000];
                FILE* fd = fopen(fifo_name, "r");
                fseek(fd, 0L, SEEK_END);
                int sz = ftell(fd);
                fseek(fd, 0L, SEEK_SET);
                fread(buffer, 1, sz, fd);

                char* message_start = check_correct_size(buffer);
                if (!message_start){
                    error_log("Receive message not complete");
                    return -1;
                }

                if (memcmp(message_start,"failed", strlen("failed")) == 0) {
                    error_log("Stat function failed");
                }
                else {
                    printf("%s", message_start);
                }

                fclose(fd);
            }
            else {
                return mystat_func(input, fifo_name);
            }
        }
    }

    return 0;
}
