#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("argument missing\n");
		exit(-1);
	}
	int n = atoi(argv[1]);
	for (int i = 0; i < n; i++) {
		printf("sleeping %d...\n", i);
		fflush(stdout);
		sleep(1);
	}
	return 0;
}
