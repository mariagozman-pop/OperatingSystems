#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define PATH_MAX 4096
#define MAGIC_SIZE 2
#define HEADER_SIZE_SIZE 2
#define VERSION_SIZE 2
#define NO_OF_SECTIONS_SIZE 1
#define SECTION_HEADER_SIZE 32
#define SECT_NAME_SIZE 20
#define SECT_TYPE_SIZE 4
#define SECT_OFFSET_SIZE 4
#define SECT_SIZE_SIZE 4

typedef struct {
    char magic_number[3];
    int header_size;
    int version;
    int no_of_sections;
    struct Section {
        char name[21];
        int type;
        int size;
        int offset;
    } sections[13]; 
} Header;

void variantCommand()
{
    printf("95161\n");
}

int checkSize(char* fullPath, int sizeGreater)
{
    //option not specified => no filter
    if(sizeGreater == -1)
        return 1;

    struct stat fileStat;
    if (stat(fullPath, &fileStat) == -1) {
        perror("stat");
        return 0;
    }

    //make sure the sb-file size is right
    if (fileStat.st_size > sizeGreater) {
        return 1;
    }
    else {
        return 0;
    }
}

int checkPermissions(char* fullPath, char *permissions)
{
    //option not specified => no filter
    if (permissions == NULL)
        return 1;

    struct stat fileStat;
    char perms_str[10]; 
    if (stat(fullPath, &fileStat) == -1) {
        perror("stat");
        return 0;
    }

    //get the permissions octal num and convert to string
    int dir_perms = fileStat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    for (int i = 0; i < 9; i++) {
        if (dir_perms & (1 << (8 - i))) {
            if (i % 3 == 0) {
                perms_str[i] = 'r'; 
            } else if (i % 3 == 1) {
                perms_str[i] = 'w'; 
            } else {
                perms_str[i] = 'x'; 
            }
        } else {
            perms_str[i] = '-';
        }
    }
    perms_str[9] = '\0';

    //compare permissions strings
    if (strcmp(perms_str, permissions) == 0) {
        return 1; 
    } else {
        return 0; 
    }
}

void listDirectoryContents(char* directoryPath, int sizeGreater, char* permissions)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir(directoryPath);
    if (dir == NULL) {
        printf("ERROR\nunable to open directory: %s\n", directoryPath);
        return;
    }

    printf("SUCCESS\n");

    //read all the contents of the directory
    while ((entry = readdir(dir)) != NULL) {

         if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char *name = entry->d_name;
        char fullPath[PATH_MAX];
        struct stat fileStat;

        //construct full path
        snprintf(fullPath, sizeof(fullPath), "%s/%s", directoryPath, name);

        if (stat(fullPath, &fileStat) == -1) {
            printf("ERROR\nunable to get file information for: %s\n", fullPath);
            continue;
        }

        //find the entry type and filter accordingly
        if (S_ISREG(fileStat.st_mode)) {
            if((checkPermissions(fullPath, permissions))&&(checkSize(fullPath, sizeGreater))){
                printf("%s\n", fullPath);
            }
        } else if (S_ISDIR(fileStat.st_mode)) {
            if(checkPermissions(fullPath, permissions)){
                 if(sizeGreater == -1){
                    printf("%s\n", fullPath);
                 }
            }
        } else {
            printf("ERROR\nunknown type: %s\n", name);
        }
    }

    closedir(dir);
}

