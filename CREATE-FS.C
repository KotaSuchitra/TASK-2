/* create_fs.c
   Creates nested directories (3 levels), multiple files per directory with random text,
   and writes a summary CSV with: path,size_bytes,line_count,creation_time.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>   // mkdir
#include <errno.h>
#include <unistd.h>     // access

#define ROOT_DIR "example_root"
#define DIRS_L1 2       // number of dirs at level 1
#define DIRS_L2 2       // number of dirs at level 2 inside each L1
#define DIRS_L3 2       // number of dirs at level 3 inside each L2
#define FILES_PER_DIR 2 // number of files to create in each directory
#define LINES_PER_FILE 10
#define MAX_PATH 4096

// Helper: create a directory if it doesn't exist (mode 0755).
int make_dir_if_needed(const char *path) {
    if (access(path, F_OK) == 0) {
        return 0; // already exists
    }
    if (mkdir(path, 0755) != 0) {
        perror("mkdir");
        return -1;
    }
    return 0;
}

// Helper: generate a random word of length 1..maxw into buffer (null-terminated).
void gen_random_word(char *buf, int maxw) {
    int len = (rand() % maxw) + 1; // at least 1 char
    for (int i = 0; i < len; ++i) {
        buf[i] = 'a' + (rand() % 26);
    }
    buf[len] = '\0';
}

// Create a file at `filepath` and write random text with `lines` lines.
// Also returns the bytes written and line count through pointers and records creation time string.
int create_random_text_file(const char *filepath, int lines, long *out_bytes, int *out_lines, char *created_time_str, size_t time_str_len) {
    FILE *f = fopen(filepath, "w");
    if (!f) {
        perror("fopen");
        return -1;
    }

    // creation timestamp (recorded now)
    time_t now = time(NULL);
    struct tm tmnow;
    localtime_r(&now, &tmnow);
    strftime(created_time_str, time_str_len, "%Y-%m-%d %H:%M:%S", &tmnow);

    long bytes_written = 0;
    int line_count = 0;
    char word[64];

    for (int i = 0; i < lines; ++i) {
        int words_in_line = (rand() % 8) + 1; // 1..8 words
        for (int w = 0; w < words_in_line; ++w) {
            gen_random_word(word, 10);
            int n = fprintf(f, "%s", word);
            if (n < 0) { fclose(f); return -1; }
            bytes_written += n;
            if (w < words_in_line - 1) {
                fputc(' ', f);
                bytes_written += 1;
            }
        }
        fputc('\n', f);
        bytes_written += 1;
        line_count++;
    }

    fflush(f);
    fclose(f);

    if (out_bytes) *out_bytes = bytes_written;
    if (out_lines) *out_lines = line_count;
    return 0;
}

int main(void) {
    srand((unsigned)time(NULL));

    // Open summary CSV for writing (overwrites if exists)
    FILE *summary = fopen("summary.csv", "w");
    if (!summary) {
        perror("fopen summary.csv");
        return 1;
    }
    fprintf(summary, "file_path,size_bytes,line_count,creation_time\n");

    // Make root directory
    if (make_dir_if_needed(ROOT_DIR) != 0) {
        fprintf(stderr, "Failed to create root dir '%s'\n", ROOT_DIR);
        fclose(summary);
        return 1;
    }

    char path_l1[MAX_PATH], path_l2[MAX_PATH], path_l3[MAX_PATH];
    char file_path[MAX_PATH];
    char created_time[64];

    // Level 1 directories
    for (int i = 1; i <= DIRS_L1; ++i) {
        snprintf(path_l1, sizeof(path_l1), "%s/dir_l1_%d", ROOT_DIR, i);
        if (make_dir_if_needed(path_l1) != 0) continue;

        // Create some files in level1 directory
        for (int fidx = 1; fidx <= FILES_PER_DIR; ++fidx) {
            snprintf(file_path, sizeof(file_path), "%s/file_l1_%d_%d.txt", path_l1, i, fidx);
            long bytes; int lines;
            if (create_random_text_file(file_path, LINES_PER_FILE, &bytes, &lines, created_time, sizeof(created_time)) == 0) {
                fprintf(summary, "%s,%ld,%d,%s\n", file_path, bytes, lines, created_time);
                printf("Created %s (%ld bytes, %d lines)\n", file_path, bytes, lines);
            }
        }

        // Level 2 directories inside each level 1
        for (int j = 1; j <= DIRS_L2; ++j) {
            snprintf(path_l2, sizeof(path_l2), "%s/dir_l2_%d", path_l1, j);
            if (make_dir_if_needed(path_l2) != 0) continue;

            // Create some files in level2 directory
            for (int fidx = 1; fidx <= FILES_PER_DIR; ++fidx) {
                snprintf(file_path, sizeof(file_path), "%s/file_l2_%d_%d.txt", path_l2, j, fidx);
                long bytes; int lines;
                if (create_random_text_file(file_path, LINES_PER_FILE, &bytes, &lines, created_time, sizeof(created_time)) == 0) {
                    fprintf(summary, "%s,%ld,%d,%s\n", file_path, bytes, lines, created_time);
                    printf("Created %s (%ld bytes, %d lines)\n", file_path, bytes, lines);
                }
            }

            // Level 3 directories inside each level 2
            for (int k = 1; k <= DIRS_L3; ++k) {
                snprintf(path_l3, sizeof(path_l3), "%s/dir_l3_%d", path_l2, k);
                if (make_dir_if_needed(path_l3) != 0) continue;

                // Create some files in level3 directory
                for (int fidx = 1; fidx <= FILES_PER_DIR; ++fidx) {
                    snprintf(file_path, sizeof(file_path), "%s/file_l3_%d_%d.txt", path_l3, k, fidx);
                    long bytes; int lines;
                    if (create_random_text_file(file_path, LINES_PER_FILE, &bytes, &lines, created_time, sizeof(created_time)) == 0) {
                        fprintf(summary, "%s,%ld,%d,%s\n", file_path, bytes, lines, created_time);
                        printf("Created %s (%ld bytes, %d lines)\n", file_path, bytes, lines);
                    }
                }
            } // end level3
        } // end level2
    } // end level1

    fclose(summary);
    printf("Summary written to summary.csv\n");
    return 0;
}
