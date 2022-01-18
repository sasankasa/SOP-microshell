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

// maksymalna długość polecenia
const size_t MAX_CMD_LEN = 255;

// wykonuje program i sprawdza błąd
void my_exec(char **argv){
    int r = execvp(argv[0], argv); // zastępuje aktualny proces programem znajdującym się w argv[0]
    if (r == -1) {
        if (errno == ENOENT) {
            fprintf(stderr, "%s: command not found\n", argv[0]);
        } else {
            perror("execvp()");
        }
        exit(-1);
    }
}
// usuń katalog i jego wnętrze
int remove_directory(char *path){
    DIR *d = opendir(path); // otwieram katalog
    if(d==NULL){
        perror("opendir()");
        return -1;
    }
    struct dirent *dir; // struktura zawierająca informacje o elemencie katalogu
    // pętla po wszystkich elementach katalogu
    while ((dir = readdir(d)) != NULL){
        if(dir->d_type != DT_DIR){
            // jeśli to nie katalog
            // obliczam długość połączonej ścieżki
            size_t buf_len = strlen(path) + 1 + strlen(dir->d_name) + 1;
            char f_path[buf_len];
            //łączy nazwy katalogu i pliku
            sprintf(f_path, "%s/%s", path, dir->d_name);
            if (remove(f_path) ==-1){
                perror("remove()");
                return -1;
            } else {
                printf("'%s' was deleted successfully\n", f_path);
            }
        } else if(dir->d_type == DT_DIR && strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0 ) {
            //jeśli to jest katalog, ale nie jest to '.' ani '..'
            size_t buf_len = strlen(path) + 1 + strlen(dir->d_name) + 1;
            char f_path[buf_len];
            //łączy nazwy katalogów
            sprintf(f_path, "%s/%s", path, dir->d_name);
            if (remove_directory(f_path) == -1) {
                perror("remove()");
                return -1;
            } else {
                printf("'%s' was deleted successfully\n", f_path);
            }
        }
    }
    // katalog path jest pusty, więc mogę go usunąć
    if(remove(path)==0){
        printf("'%s' was deleted successfully\n", path);
    } else {
        perror("remove()");
        return -1;
    }
    closedir(d);
    return 0;
}

//sprawdzam czy path jest katalogiem czy nie
int is_directory(const char *path) {
    struct stat stat_buf; //struktura zawierająca informacje o pliku
    //pobierz informacje o pliku
    if (stat(path, &stat_buf) != 0){
        return 0;
    }
    // zwraca true jeśli st_mode jest katalogiem
    return S_ISDIR(stat_buf.st_mode);
}

// dzieli input na listę argumentów oddzielonych spacjami
int parse_args(char *input, char **argv, size_t max_args) {
    size_t i=0; // który argument wczytujemy
    size_t j=0; // pozycja w impucie
    size_t k=0; // początek aktualnie wczytywanego argumentu
    // dopóki mamy miejsce w tablicy argumentów i nie wczytaliśmy całego inputu
    while(i < max_args - 2 && input[j] != '\0'){
        if(input[j]==' '){ // jak aktualnie wczytywany argument się skończył
            if ((j-k) != 0) { // jeśli poprzedni znak to nie spacja
                // kopiujemy aktualny argument
                strncpy(argv[i], input + k, j - k);
                // strncpy nie dodaje '\0', więc robimy to sami
                argv[i][j - k] = '\0';
                i++;
            }
            k = j+1;
        }
        j++;
    }
    // jeśli input ma za dużo argumentów
    if (input[j] != '\0') {
        errno = E2BIG;
        return -1;
    }
    // kopiujemy ostatni argument
    if (j!=k && input[j]=='\0'){
        strcpy(argv[i], input+k);
        i++;
    }
    argv[i]=NULL; // koniec argumentów
    return 0;
}

// wyświetla lines_count ostatnich linijek w path
void tail(char *path, int lines_count) {
    size_t MAX_LINE_LEN = 1000; // maksymalna długość linijki
    size_t MAX_LINES = 1000; // maksymalna ilość linijek
    if (MAX_LINES < lines_count) {
        printf("requested too many lines, max is %zu\n", MAX_LINES);
        return;
    }
    char tail[MAX_LINES][MAX_LINE_LEN];
    FILE *file = fopen(path, "r"); // otwieram plik do odczytu
    if (file == NULL) {
        perror("fopen()");
        return;
    }
    int i = 0;
    while (fgets(tail[i % MAX_LINES], MAX_LINE_LEN, file) != NULL) {
        // dopóki plik nam się nie skończy, pobieraj całą linijkę do tail
        i++;
    }
    fclose(file);
    // wypisz ostatnie lines_count linijek, zaczynajac od (i-lines_count) % MAX_LINES
    for (int j = 0; j < lines_count; j++) {
        printf("%s", tail[(i - lines_count + j) % MAX_LINES]);
    }
}