//recursive version of listDirectoryContents
void listDirectoryContentsRec(char* directoryPath, int sizeGreater, char* permissions)
{
    static int succFlag = 0;
    DIR *dir;
    struct dirent *entry;

    dir = opendir(directoryPath);
    if (dir == NULL) {
        printf("ERROR\nunable to open directory: %s\n", directoryPath);
        return;
    }

    if(succFlag == 0){
        printf("SUCCESS\n");
        succFlag = 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char *name = entry->d_name;
        char fullPath[PATH_MAX];
        struct stat fileStat;

        snprintf(fullPath, sizeof(fullPath), "%s/%s", directoryPath, name);

        if (stat(fullPath, &fileStat) == -1) {
            printf("ERROR\nunable to get file information for: %s\n", fullPath);
            continue;
        }

        if (lstat(fullPath, &fileStat) == -1) {
            perror("lstat");
            closedir(dir);
            return; 
        }

        if (S_ISLNK(fileStat.st_mode)) {
            continue;
        } else { 
            if (S_ISREG(fileStat.st_mode)) {
                if((checkPermissions(fullPath, permissions))&&(checkSize(fullPath, sizeGreater))){
                    printf("%s\n", fullPath);
                }
            } else if (S_ISDIR(fileStat.st_mode)) {
                if((checkPermissions(fullPath, permissions))){
                    if(sizeGreater == -1){
                        printf("%s\n", fullPath);
                    }
                    listDirectoryContentsRec(fullPath, sizeGreater, permissions);
                }
            } else{
                printf("ERROR\nunknown type");
            }
        }
    }

    closedir(dir);
}

void listCommand(int numArgs, char **args) 
{
    char *directoryPath = NULL;
    int recursiveFlag = 0;
    int sizeGreater = -1; 
    char *permissions = NULL;

    //parse options
    for (int i = 0; i < numArgs; i++) {
        if (strcmp(args[i], "recursive") == 0) {
            recursiveFlag = 1;
        } else if (strncmp(args[i], "size_greater=", 13) == 0) {
            sizeGreater = atoi(args[i] + 13); 
        } else if (strncmp(args[i], "permissions=", 12) == 0) {
            permissions = args[i] + 12; 
            if (strlen(permissions) != 9) {
                printf("ERROR\ninvalid permissions format\n");
                return;
            }
        } else if (strncmp(args[i], "path=", 5) == 0) {
            directoryPath = args[i] + 5;
        } else {
            printf("ERROR\nunsupported option: %s\n", args[i]);
            return; 
        }
    }

    if(directoryPath == NULL){ 
        printf("ERROR\nno directory path provided\n");
        return;
    }

    //ensure the right permissions string format
    if(permissions!=NULL){
        for (int j = 0; j < 9; j++) {
            if ((j % 3 == 0) && permissions[j] != 'r' && permissions[j] != '-') {
                printf("ERROR\ninvalid permission string format\n");
                return;
            }
            if ((j % 3 == 1) && permissions[j] != 'w' && permissions[j] != '-') {
                printf("ERROR\ninvalid permission string format\n");
                return;
            }
            if ((j % 3 == 2) && permissions[j] != 'x' && permissions[j] != '-') {
                printf("ERROR\ninvalid permission string format\n");
                return;
            }
        }
    }

    if(recursiveFlag)
        listDirectoryContentsRec(directoryPath, sizeGreater, permissions);
    else
        listDirectoryContents(directoryPath, sizeGreater, permissions);
}

int allowed_section_types[] = {47, 66, 39, 75, 31, 48};

//initializes the header variables to 0 (very important!!)
void initHeader(Header *header) 
{
    header->header_size = 0;
    header->version = 0;
    header->no_of_sections = 0;

    for (int i = 0; i < 13; i++) {
        memset(header->sections[i].name, 0, SECT_NAME_SIZE);
    }

    for (int i = 0; i < 3; i++) {
        memset(header->magic_number, 0, MAGIC_SIZE);
    }

    for (int i = 0; i < 13; i++) {
        header->sections[i].type = 0;
        header->sections[i].size = 0;
        header->sections[i].offset = 0;
    }
}

