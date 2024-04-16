#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

/**
 * This program implements prog1 && prog2 | prog3 > file
 * exits with code 0 in case of success, even if prog1 returns with non-zero value
 * exits with code 0 in case of failure at any stage
 * 
*/

int main(int argc, char* argv[]) {
    if(argc != 5) {
        fprintf(stderr, "usage: %s prog1 prog2 prog3 file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* prog1 = argv[1];
    char* prog2 = argv[2];
    char* prog3 = argv[3];
    char* file = argv[4];

    int status1, status2;
    int fork_res = fork();
    if(fork_res == -1) {
        perror("fork 1");
        exit(EXIT_FAILURE);
    }
    if(fork_res == 0) {
        // we use exelp from exec* family to be able to use $PATH environment variable
        execlp(prog1, prog1, NULL);
        // if code after exec* executes, that means exec didn't work and error should be returned
        perror("exec prog1");
        exit(EXIT_FAILURE);
    }
    // wait for prog1 to finish and store its exit value in status variable
    wait(&status1);

    // if prog1 exited with non-zero status, program stops with exit code 0
    if(status1 != 0) {
        exit(WEXITSTATUS(status1));
    }

    int pipe_fds[2];
    if(pipe(pipe_fds) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    int pipe_read = pipe_fds[STDIN_FILENO];
    int pipe_write = pipe_fds[STDOUT_FILENO];

    // open file, create if it does not exist and truncate if it does.
    // if the file is created, it is readable, writeable and executable
    // by everyone
    int fd = open(file, O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if(fd < 0) {
        perror("file");
        exit(EXIT_FAILURE);
    }

    fork_res = fork();
    if(fork_res == -1) {
        perror("fork 2");
        exit(EXIT_FAILURE);
    }
    if(fork_res == 0) {
        close(pipe_read);
        close(STDOUT_FILENO);
        // redirect standard output of prog2 to the pipe's write end
        dup2(pipe_write, STDOUT_FILENO);
        close(pipe_write);
        
        execlp(prog2, prog2, NULL);
        perror("exec prog2");
        exit(EXIT_FAILURE);
    }

    fork_res = fork();
    if(fork_res == 0) {
        close(pipe_write);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        // redirect standard input of the prog3 from the pipe's read end
        dup2(pipe_read, STDIN_FILENO);
        // redirect standart output of the prog3 to the file
        dup2(fd, STDOUT_FILENO);
        close(pipe_read);
        close(fd);

        execlp(prog3, prog3, NULL);
        perror("exec prog3");
        exit(EXIT_FAILURE);
    }

    close(pipe_read);
    close(pipe_write);

    wait(&status1);
    wait(&status2);

    exit(WEXITSTATUS(status1) | WEXITSTATUS(status2));
}