// wypisuje znak zachęty
void prompt() {
    char cwd[PATH_MAX];
    char host[30];
    // pobieramy aktualny katalog roboczy
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd()");
        exit(-1);
    }
    // pobieramy nazwę hosta
    if (gethostname(host, sizeof(host)) != 0) {
        perror("gethostname()");
        exit(-1);
    }
    // wyświetlamy nazwę użytkowanika, hosta i CWD
    printf("%s%s@%s%s:%s[%s] %s$ ", CYN, getenv("USER"), host, WHT, MAG, cwd, WHT);
}

void help(){
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("          %smicroshell.c%s\n", BGRN, WHT);
    printf("       by %sOliwia Michalik%s\n", BHGRN, WHT);
    printf("     functionalities to use:\n");
    printf("       %scd, help, exit, pwd\n", BBRW);
    printf("        isdir, rm, 'pipe'%s\n", WHT);
    printf("         also supported:\n");
    printf("   * signals (SIGINT, SIGTSTP)\n");
    printf("            * pipes\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

// wykonujemy komendy z imputa
void execute_command(char *input){
    size_t MAX_ARGS = 10;
    char my_argv[MAX_ARGS][MAX_CMD_LEN];
    char *argv[MAX_ARGS];
    // z tablicy tablic robimy tablicę wskaźników
    for (size_t i = 0; i < MAX_ARGS; i++) {
        argv[i] = my_argv[i];
    }
    if (parse_args(input, argv, MAX_ARGS) == -1) {
        perror("parse_args()");
        return;
    }
    // pierwszy argument w tablicy argv to nazwa polecenia
    char *command = argv[0];
    int argc = 0;
    // zlicza liczbę argumentów w argv
    for (; argv[argc] != NULL; argc++);
    if(command == NULL){
        //jeśli imput to enter lub same spacje
        return;
    } else if (0 == strcmp(command, "exit")) {
        // komenda wyjścia z programu
        printf("exiting...\n");
        exit(0);
    } else if (0 == strcmp(command, "cd")){
        char *to;
        if(argc < 2){
            // jeśli nie mamy żadnych argumnetów do cd -> idź do katalogu domowego
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
        if (getcwd(cwd, sizeof(cwd)) == NULL){
            perror("getcwd()");
            return;
        }
        printf("%s%s%s\n", GRN, cwd, WHT);
    } else if (0==strcmp(command, "tail")) {
        if (argc < 2) {
            printf("tail: missing argument\n");
            return;
        }
        // jeśli mamy argument do komendy
        char *path = argv[argc - 1];
        int lines_count;
        if (argc == 2) {
            lines_count = 10;
        } else if (argc == 3) {
            lines_count = -atoi(argv[1]); // zmień char* "-n" na int n
        } else {
            lines_count = atoi(argv[2]);
        }
        tail(path, lines_count);
    } else if (0==strcmp(command, "rm")){
        if(strcmp(argv[1], "-r")!=0){
            // jeśli usuwamy nierekursywnie
            for (int i=1; i<argc; i++) {
                if(is_directory(argv[i])==0){
                    printf("removing '%s'...\n", argv[i]);
                    if(remove(argv[i])!=0){
                        perror("remove()");
                    } else {
                        printf("'%s' was deleted successfully\n", argv[i]);
                    }
                } else {
                    fprintf(stderr, "rm: cannot remove '%s': is a directory\n", argv[i]);
                }
            }
        } else {
            // jeśli usuwamy rekursywnie
            for (int k=2; k<argc; k++){
                if(!is_directory(argv[k])){
                    if (remove(argv[k])!=0){
                        perror("remove()");
                    } else {
                        printf("'%s' was deleted successfully\n", argv[k]);
                    }
                } else {
                    printf("removing '%s'\n", argv[k]);
                    if (remove_directory(argv[k])!=0){
                        perror("remove_directory()");
                        return;
                    }
                }
            }
        }
    } else if (0==strcmp(command, "isdir")) {
        // sprawdzam czy argument jest katalogiem
        if (is_directory(argv[1])!= 0) {
            printf("'%s' is directory\n", argv[1]);
        } else
            printf("'%s' is NOT directory\n", argv[1]);
    } else {
        pid_t p = fork();
        if(p==-1){
            perror("fork()\n");
        } else if (p == 0){
            int r = execvp(command, argv);
            if (r == -1){
                if (errno == ENOENT) {
                    fprintf(stderr, "%s: command not found\n", argv[0]);
                } else {
                    perror("execvp() error");
                }
            }
            exit(0);
        } else {
            pid_t result = waitpid(p, NULL, 0);
            if(result == -1){
                perror("waitpid() error");
            }
        }
    }
}

int read_input(char *input, int buf_size) {
    // pobiera input z klawiatury
    if (fgets(input, buf_size, stdin) == NULL) {
        return -1;
    }
    size_t len = strlen(input); // oblicza długość inputu
    if (input[len - 1] != '\n') {
        errno = E2BIG;
        return -1;
    }
    input[len - 1] = '\0'; // ustawia ostatni znak inputu jako '\0'
    return 0;
}

int main() {
    while (true) {
        char input[MAX_CMD_LEN];
        prompt();
        if (read_input(&input, MAX_CMD_LEN) != 0) {
            perror("read_input()");
            exit(-1);
        }
        execute_command(input);
    }
    return 0;
}