int corrupted(char *filePath, Header* header, int printFlag) 
{
    int fd = open(filePath, O_RDONLY);
    if (fd == -1) {
        printf("ERROR\nfailed to open file: %s\n", filePath);
        return -1;
    }

    //start the reading from the end
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        perror("lseek");
        close(fd);
        return -1;
    }

    //find the magic number
    if (lseek(fd, -MAGIC_SIZE, SEEK_END) == -1) {
        perror("lseek");
        close(fd);
        return -1;
    }

    if (read(fd, header->magic_number, MAGIC_SIZE) != MAGIC_SIZE) {
        perror("read");
        close(fd);
        return -1;
    }
    header->magic_number[MAGIC_SIZE] = '\0';
    
    //check for the correct magic number
    if (strcmp(header->magic_number, "Fh") != 0) {
        if(printFlag) {
            printf("ERROR\nwrong magic\n");
        }
        close(fd);
        return -1;
    }

    //find the header size
    off_t header_size_offset = -MAGIC_SIZE - HEADER_SIZE_SIZE;
    if (lseek(fd, header_size_offset, SEEK_END) == -1) {
        perror("lseek");
        close(fd);
        return -1;
    }
    if (read(fd, &header->header_size, HEADER_SIZE_SIZE) != HEADER_SIZE_SIZE) {
        perror("read");
        close(fd);
        return -1;
    }

    //start again, but from the beginning of the header
    if (lseek(fd, -header->header_size, SEEK_END) == -1) {
        perror("lseek");
        close(fd);
        return -1;
    }

    //read and check for the correct version
    if (read(fd, &header->version, VERSION_SIZE) != VERSION_SIZE) {
        perror("read");
        close(fd);
        return -1;
    }
    if (header->version > 115 || header->version < 87) {
        if(printFlag) {
            printf("ERROR\nwrong version\n");
        }
        close(fd);
        return -1;
    }

    if (read(fd, &header->no_of_sections, NO_OF_SECTIONS_SIZE) != NO_OF_SECTIONS_SIZE) {
        perror("read");
        close(fd);
        return -1;
    }

    //check if the number of sections is correct
    if ((header->no_of_sections != 2 && header->no_of_sections < 6) || header->no_of_sections > 12) {
        if(printFlag) {
            printf("ERROR\nwrong sect_nr\n");
        }
        close(fd);
        return -1;
    }

    //read and check each section header
    for (int i = 0; i < header->no_of_sections; i++) {
        //read the section header title from the file
        if (read(fd, &header->sections[i].name, SECT_NAME_SIZE) != SECT_NAME_SIZE) {
            perror("read");
            close(fd);
            return -1;
        }
        header->sections[i].name[SECT_NAME_SIZE] = '\0';

        //read the section header type from the file
        if (read(fd, &header->sections[i].type, SECT_TYPE_SIZE) != SECT_TYPE_SIZE) {
            perror("read");
            close(fd);
            return -1;
        }
        
        //check if the section type is in the allowed set
        int j;
        for (j = 0; j < 6; j++) {
            if (header->sections[i].type == allowed_section_types[j]) {
                break;
            }
        }
        
        //if the section type is not in the allowed set
        if (j == 6) {
            if(printFlag) {
                printf("ERROR\nwrong sect_types\n");
            }
            close(fd);
            return -1;
        }

        //read the section header offset from the file
        if (read(fd, &header->sections[i].offset, SECT_OFFSET_SIZE) != SECT_OFFSET_SIZE) {
            perror("read");
            close(fd);
            return -1;
        }

        //read the section header size from the file
        if (read(fd, &header->sections[i].size, SECT_SIZE_SIZE) != SECT_SIZE_SIZE) {
            perror("read");
            close(fd);
            return -1;
        }
    }
    return 0;
}

void parseCommand(char* argument)
{
    Header header;
    initHeader(&header);

    //parse path
    if (strlen(argument) <= 5 || strncmp(argument, "path=", 5) != 0) {
        printf("ERROR\ninvalid path format: %s\n", argument);
        return;
    }
    char* filePath = argument + 5;

    if(!(corrupted(filePath, &header, 1))) {
        printf("SUCCESS\n");
        printf("version=%d\n", header.version);
        printf("nr_sections=%d\n", header.no_of_sections);
        for(int i = 0; i < header.no_of_sections; i++){
            printf("section%d: %s %d %d\n", i+1, header.sections[i].name, header.sections[i].type, header.sections[i].size);
        }
    }
}

