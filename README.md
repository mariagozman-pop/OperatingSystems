# Operating System Mini Projects

This repository contains three mini projects related to operating system concepts, including file system management, process and thread synchronization, and inter-process communication. Below are detailed descriptions of each project.

---

## 1. File System Management

### Overview
This project implements a simple file system management tool that allows users to list, parse, and extract files based on certain conditions.

### Features
- **List files and directories:** Supports recursive listing and filtering by size and permissions.
- **Parse file structure:** Reads a custom file format with a specific header structure.
- **Extract content:** Retrieves specific lines from a file section while ensuring integrity checks.
- **Search for files:** Finds files based on specific content criteria.

### Usage
The program supports the following commands:
- `variant` – Displays the variant number.
- `list path=<directory_path> [recursive] [size_greater=<size>] [permissions=<permissions>]` – Lists files with optional filtering.
- `parse path=<file_path>` – Parses and validates the custom file format.
- `extract path=<file_path> section=<section_no> line=<line_no>` – Extracts a specific line from a file.
- `findall path=<directory_path>` – Finds all files with at least one section containing 16 lines.

---

## 2. Processes and Threads Management

### Overview
This project demonstrates process creation, synchronization, and thread management using mutexes, condition variables, and semaphores.

### Features
- **Process creation:** Implements a specific process hierarchy using `fork()`.
- **Thread synchronization:** Uses mutexes and condition variables for controlled execution.
- **Thread execution limits:** Restricts the number of concurrently running threads.
- **Inter-process thread coordination:** Ensures specific execution order between processes and their threads.

### Usage
The main program initializes processes and threads as follows:
- Creates child processes and threads within them.
- Synchronizes execution of threads across processes using semaphores.
- Limits concurrent execution of specific threads.

---

## 3. Inter-Process Communication Mechanisms

### Overview
This project implements inter-process communication (IPC) using named pipes and shared memory.

### Features
- **Named pipes (FIFO):** Facilitates communication between two processes.
- **Shared memory:** Allows memory sharing across processes.
- **File mapping:** Supports reading from mapped files into shared memory.
- **Custom command handling:** Reads and writes structured commands via pipes.

### Usage
The main program interacts using the following commands:
- `PING` – Responds with `PONG` and an identifier.
- `CREATE_SHM <size>` – Creates a shared memory segment.
- `WRITE_TO_SHM <offset> <value>` – Writes data to shared memory.
- `MAP_FILE <file_path>` – Maps a file into memory.
- `READ_FROM_FILE_OFFSET <offset> <num_bytes>` – Reads bytes from a file at a specific offset.
- `READ_FROM_FILE_SECTION <section_no> <offset> <num_bytes>` – Reads from a section of a file.
- `READ_FROM_LOGICAL_SPACE_OFFSET <logical_offset> <num_bytes>` – Reads based on logical offsets.
- `EXIT` – Terminates the process.

## Conclusion
These three mini projects provide a practical understanding of essential operating system concepts, including file system management, process control, thread synchronization, and inter-process communication. By implementing these projects, I gained hands-on experience with system calls, concurrency control, and resource sharing, which are crucial for developing efficient and robust operating system utilities.



