#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void sigterm(int fd) {
	puts("SIGTERM SENT");
	exit(1);
}

int main() {
	signal(SIGTERM,sigterm);
	while(1) {
		usleep(100000);
	}

}
