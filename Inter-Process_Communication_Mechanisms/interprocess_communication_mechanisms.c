#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>    
#include <sys/mman.h> 
#include <sys/stat.h> 
#include <unistd.h> 
#include <errno.h>
#include <string.h>

#define RESP_PIPE "RESP_PIPE_95161"
#define REQ_PIPE "REQ_PIPE_95161"
#define HELLO_MSG "HELLO!"
#define ERROR_MSG "ERROR!"
#define SUCCESS_MSG "SUCCESS!"
#define BUFFER_SIZE 250
#define MAX_SECTIONS 12
#define ALIGNMENT 2048

typedef struct {
    char name[50];
    unsigned int type;
    unsigned int offset;
    unsigned int logical_offset;
    unsigned int size;
} SectionHeader;

typedef struct {
    char magic[3];
    unsigned int header_size;
    unsigned int version;
    unsigned int no_of_sections;
    SectionHeader sections[MAX_SECTIONS];
} Header;

int resp_fd, req_fd;
void *shm_addr = NULL;
unsigned int shm_size = 0;
void *file_addr = NULL;
unsigned int file_size = 0;
Header header;

void write_string(char* message) 
{
    if (write(resp_fd, message, strlen(message)) == -1) 
    {
        printf(ERROR_MSG);
        perror("\ncannot write to the response pipe");
        close(req_fd);
        close(resp_fd);
        unlink(RESP_PIPE);
        exit(EXIT_FAILURE);
    }
}

void write_number(unsigned int number) 
{
    if (write(resp_fd, &number, sizeof(number)) == -1) 
    {
        printf(ERROR_MSG);
        perror("\ncannot write to the response pipe");
        close(req_fd);
        close(resp_fd);
        unlink(RESP_PIPE);
        exit(EXIT_FAILURE);
    }
}

int read_string(char* buffer)
{
    int index = 0;
    char c = '\0';
    size_t bytes_read = 0;

    while ((bytes_read = read(req_fd, &c, 1)) > 0) 
    {
        if (c == '!') 
        {
            buffer[index++] = '\0';
            break;
        }
        buffer[index++] = c;

        if (index >= BUFFER_SIZE - 1) 
        {
            printf(ERROR_MSG);
            printf("\nstring-field exceeds maximum size\n");
            buffer[index] = '\0';
            break;
        }
    }
    if (bytes_read == -1) 
    {
        printf(ERROR_MSG);
        perror("\ncannot read from the request pipe");
        return -1;
    }
    return 0;
}

void read_number(unsigned int *num)
{
    size_t bytes_read = 0;

    bytes_read = read(req_fd, num, (sizeof(*num)));

    if (bytes_read == -1) 
    {
        printf(ERROR_MSG);
        perror("\ncannot read from the request pipe");
    }
}

void ping_request() 
{
    char ping[6] = "PING!";
    write_string(ping);

    char pong[6] = "PONG!";
    write_string(pong);

    unsigned int num = 95161;
    write_number(num);
}

int create_shm_request() 
{
    const char *shm_name = "/R9LsHC";
    const mode_t shm_permissions = 0664;

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, shm_permissions);
    if (shm_fd == -1) {
        return -1;
    }

    if (ftruncate(shm_fd, shm_size) == -1) 
    {
        close(shm_fd);
        return -1;
    }

    shm_addr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_addr == MAP_FAILED) 
    {
        close(shm_fd);
        return -1;
    }

    return 0;
}

int write_to_shm_request(unsigned int shm_size, unsigned int offset, unsigned int value) 
{
    if (offset < 0 || offset + sizeof(value) > shm_size) {
        return -1;
    }

    memcpy((char*)shm_addr + offset, &value, sizeof(value));

    return 0;
}

int map_file_request(char* file_name) 
{
    int fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) 
    {
        close(fd);
        return -1;
    }

    file_size = st.st_size;
    if (file_size == 0) 
    {
        close(fd);
        return -1;
    }

    file_addr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_addr == MAP_FAILED) 
    {
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}


int read_from_file_offset(unsigned int offset, unsigned int no_of_bytes) 
{
    if (offset + no_of_bytes > file_size) {
        return -1;
    }

    memcpy(shm_addr, file_addr + offset, no_of_bytes);

    return 0;
}

