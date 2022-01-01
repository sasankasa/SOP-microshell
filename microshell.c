#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAG "\e[1;35m"
#define CYN "\e[1;36m"
#define WHT "\e[0;38m"
#define BGRN "\e[1;32m"
#define GRN "\e[0;32m"
#define BBRW "\e[1;33m"
#define BHGRN "\e[1;92m"

const size_t MAX_CMD_LEN = 255;
struct sleeping_job {
    bool occupied;
    pid_t pid;
    char *cmd;
};
const size_t MAX_JOBS_COUNT = 10;
struct sleeping_job *jobs;

void my_exec(char **argv){
    int r = execvp(argv[0], argv);
    if (r == -1) {
        if (errno == ENOENT) {
            fprintf(stderr, "%s: command not found\n", argv[0]);
        } else {
            perror("execvp()");
        }
        exit(-1);
    }
}

int wait_for_child(pid_t p, char* cmd) {
    while (true) {
        int wstatus;
        pid_t result = waitpid(p, &wstatus, WUNTRACED);
        if(result == -1){
            if (errno != EINTR) {
                return -1;
            }
        } else {
            if (WIFSTOPPED(wstatus)) {
                for (size_t i = 0; i < MAX_JOBS_COUNT; i++) {
                    if (jobs[i].occupied) {
                        continue;
                    }
                    jobs[i].occupied = true;
                    jobs[i].pid = p;
                    jobs[i].cmd = malloc(strlen(cmd) + 1);
                    strcpy(jobs[i].cmd, cmd);
                    printf("[%zu] Stopped %s\n", i, jobs[i].cmd);
                    break;
                }
            }
            break;
        }
    }
    return 0;
}

int remove_directory(char *path){
    DIR *d = opendir(path);
    if(d==NULL){
        perror("opendir()");
    }
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL){
        if(dir->d_type != DT_DIR){
            size_t buf_len = strlen(path) + strlen(dir->d_name) + 1;
            char f_path[buf_len];
            sprintf(f_path, "%s/%s", path, dir->d_name);
            if (remove(f_path) ==-1){
                perror("remove()");
            } else {
                printf("'%s' was deleted successfully\n", f_path);
            }
        } else if(dir->d_type == DT_DIR && strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0 ){
            size_t buf_len = strlen(path) + strlen(dir->d_name) + 1;
            char f_path[buf_len];
            sprintf(f_path, "%s/%s", path, dir->d_name);
            if (remove(f_path)==-1){
                if (remove_directory(f_path)==-1){
                    perror("remove()");
                } else {
                    printf("'%s' was deleted successfully\n", f_path);
                }
            } else {
                printf("'%s' was deleted successfully\n", f_path);
            }

        }
    }
    if(remove(path)==0){
        printf("'%s' was deleted successfully\n", path);
    } else {
        perror("remove()");
        return -1;
    }
    closedir(d);
    return 0;
}

int is_directory(const char *path) {
    struct stat stat_buf;
    if (stat(path, &stat_buf) != 0){
        return 0;
    }
    return S_ISDIR(stat_buf.st_mode);
}

void parse_args(char *input, char **argv, size_t max_args) {
    size_t i=0;
    size_t j=0;
    size_t k=0;
    while(i < max_args - 1 && input[j] != '\0'){
        if(input[j]==' '){
            if ((j-k) != 0) {
                strncpy(argv[i], input + k, j - k);
                argv[i][j - k] = '\0';
                i++;
            }
            k = j+1;
        }
        j++;
    }
    if (j!=k && input[j]=='\0'){
        strcpy(argv[i], input+k);
        i++;
    }
    argv[i]=NULL;
}

