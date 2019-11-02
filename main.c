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
#include <dirent.h>
#include <time.h>
#include <sys/socket.h>

# define PARSE_CONDITION(c) memcmp(input, c , strlen(c)) == 0
#define READ 0
#define WRITE 1

void console_log(const char* str) {
	printf(">>>%s\n", str);
}
void error_log(const char* str) {
	printf("ERROR: %s\n", str);
}


int login_func(char input[100], int pipe[2]) {
	FILE* fd = fopen("../users.txt", "r");
	if (fd == NULL) {
		close(pipe[READ]);
		write(pipe[WRITE], "9:not found", strlen("9:not found"));
		return -1;
	}

	char line[100];
	bool found = false;
	while (fgets(line, sizeof(line), fd)) {
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';
		if (strcmp(input + strlen("login:"), line) == 0 || strcmp(input + strlen("login:"), line - 1) == 0) {
			close(pipe[READ]);
			write(pipe[WRITE], "7:success", strlen("7:success"));

			found = true;
			break;
		}
	}
	fclose(fd);

	if (!found) {
		close(pipe[READ]);
		write(pipe[WRITE], "6:failed", strlen("6:failed"));
		return 0;
	}

	return 1;
}

struct get_size {
	int len;
	char* newStart;
};

struct get_size get_message_size(char* msg) {
	struct get_size sc;

	char tmp[10];
	strncpy(tmp, msg, 10);
	char* st = strtok(tmp, ":");
	char* end;
	sc.len = strtol(st, &end, 10);
	sc.newStart = msg + strlen(st) + 1;

	return sc;
}

int child_info_func(char* input, const char* fifo_name) {
	struct stat st;
	char permisiuni[10] = "---------";

	int file = open(fifo_name, O_WRONLY);

	char buffer[1000];

	if (0 != stat(input, &st))
	{
		write(file, "6:failed", strlen("6:failed"));
		close(file);
		return -1;
	}

	sprintf(buffer, "UID: %ld", (long)st.st_uid);
	sprintf(buffer + strlen(buffer), "\nGID: %ld", (long)st.st_gid);

	sprintf(buffer + strlen(buffer), "\nType: ");
	switch (S_IFMT & st.st_mode)
	{
	case S_IFDIR: sprintf(buffer + strlen(buffer), "Director"); break;
	case S_IFREG: sprintf(buffer + strlen(buffer), "File"); break;
	case S_IFLNK: sprintf(buffer + strlen(buffer), "Link"); break;
	case S_IFIFO: sprintf(buffer + strlen(buffer), "Fifo"); break;
	case S_IFSOCK: sprintf(buffer + strlen(buffer), "Socket"); break;
	default: sprintf(buffer + strlen(buffer), "Unknown file type");
	}

	sprintf(buffer + strlen(buffer), "\nSize: %lld kb", (long long)st.st_size / 1024);

	if (S_IRUSR & st.st_mode) permisiuni[0] = 'r';
	if (S_IWUSR & st.st_mode) permisiuni[1] = 'w';
	if (S_IXUSR & st.st_mode) permisiuni[2] = 'x';
	if (S_IRGRP & st.st_mode) permisiuni[3] = 'r';
	if (S_IWGRP & st.st_mode) permisiuni[4] = 'w';
	if (S_IXGRP & st.st_mode) permisiuni[5] = 'x';
	if (S_IROTH & st.st_mode) permisiuni[6] = 'r';
	if (S_IWOTH & st.st_mode) permisiuni[7] = 'w';
	if (S_IXOTH & st.st_mode) permisiuni[8] = 'x';

	sprintf(buffer + strlen(buffer), "\nPermissions: %s", permisiuni);

	sprintf(buffer + strlen(buffer), "\n%s: %lld", "std_ino", st.st_ino);
	sprintf(buffer + strlen(buffer), "\n%s: %lld", "std_dev", st.st_dev);
	sprintf(buffer + strlen(buffer), "\n%s: %lld", "st_atim", st.st_atim);
	sprintf(buffer + strlen(buffer), "\n%s: %lld", "st_ctim", st.st_ctim);
	sprintf(buffer + strlen(buffer), "\n%s: %lld", "st_mtim", st.st_mtim);

	struct tm dt;

	dt = *(gmtime(&st.st_ctime));
	//sprintf(buffer + strlen(buffer),"\nCreated on: %d-%d-%d %d:%d:%d", dt.tm_mday, dt.tm_mon, dt.tm_year + 1900,dt.tm_hour, dt.tm_min, dt.tm_sec);

	dt = *(gmtime(&st.st_mtime));
	//sprintf(buffer + strlen(buffer),"\nModified on: %d-%d-%d %d:%d:%d", dt.tm_mday, dt.tm_mon, dt.tm_year + 1900,dt.tm_hour, dt.tm_min, dt.tm_sec);

	int sz = strlen(buffer);

	char number[10];
	memset(number, '\0', 10);
	sprintf(number, "%d", sz);

	write(file, number, strlen(number));
	write(file, ":", 1);
	write(file, buffer, sz);

	close(file);
	return 1;
}

bool is_dir(char* path) {
	struct stat st;

	if (0 != stat(path, &st))
	{
		return -1;
	}

	return (S_IFMT & st.st_mode) == S_IFDIR;
}