void get_header_info ()
{
    int* header_ptr = file_addr + file_size - 2;
    memcpy(&header.magic, header_ptr, 2);

    header_ptr = file_addr + file_size - 4;
    memcpy(&header.header_size, header_ptr, 2);

    header_ptr = file_addr + file_size - header.header_size;
    memcpy(&header.version, header_ptr, 2);

    header_ptr = file_addr + file_size - header.header_size + 2;
    memcpy(&header.no_of_sections, header_ptr, 1);

    header_ptr = file_addr + file_size - header.header_size + 3;
    int x = 3;

    for (int i = 0; i < header.no_of_sections; i++) 
    {
        memcpy(&header.sections[i].name, header_ptr, 20);
        x = x + 20;
        header_ptr = file_addr + file_size - header.header_size + x;

        memcpy(&header.sections[i].type, header_ptr, 4);
        x = x + 4;
        header_ptr = file_addr + file_size - header.header_size + x;

        memcpy(&header.sections[i].offset, header_ptr, 4);
        x = x + 4;
        header_ptr = file_addr + file_size - header.header_size + x;

        memcpy(&header.sections[i].size, header_ptr, 4);
        x = x + 4;
        header_ptr = file_addr + file_size - header.header_size + x;
    }
}

int read_from_file_section(unsigned int section_no, unsigned int offset, unsigned int no_of_bytes) 
{
    get_header_info();

    if (section_no < 1 || section_no > header.no_of_sections) {
        return -1;
    }

    if (offset >= header.sections[section_no - 1].size) {
        return -1;
    }

    unsigned int section_start = header.sections[section_no - 1].offset + offset;

    if (no_of_bytes > header.sections[section_no - 1].size - offset) {
        return -1;
    }

    memcpy(shm_addr, (char*)file_addr + section_start, no_of_bytes);

    return 0;
}

void bubble_sort_sections(SectionHeader sections[], int n) {
    int i, j;
    for (i = 0; i < n-1; i++) {
        for (j = 0; j < n-i-1; j++) {
            if (sections[j].offset > sections[j+1].offset) {
                SectionHeader temp = sections[j];
                sections[j] = sections[j+1];
                sections[j+1] = temp;
            }
        }
    }
}

