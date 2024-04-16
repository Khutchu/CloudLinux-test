#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <linux/limits.h>

#define MAX_DIR_SIZE 512   /* maximum number of files in a directory */
#define MAX_PATHS_NUM 16    /* maximum number of paths user can specify */
#define INDENT_SIZE 4       /* size of indentation that's used  */

#define min(a, b) (((a) < (b)) ? (a) : (b))


const char* default_dir = ".";

int ls(const char* dir_path);
static int ls_helper(char* dir_path, int depth);
int read_entries(DIR* dir_ptr, struct dirent* entries[]);
int entry_cmp(const void* a, const void* b);
void print_with_indent(const char* str, int depth);
static int traverse(char* dir_path, struct dirent* entries[], int n_dir_entries, int depth);


/**
 * The program prints the contents of the directory and its subdirectories recursively.
 * If no arguments are specified, it prints contents of the current working directory.
 * User can specify which directory's contents to see by providing the paths as
 * arguments.
 * The program exits with EXIT_SUCCESS if every path was traversed and printed succesfully
 * and EXIT_FAILURE if any of them was not.
*/
int main(int argc, char* argv[]) {
    int exit_val = EXIT_SUCCESS;

    if (argc < 2) {
        exit_val = ls(default_dir);
    } else {
        for(int i = 1; i < argc; i++) {
            exit_val |= ls(argv[i]);
            printf("\n");
        }
    }
    exit(exit_val);
}


int ls(const char* dir_path) {
    // make sure the path exists
    if (access(dir_path, F_OK) != 0) {
        printf("%s: No such file or directory\n", dir_path);
    	return EXIT_FAILURE;
    }

    // acquire the absolute path from relative (or already absolute path)
    // and store it in buffer with length equal to maximal length of path
    // in Linux
    char* abs_path = (char*) malloc(PATH_MAX);
    if (abs_path == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }
    if (realpath(dir_path, abs_path) == NULL) {
        perror("realpath");
        free(abs_path);
        return EXIT_FAILURE;
    }

    printf("%s\n", abs_path);
    int res = ls_helper(abs_path, 1);
    free(abs_path);
    return res;
}

/**
 * Recursive function that is used by ls.
 * It takes absolute path and indentation depth
 * as arguments and prints contents if the path refers
 * to the directory, nothing otherwise.
*/
static int ls_helper(char* dir_path, int depth) {
    struct stat dir_path_stat;
    if (stat(dir_path, &dir_path_stat) != 0) {
        perror("stat");
        return EXIT_FAILURE;
    }
    // if path refers to file nothing happens and EXIT_SUCCESS is returned
    if (! S_ISDIR(dir_path_stat.st_mode)) {
        return EXIT_SUCCESS;
    }

    DIR* dir_ptr = opendir(dir_path);
    if (dir_ptr == NULL) {
        perror("opendir");
        return EXIT_FAILURE;
    }

    struct dirent* entries[MAX_DIR_SIZE];
    int n_dir_entries = read_entries(dir_ptr, entries);

    // sort entries alphabetically
    qsort(entries, n_dir_entries, sizeof(struct dirent *), entry_cmp);

    int res = traverse(dir_path, entries, n_dir_entries, depth);
    closedir(dir_ptr);
    return res;
}

/**
 * reads (non-hidden) directory entry pointers from dir_ptr and stores them into entries array.
 * returns number of (non-hidden) elements it read.
*/

int read_entries(DIR* dir_ptr, struct dirent* entries[]) {
    struct dirent* cur_entry_ptr;
    int n = 0;
    // store directory entry pointers in the array that will be sorted afterwards
    while((cur_entry_ptr = readdir(dir_ptr)) != NULL) {
        if (cur_entry_ptr -> d_name[0] != '.') {     // ignore hidden files, . and ..
            
            entries[n] = cur_entry_ptr;
            n++;
            if (n == MAX_DIR_SIZE)
                break;
        }
    }
    return n;
}

int entry_cmp(const void *f1, const void *f2) {
    struct dirent* dir_entry1 = *(struct dirent **) f1;
    struct dirent* dir_entry2 = *(struct dirent **) f2;

    return strcmp(dir_entry1 -> d_name, dir_entry2 -> d_name);
}

// prints str indented by depth*INDENT_SIZE spaces ending with newline
void print_with_indent(const char* str, int depth) {
    for(int i = 0; i < depth * INDENT_SIZE; i++) write(1, " ", 1);
    printf("%s\n", str);
}

/**
 * Prints names of directory entries stored in entries recursively.
 * dir_path       - path to the directory
 * entries        - list of entries of the dicetory
 * n_dir_endtries - number of entries in the list
 * depth          - depth at which is the ls_helper that called this function
 * 
 * returns EXIT_FAILURE if ls_helper fails in some way or if it gets so deep that
 * the full path exceeds PATH_MAX
 * 
*/
static int traverse(char* dir_path, struct dirent* entries[], int n_dir_entries, int depth) {
    // creating and cleaning whole buffer of absolute path would be resource
    // and time-consuming, so I use the same buffer again by saving its size
    // before modification as we are not modifying what's already written
    // in it, only appending new data every time ls_helper is called
    int path_len = strlen(dir_path);
    dir_path[path_len] = '/';
    path_len++;

    int res = EXIT_SUCCESS;
    for(int i = 0; i < n_dir_entries; i++) {
        // return string stored in dir_path to the initial form with '/' at the end
        dir_path[path_len] = '\0';
        char* cur_entry_name = entries[i] -> d_name;
        int cur_entry_name_len = strlen(cur_entry_name);
        print_with_indent(cur_entry_name, depth);

        if (path_len + cur_entry_name_len < PATH_MAX) {
            strcat(dir_path, cur_entry_name);
            dir_path[path_len + cur_entry_name_len] = '\0';
            res |= ls_helper(dir_path, depth+1);
        } else {
            printf("%s%s: path too long to work with\n", dir_path, cur_entry_name);
            res |= EXIT_FAILURE;
        }
    }
    // return string stored in dir_path to the initial form
    dir_path[path_len-1] = '\0';
    return res;
}
