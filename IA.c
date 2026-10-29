#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>


#define BUFFER_SIZE 256
#define NUM_CHILDREN 3

pid_t children[NUM_CHILDREN];

void handle_sigint(int sig){
printf("\n[Interrupt | PID %d] Signal Ctrl+C detected! Terminating program...\n", getpid());
for (int i = 0; i < NUM_CHILDREN; i++){
if (children[i] > 0){
kill(children[i], SIGTERM);
}
}

while (wait(NULL) > 0);
printf("[INTERRUPT] Exited successfully.\n");
exit(0);
}

int main() {
int pipe_fd[NUM_CHILDREN][2];
char message[BUFFER_SIZE];

if (signal(SIGINT, handle_sigint) == SIG_ERR) {
perror("Error catching signal");
exit(1);
}

for (int i = 0; i < NUM_CHILDREN; i++){
if (pipe(pipe_fd[i]) == -1) {
perror("Pipe creation failed");
exit(1);
}
}

for (int i = 0; i < NUM_CHILDREN; i++){
children[i] = fork();

if (children[i] < 0) {
perror("Fork failed");
exit(1);
}

if (children[i] == 0) {
int child_num = i + 1;
for (int j = 0; j < NUM_CHILDREN; j++){
close(pipe_fd[j][1]);
if (j != i){
close(pipe_fd[j][0]);
}
}

char read_buf[BUFFER_SIZE];
int bytes_read;

while ((bytes_read = read(pipe_fd[i][0], read_buf, sizeof(read_buf) - 1)) > 0){
read_buf[bytes_read] = '\0';
read_buf[strcspn(read_buf, "\r\n")] =0;
printf("[Child %d | PID: %d] Received: \"%s\"\n", child_num, getpid(), read_buf);
}
printf("[Child %d | PID: %d] pipe closed, exiting.\n", child_num, getpid());
close(pipe_fd[i][0]);
exit(0);
}
}

for (int i = 0; i < NUM_CHILDREN; i++){
close(pipe_fd[i][0]);
}

printf("[PARENT | PID %d] Created %d child processes\n", getpid(), NUM_CHILDREN);
printf("Type a message and press Enter to send it to children.\n");
printf("Type [exit] to exit or press Ctrl+C to interrupt.\n\n");

while (1){
printf("Enter message: ");
fflush(stdout);

if (fgets(message, sizeof(message), stdin) == NULL){
break;
}

if (strncmp(message, "exit", 4) == 0 && (message[4] == '\n' || message[4] == '\0')){
break;
}

for (int i = 0; i < NUM_CHILDREN; i++){
write(pipe_fd[i][1], message, strlen(message));
}

usleep(100000);
}

for (int i = 0; i < NUM_CHILDREN; i++){
close (pipe_fd[i][1]);
}

for (int i = 0; i < NUM_CHILDREN; i++){
wait(NULL);
}

printf("[PARENT | PID %d] All children terminated. Program ended\n", getpid());
return 0;
}