int read_from_logical_space_offset(unsigned int logical_offset, unsigned int no_of_bytes) {

    get_header_info();

    bubble_sort_sections(header.sections, header.no_of_sections);

    unsigned int index = 0;
    int section_no = 0;

    for(int i = 0; i < header.no_of_sections; i++)
    {
        header.sections[i].logical_offset =  index;

        memcpy(shm_addr + header.sections[i].logical_offset, (char*)file_addr + header.sections[i].offset, header.sections[i].size);

        if((logical_offset >= header.sections[i].logical_offset) &&
        (logical_offset <= (header.sections[i].logical_offset + header.sections[i].size)))
        {
            section_no = i + 1;
            memcpy(shm_addr, shm_addr + logical_offset, no_of_bytes);
        }

        index = (((header.sections[i].logical_offset + header.sections[i].size) / ALIGNMENT) * ALIGNMENT) + ALIGNMENT;
    }

    if(section_no == 0) {
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[]) 
{
    // create the response pipe
    if (mkfifo(RESP_PIPE, 0666) == -1) 
    {
        if (errno != EEXIST) {
            printf(ERROR_MSG);
            perror("cannot create the response pipe");
            exit(EXIT_FAILURE);
        }
    }

    // open the request pipe for reading
    req_fd = open(REQ_PIPE, O_RDONLY);
    if (req_fd == -1) 
    {
        printf(ERROR_MSG);
        perror("cannot open the request pipe");
        unlink(RESP_PIPE);
        exit(EXIT_FAILURE);
    }

    // open the response pipe for writing
    resp_fd = open(RESP_PIPE, O_WRONLY);
    if (resp_fd == -1) 
    {
        printf(ERROR_MSG);
        perror("cannot open the response pipe");
        close(req_fd);
        unlink(RESP_PIPE);
        exit(EXIT_FAILURE);
    }

    // write the hello message
    if (write(resp_fd, HELLO_MSG, strlen(HELLO_MSG)) == -1) 
    {
        printf(ERROR_MSG);
        perror("cannot write to the response pipe");
        close(req_fd);
        close(resp_fd);
        unlink(RESP_PIPE);
        exit(EXIT_FAILURE);
    }

    // print success
    printf(SUCCESS_MSG);
    printf("\n");

    while (1) 
    {
        int check = 0;
        char buffer[BUFFER_SIZE];
        check = read_string(buffer);

        if(check == -1) 
        {
            break;
        }

        if(strcmp((char *)buffer, "EXIT") == 0) 
        {
            break;
        }

        else if (strcmp((char *)buffer, "PING") == 0) 
        {
            ping_request(resp_fd);
        }

        else if(strcmp((char *)buffer, "CREATE_SHM") == 0) 
        {
            read_number(&shm_size);

            check = create_shm_request(shm_size);

            char message[12] = "CREATE_SHM!";
            if(check == 0)
            {
                write_string(message);
                write_string(SUCCESS_MSG);
            }
            else 
            {
                write_string(message);
                write_string(ERROR_MSG);
            }
        }

        else if(strcmp((char *)buffer, "WRITE_TO_SHM") == 0)
        {
            unsigned int offset = 0;
            unsigned int value = 0;
            read_number(&offset);
            read_number(&value);

            check = write_to_shm_request(shm_size, offset, value);

            char message[14] = "WRITE_TO_SHM!";
            if(check == 0)
            {
                write_string(message);
                write_string(SUCCESS_MSG);
            }
            else 
            {
                write_string(message);
                write_string(ERROR_MSG);
            }
        }

        else if(strcmp((char *)buffer, "MAP_FILE") == 0) 
        {
            char file_name[BUFFER_SIZE] = "\0";
            read_string(file_name);

            check = map_file_request(file_name);

            char message[10] = "MAP_FILE!";
            if(check == 0)
            {
                write_string(message);
                write_string(SUCCESS_MSG);
            }
            else 
            {
                write_string(message);
                write_string(ERROR_MSG);
            }
        }

        else if(strcmp((char *)buffer, "READ_FROM_FILE_OFFSET") == 0) 
        {
            unsigned int offset = 0;
            unsigned int no_of_bytes = 0;
            read_number(&offset);
            read_number(&no_of_bytes);

            check = read_from_file_offset(offset, no_of_bytes);

            char message[23] = "READ_FROM_FILE_OFFSET!";
            if(check == 0)
            {
                write_string(message);
                write_string(SUCCESS_MSG);
            }
            else 
            {
                write_string(message);
                write_string(ERROR_MSG);
            }
        }

        else if(strcmp((char *)buffer, "READ_FROM_FILE_SECTION") == 0) 
        {
            unsigned int section_no = 0;
            unsigned int offset = 0;
            unsigned int no_of_bytes = 0;
            read_number(&section_no);
            read_number(&offset);
            read_number(&no_of_bytes);

            check = read_from_file_section(section_no, offset, no_of_bytes);

            char message[24] = "READ_FROM_FILE_SECTION!";
            if(check == 0)
            {
                write_string(message);
                write_string(SUCCESS_MSG);
            }
            else 
            {
                write_string(message);
                write_string(ERROR_MSG);
            }
        }

        else if(strcmp((char *)buffer, "READ_FROM_LOGICAL_SPACE_OFFSET") == 0) 
        {
            unsigned int logical_offset = 0;
            unsigned int no_of_bytes = 0;
            read_number(&logical_offset);
            read_number(&no_of_bytes);

            check = read_from_logical_space_offset(logical_offset, no_of_bytes);

            char message[32] = "READ_FROM_LOGICAL_SPACE_OFFSET!";
            if(check == 0)
            {
                write_string(message);
                write_string(SUCCESS_MSG);
            }
            else 
            {
                write_string(message);
                write_string(ERROR_MSG);
            }
        }
    }

    close(req_fd);
    close(resp_fd);
    unlink(RESP_PIPE);

    return 0;
}