void extractCommand(char** args) 
{
    char* path = NULL;
    int section = -1;
    int line = -1;

    //parse arguments
    for (int i = 0; i < 3; i++) {
        if (strncmp(args[i], "path=", 5) == 0) {
            path = args[i] + 5;
        } else if (strncmp(args[i], "section=", 8) == 0) {
            if(atoi(args[i] + 8)>0)
                section = atoi(args[i] + 8);
        } else if (strncmp(args[i], "line=", 5) == 0) {
            if(atoi(args[i] + 5)>0)
                line = atoi(args[i] + 5);
        }
    }

    //check if all required arguments are provided
    if (path == NULL){
        printf("ERROR\ninvalid path\n");
        return;
    } else if (section == -1){
        printf("ERROR\ninvalid section\n");
        return;
    }
    else if (line == -1) {
        printf("ERROR\ninvalid line\n");
        return;
    }

    //open file
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("ERROR\nfailed to open file: %s\n", path);
        return;
    }
   
    Header header;
    initHeader(&header);

    //check if file complies with SF format
    if(corrupted(path, &header, 0) == -1) {
        printf("ERROR\ninvalid file\n");
        close(fd);
        return;
    }

    //check if the section exists in the file
    if(header.no_of_sections < section) {
        printf("ERROR\ninvalid section\n");
        close(fd);
        return;
    }

    //start from the end of the section
    off_t sectionEnd = header.sections[section-1].offset + header.sections[section-1].size + 1;
    if (lseek(fd, sectionEnd, SEEK_SET) == -1) {
        perror("lseek1");
        close(fd);
        return;
    }

    //find the corresponding line while reading backwards
    int sectionCount = header.sections[section-1].size;
    char currentChar = '\0';
    int offsetCount = 0;
    if(line !=1) {
        while (sectionCount > 0 && lseek(fd, -1, SEEK_CUR) != -1) { 
            if (read(fd, &currentChar, 1) == -1) {
                perror("read");
                close(fd);
                return;
            }
            //if new line character is met, increment the line count
            if (currentChar == 0x0A) { 
                offsetCount++; 
                if (offsetCount == line - 1) { 
                    break;
                }
            }

            if (lseek(fd, -1, SEEK_CUR) == -1) {
                perror("lseek");
                close(fd);
                return;
            }
    
            //if we have reached the end of the section, the line is not present in it
            sectionCount--;
            if(sectionCount == -1){
                printf("ERROR\ninvalid line\n");
                close(fd);
                return;
            }
        }
    }

    //read the line backwards
    char *buffer = (char *)malloc(64 * sizeof(char));
    if (buffer == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    int bufferIndex = 0;
    size_t bufferSize = 64;

     while (lseek(fd, -2, SEEK_CUR) != -1) {
        if (read(fd, &currentChar, 1) != 1) {
            perror("read");
            close(fd);
            free(buffer);
            exit(EXIT_FAILURE);
        }
        if (currentChar == 0x0A) {
            break;
        }
        buffer[bufferIndex++] = currentChar;

        if (bufferIndex >= bufferSize - 1) {
            bufferSize *= 2;
            char* temp = (char*)realloc(buffer, bufferSize * sizeof(char));
            if (temp == NULL) {
                perror("Memory allocation failed");
                close(fd);
                free(buffer);
                exit(EXIT_FAILURE);
            }
            buffer = temp;
        }
    }

    buffer[bufferIndex] = '\0';

    char* correct_line = malloc(strlen(buffer) + 1);
    strcpy(correct_line, buffer);
    free(buffer);

    printf("SUCCESS\n%s\n", correct_line);
    free(correct_line);
}