void tail(char *path, int lines_count){
    size_t MAX_LINE_LEN = 1000;
    size_t MAX_LINES = 1000;
    char tail[MAX_LINES][MAX_LINE_LEN];
    FILE *file;
    file = fopen(path, "r");
    if (file != NULL){
        int i=1;
        while(fgets(tail[i],MAX_LINE_LEN, file)!=NULL){
            i++;
        }
        int k=i-lines_count;
        while(k!=i){
            printf("%s", tail[k]);
            k++;
        }
        fclose(file);
    } else {
        perror("tail()");
    }
}

void prompt() {
    char cwd[PATH_MAX];
    char host[30];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd()");
        exit(-1);
    }
    if (gethostname(host, sizeof(host)) != 0) {
        perror("gethostbynampe()");
        exit(-1);
    }
    printf("%s%s@%s%s:%s[%s] %s$ ", CYN, getenv("USER"), host, WHT, MAG, cwd, WHT);
}

void help(){
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("          %smicroshell.c%s\n", BGRN, WHT);
    printf("       by %sOliwia Michalik%s\n", BHGRN, WHT);
    printf("     functionalities to use:\n");
    printf("       %scd, help, exit, pwd\n", BBRW);
    printf("    isdir, rm, whatis, 'pipe'%s\n", WHT);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

void execute_command(char *input){
    size_t MAX_ARGS = 10;
    char my_argv[MAX_ARGS][MAX_CMD_LEN];
    char *argv[MAX_ARGS];
    for (size_t i = 0; i < MAX_ARGS; i++) {
        argv[i] = my_argv[i];
    }
    parse_args(input, argv, MAX_ARGS);
    char *command = argv[0];
    int argc = 0;
    for (; argv[argc] != NULL; argc++);
    if(command == NULL){
        return;
    } else if (0 == strcmp(command, "exit")) {
        printf("exiting...\n");
        exit(0);
    } else if (0 == strcmp(command, "fg")) {
        if (argc < 2) {
            printf("fg: argument needed\n");
            return;
        }
        int id = atoi(argv[1]);
        if (id < 0 || id > (int) MAX_JOBS_COUNT || !jobs[id].occupied) {
            printf("fg: unknown job %d\n", id);
            return;
        }
        char cmd[MAX_CMD_LEN];
        strcpy(cmd, jobs[id].cmd);
        printf("%s\n", jobs[id].cmd);
        free(jobs[id].cmd);
        jobs[id].occupied = false;
        if (kill(jobs[id].pid, SIGCONT) != 0) {
            perror("kill()");
            return;
        }
        if (wait_for_child(jobs[id].pid, cmd) != 0) {
            perror("wait_for_child()");
            return;
        }
    } else if (0 == strcmp(command, "cd")){
        char *to;
        if(argc < 2){
            to = getenv("HOME");
        } else {
            to = argv[1];
        }
        if(chdir(to)==-1){
            perror("cd");
        }
    } else if (0==strcmp(command, "help")){
        help();
    } else if(0==strcmp(command, "pwd")){
        char cwd[PATH_MAX];
        getcwd(cwd, sizeof(cwd));
        printf("%s%s%s\n", GRN, cwd, WHT);
    } else if (0==strcmp(command, "tail")){
        if(argc>=2){
            char *path=argv[argc-1];
            int lines_count;
            if(argc==2){
                lines_count=10;
            } else if (argc==3){
                lines_count=(atoi(argv[1]));
                if(lines_count<0){
                    lines_count=-lines_count;
                }
            } else {
                lines_count=atoi(argv[2]);
            }
            tail(path, lines_count);
        } else {
            perror("tail");
        }
    } else if (0==strcmp(command, "rm")){
        if(strcmp(argv[1], "-r")!=0){
            int i=1;
            while (i<argc){
                if(is_directory(argv[i])==0){
                    printf("removing '%s'...\n", argv[i]);
                    if(remove(argv[i])==0){
                        printf("'%s' was deleted successfully\n", argv[i]);
                    } else {
                        perror("rm");
                    }
                } else {
                    fprintf(stderr, "rm: cannot remove '%s': is a directory\n", argv[i]);
                }
                i++;
            }
        } else {
            int k=2;
            while(k<argc){
                printf("removing '%s'\n", argv[k]);
                remove_directory(argv[k]);
                k++;
            }
        }
    } else if (0==strcmp(command, "isdir")) {
        if (is_directory(argv[1])!= 0) {
            printf("'%s' is directory\n", argv[1]);
        } else
            printf("'%s' is NOT directory\n", argv[1]);
    } else {
        pid_t p = fork();
        if (p == -1) {
            perror("fork()");
            exit(-1);
        }
        if (p == 0) {
            int pipe_pos = -1;
            int pipefd[2] = {-1, -1};
            int next_stdout = 1;
            int next_stdin = 0;
            while (true) {
                for (int i = argc - 1; i >= 0; i--) {
                    if (strcmp(argv[i], "|") == 0) {
                        pipe_pos = i;
                        break;
                    }
                }
                argc = pipe_pos;
                argv[argc] = NULL;
                if (pipe_pos == -1) {
                    next_stdin = 0;
                    break;
                }
                if (pipe(pipefd) == -1) {
                    perror("pipe()");
                    exit(-1);
                }
                pid_t p = fork();
                if (p == -1) {
                    perror("fork()");
                    exit(-1);
                }
                if (p != 0) {
                    next_stdin = pipefd[0];
                    break;
                } else {
                    next_stdout = pipefd[1];
                    pipe_pos = -1;
                }
            }
            int arg_start = (pipe_pos != -1) ? pipe_pos + 1 : 0;
            if (next_stdin != 0) {
                if (dup2(next_stdin, STDIN_FILENO) == -1) {
                    perror("dup2()");
                    exit(-1);
                }
            }
            if (next_stdout != 1) {
                if (dup2(next_stdout, STDOUT_FILENO) == -1) {
                    perror("dup2()");
                    exit(-1);
                }
            }
            if (pipefd[0] != -1) {
                close(pipefd[0]);
            }
            if (pipefd[1] != -1) {
                close(pipefd[1]);
            }

            my_exec(argv + arg_start);
        } else {
            int last_pipe = -1;
            for (int i = 0; input[i] != '\0'; i++) {
                if (input[i] == '|') {
                    last_pipe = i;
                }
                if (input[i] == ' ' && last_pipe == i-1) {
                    last_pipe = i;
                }
            }
            char *cmd = input + last_pipe + 1;
            if (wait_for_child(p, cmd) != 0) {
                perror("wait_for_child()");
                exit(-1);
            }
        }
    }
}

int read_input(char *input, int buf_size) {
    if (fgets(input, buf_size, stdin) == NULL) {
        return -1;
    }
    size_t len = strlen(input);
    if (input[len - 1] != '\n') {
        errno = E2BIG;
        return -1;
    }
    input[len - 1] = '\0';
    return 0;
}

void sigint_handler(int sig, siginfo_t *info, void *ucontext) {}
void sigtstp_handler(int sig, siginfo_t *info, void *ucontext) {}

int main() {
    jobs = calloc(sizeof(struct sleeping_job), MAX_JOBS_COUNT);
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = sigint_handler;
    if (sigaction(SIGINT, &sa, NULL) != 0) {
        perror("sigaction()");
        exit(-1);
    }

    sa.sa_sigaction = sigtstp_handler;
    if (sigaction(SIGTSTP, &sa, NULL) != 0) {
        perror("sigaction()");
        exit(-1);
    }

    while (true) {
        char input[MAX_CMD_LEN];
        prompt();
        if (read_input(&input, MAX_CMD_LEN) != 0) {
            if (feof(stdin)) {
                printf("\n");
                exit(0);
            }
            if (errno == EINTR) {
                printf("\n");
                continue;
            }
            perror("read_input()");
            exit(-1);
        }
        execute_command(input);
    }
    return 0;
}