int get_info_func(char* input) {
	if (memcmp(input, "mystat", 6) != 0) {
		char tmp[PATH_MAX];
		sprintf(tmp, "%s", input);
		sprintf(input, "mystat %s", tmp);
	}

	//printf("Stat function called with arg %s\n",input);

	char path[100];
	if (getcwd(path, sizeof(path)) == NULL) {
		error_log("getcwd error");
		return -1;
	}

	char* fifo_name = "fisier.txt";
	strcat(path + strlen(path), "/");
	strcat(path + strlen(path), fifo_name);
	remove(path);

	if (-1 == mkfifo(path, 0777)) {
		error_log("Could not create fifo");
		perror("Descr:");
		return -1;
	}
	pid_t pid = fork();

	if (pid == -1) {
		error_log("Fork failed");
		return 0;
	}
	else if (pid != 0) {
		sleep(5);
		int file = open(fifo_name, O_RDONLY);

		char buffer[1000];
		memset(buffer, '\0', 11);

		read(file, buffer, 1000);

		struct get_size size = get_message_size(buffer);
		if (strlen(size.newStart) != size.len) {
			error_log("Receive message not complete");
			printf("%s len %d\n", size.newStart, strlen(size.newStart));
			printf("%d", size.len);
			close(file);
			return -1;
		}

		if (memcmp(size.newStart, "failed", strlen("failed")) == 0) {
			error_log("Stat function failed. Sanitisation of paths is required in some cases.");
		}
		else {
			printf("%s\n", size.newStart);
		}

		char c;
		while (0 != read(file, &c, 1));

		close(file);

		return 1;
	}
	else {
		return child_info_func(input + strlen("mystat "), path);
	}
}

int search_in_dir(char *path, char* goal)
{
	DIR *directory;
	struct dirent *dir_entry;
	struct stat st;
	char nume[PATH_MAX];
	int dir;
	int last_search_res = 0;

	dir = is_dir(path);

	if (dir != 1)
	{
		int len = strlen(goal + strlen("mystat "));
		if (strcmp(path + strlen(path) - len, goal + strlen("mystat ")) == 0 && path[strlen(path) - len - 1] == '/')
		{
			int res = get_info_func(path);
			if (res != 1)
				return -1;
			return 1;
		}
	}
	else
	{
		if (NULL == (directory = opendir(path)))
		{
			error_log("Unable to open directory");
			return -1;
		}

		while (NULL != (dir_entry = readdir(directory)) && last_search_res == 0)
		{
			if ((strcmp(dir_entry->d_name, ".") != 0) && (strcmp(dir_entry->d_name, "..") != 0))
			{
				sprintf(nume, "%s/%s", path, dir_entry->d_name);
				last_search_res = search_in_dir(nume, goal);
			}
		}

		closedir(directory);
	}

	return last_search_res;
}

int call_find_func(char* input) {
	int sockets[2], child;
	char buf[1024];

	socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	child = fork();

	if (child) { // parent
		close(sockets[1]);

		int res;

		write(sockets[0], input, strlen(input));

		read(sockets[0], &res, 4);

		close(sockets[0]);

		if (res == 0)
			console_log("File not found");
		if (res < 0)
			return res;
		return 1;


	}
	else { // child
		close(sockets[0]);

		read(sockets[1], buf, 1024);

		int res = search_in_dir("..", input);

		write(sockets[1], &res, 4);

		close(sockets[1]);
	}
	return 0;
}

int main(int argc, char*argv[])
{
	char input[100];

	while (true) {
		char* s = fgets(input, sizeof(input), stdin);
		input[strlen(input) - 1] = '\0';
		if (s == NULL)
			break;

		if (PARSE_CONDITION("quit")) {
			break;
		}
		else if (PARSE_CONDITION("login:")) {
			int return_val = 0;
			int pipe_d[2];

			pipe(pipe_d);
			pid_t pid = fork();
			if (pid == -1) {
				error_log("Fork failed");
				return -1;
			}
			else if (pid != 0) {
				wait(&return_val);

				char buff[20];
				int len_read = read(pipe_d[READ], buff, 100);
				buff[len_read] = '\0';

				struct get_size size = get_message_size(buff);
				if (strlen(size.newStart) != size.len) {
					error_log("Receive message not complete");
					return -1;
				}

				if (memcmp(size.newStart, "not found", strlen("not found")) == 0) {
					console_log("Users db not found - login functionality not available");
				}
				else if (memcmp(size.newStart, "success", strlen("success")) == 0) {
					console_log("User recognized");
					console_log("You are now logged in");
				}
				else if (memcmp(size.newStart, "failed", strlen("failed")) == 0) {
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
		else if (PARSE_CONDITION("mystat ")) {
			int res = get_info_func(input);
			if (res <= 0)
				return res;
			execl(argv[0], argv[0], "2", NULL);
		}
		else if (PARSE_CONDITION("cwd")) {
			char path[100];
			if (getcwd(path, sizeof(path)) == NULL) {
				error_log("getcwd error");
				return -1;
			}
			printf("%s\n", path);
		}
		else if (PARSE_CONDITION("myfind ")) {
			int res = call_find_func(input);
			if (res < 0)
				return res;
		}
		else {
			printf("Command not recognized\n");
		}
	}

	return 0;
}