int checkLines(char *filePath, Header* header) 
{
    int fd = open(filePath, O_RDONLY);
    if (fd == -1) {
        printf("ERROR\nfailed to open file: %s\n", filePath);
        return 0;
    }

    // Iterate through all sections
    for (int i = 0; i < header->no_of_sections; i++) {
        off_t sectionStart = header->sections[i].offset;
        off_t sectionSize = header->sections[i].size;
        int lineCount = 1;
        
        // Allocate memory for the buffer
        char *buffer = (char *)malloc(sectionSize);
        if (buffer == NULL) {
            printf("ERROR\nfailed to allocate memory\n");
            close(fd);
            return 0;
        }

        // Go to the beginning of the section
        if (lseek(fd, sectionStart, SEEK_SET) == -1) {
            perror("lseek");
            free(buffer);
            close(fd);
            return 0;
        }

        // Read the current section into the buffer
        ssize_t bytes_read = read(fd, buffer, sectionSize);
        if (bytes_read == -1) {
            perror("read");
            free(buffer); 
            close(fd);
            return 0;
        } else if (bytes_read != sectionSize) {
            printf("ERROR: Failed to read the entire section\n");
            free(buffer);
            close(fd);
            return 0;
        }

        // Count the number of lines in the current section
        for (int j = 0; j < sectionSize; j++) {
            if (buffer[j] == '\n') {
                lineCount++;
            }
        }

        if (lineCount == 16) {
            free(buffer);
            close(fd);
            return 1;
        }

        free(buffer);
    }

    close(fd);
    return 0; // 16 lines not found in any section
}

void findAll(char *dirPath)
{
    static int succFlag = 0;
    DIR *dir;
    struct dirent *entry;
    dir = opendir(dirPath);

    Header header;
    initHeader(&header);

    if (dir == NULL) {
        printf("ERROR\ninvalid directory path\n");
        return;
    }

    if(succFlag == 0){
        printf("SUCCESS\n");
        succFlag = 1;
    }

    //recursilvely go through each entry
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char *name = entry->d_name;
        char fullPath[PATH_MAX];
        struct stat fileStat;

        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, name);

        if (stat(fullPath, &fileStat) == -1) {
            printf("ERROR\nunable to get file information for: %s\n", fullPath);
            continue;
        }

        if (lstat(fullPath, &fileStat) == -1) {
            perror("lstat");
            closedir(dir);
            return; 
        }

        //if the entry is a link, ignore
        if (S_ISLNK(fileStat.st_mode)) {
            continue;
        } else { 
            //if the entry is a file, check SF specifications and line nr
            if (S_ISREG(fileStat.st_mode)) {
                initHeader(&header);
                if(!(corrupted(fullPath, &header, 0))) {
                    if(checkLines(fullPath, &header)){
                        printf("%s\n", fullPath);
                    }
                }
            } else if (S_ISDIR(fileStat.st_mode)) {
                findAll(fullPath); //if the entry is a directory, it must be explored
            }
            else{
                printf("ERROR\nunknown type");
            }
        }
    }

    closedir(dir);
}

void findallCommand(char* argument)
{
    //parse path
    if (strlen(argument) <= 5 || strncmp(argument, "path=", 5) != 0) {
        printf("ERROR\ninvalid path format: %s\n", argument);
        return;
    }
    char* dirPath = argument + 5;

    findAll(dirPath);
}

int main(int argc, char **argv)
{  
    if (argc < 2) {
        printf("Please provide a command.\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "variant") == 0) {
            variantCommand();
        }
        else if (strcmp(argv[i], "list") == 0) {
            listCommand(argc - i - 1, argv + i + 1);
            break;
        }
        else if(strcmp(argv[i], "parse") == 0){
            parseCommand(argv[i+1]);
            break;
        }
        else if(strcmp(argv[i], "extract") == 0){
            extractCommand(argv + i + 1);
            break;
        }
        else if(strcmp(argv[i], "findall") == 0){
            findallCommand(argv[i+1]);
            break;
        }
        else{
            printf("unknown command: %s\n", argv[i]);
        }
    }

    return 0;
}