#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_PATH 4096
#define MAX_FILES 1000
#define MAX_DIRS  500

// Structure to store file info
typedef struct {
    char path[MAX_PATH];
    long size;
} FileInfo;

// Structure to store directory info
typedef struct {
    char path[MAX_PATH];
    int file_count;
} DirInfo;

FileInfo files[MAX_FILES];
DirInfo dirs[MAX_DIRS];
int file_count = 0;
int dir_count = 0;

// Recursive function to scan directories and collect info
void scan_directory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    char fullpath[MAX_PATH];

    int local_file_count = 0; // count files in this directory

    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(fullpath, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Recurse into subdirectory
                scan_directory(fullpath);
            } else if (S_ISREG(st.st_mode)) {
                // Regular file
                if (file_count < MAX_FILES) {
                    strcpy(files[file_count].path, fullpath);
                    files[file_count].size = st.st_size;
                    file_count++;
                }
                local_file_count++;
            }
        }
    }

    // Store directory info
    if (dir_count < MAX_DIRS) {
        strcpy(dirs[dir_count].path, path);
        dirs[dir_count].file_count = local_file_count;
        dir_count++;
    }

    closedir(dir);
}

// Comparison function for sorting (largest files first)
int compare_files(const void *a, const void *b) {
    long sa = ((FileInfo *)a)->size;
    long sb = ((FileInfo *)b)->size;
    return (sb - sa); // descending order
}

// Comparison for directories (most files first)
int compare_dirs(const void *a, const void *b) {
    int ca = ((DirInfo *)a)->file_count;
    int cb = ((DirInfo *)b)->file_count;
    return (cb - ca); // descending
}

int main(void) {
    char root_dir[] = "example_root";
    FILE *report = fopen("file_system_report.txt", "w");
    if (!report) {
        perror("file_system_report.txt");
        return 1;
    }

    // Step 1: Scan all directories recursively
    scan_directory(root_dir);

    // Step 2: Calculate total storage
    long total_storage = 0;
    for (int i = 0; i < file_count; i++)
        total_storage += files[i].size;

    // Step 3: Sort files and directories
    qsort(files, file_count, sizeof(FileInfo), compare_files);
    qsort(dirs, dir_count, sizeof(DirInfo), compare_dirs);

    // Step 4: Write report
  
    fprintf(report, "   FILE SYSTEM REPORT\n");

    fprintf(report, "Total files found: %d\n", file_count);
    fprintf(report, "Total directories found: %d\n", dir_count);
    fprintf(report, "Total storage used: %ld bytes (%.2f KB)\n\n",
            total_storage, total_storage / 1024.0);

    fprintf(report, "Top 5 Largest Files:\n");
    int top = (file_count < 5) ? file_count : 5;
    for (int i = 0; i < top; i++) {
        fprintf(report, "%d. %s — %ld bytes\n", i + 1, files[i].path, files[i].size);
    }

    fprintf(report, "\nDirectories with Most Files:\n");
   
    int top_dirs = (dir_count < 5) ? dir_count : 5;
    for (int i = 0; i < top_dirs; i++) {
        fprintf(report, "%d. %s — %d files\n", i + 1, dirs[i].path, dirs[i].file_count);
    }


    fclose(report);
    printf("Total files scanned: %d, Total storage: %ld bytes\n", file_count, total_storage);
    return 0;
}
