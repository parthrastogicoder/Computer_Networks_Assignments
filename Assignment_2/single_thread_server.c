
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <errno.h>
// importing libraries
// defining port number
#define PORT 8000
// defining buffer size
#define BUFFER_SIZE 8072
// defining infinite loop
#define INFI 1
// defining structure to hold information about a process
typedef struct {
    int pid;
    char name[256];
    long user_time;
    long kernel_time;
} process_info;
//  function to get the top two CPU-consuming processes and store the result in a string
void get_ans_processes(char *result) {
    // declaring variables
    DIR *dir;
    struct dirent *ent;
    char path[BUFFER_SIZE], buffer[BUFFER_SIZE];
    process_info ans[2] = {{0, "", 0, 0}, {0, "", 0, 0}};
    //  opening the /proc directory to read process information
    if ((dir = opendir("/proc")) != NULL) {
        // reading each entry in the directory
        while ((ent = readdir(dir)) != NULL) {
            if (isdigit(ent->d_name[0])) {
                // constructing the path to the process's stat file
                snprintf(path, sizeof(path), "/proc/%s/stat", ent->d_name);
                // opening the stat file for reading
                int fd = open(path, O_RDONLY);
                // checking if the file is opened
                if (fd != -1) {
                    read(fd, buffer, sizeof(buffer) - 1);
                    // closing the file descriptor
                    close(fd);
                    // declaring variables
                    int pid;
                    char pname[256];
                    long user_time, kernel_time;
                    // parsing the process information from the stat file
                    sscanf(buffer, "%d (%[^)]) %*c %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %ld %ld", &pid, pname, &user_time, &kernel_time);
                    long total_time = user_time + kernel_time;
                    if (total_time > ans[0].user_time + ans[0].kernel_time) {
                        ans[1] = ans[0];
                        ans[0] = (process_info){pid, "", user_time, kernel_time};
                        strcpy(ans[0].name, pname);
                    } else if (total_time > ans[1].user_time + ans[1].kernel_time) {
                        ans[1] = (process_info){pid, "", user_time, kernel_time};
                        strcpy(ans[1].name, pname);
                    }
                }
            }
        }
        closedir(dir);
    }
// formatting the result string with top 2 CPU processes
    snprintf(result, BUFFER_SIZE,
             "Top 2 CPU processes:\n"
             "1. PID: %d, Name: (%s), User Time(Ticks): %ld, Kernel Time(Ticks): %ld\n"
             "2. PID: %d, Name: (%s), User Time(Ticks): %ld, Kernel Time(Ticks): %ld\n",
             ans[0].pid, ans[0].name, ans[0].user_time, ans[0].kernel_time,
             ans[1].pid, ans[1].name, ans[1].user_time, ans[1].kernel_time);
}

// Main server function
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
//  checking if the binding is successful
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
//  accepting incoming connections
    printf("Server listening on port %d\n", PORT);
    while (INFI) {
        // Accept a new connection
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        // Get client information
        struct sockaddr_in addr;
        // Get the client address information
        socklen_t addr_len = sizeof(addr);
        // Get the client IP address and port
        getpeername(new_socket, (struct sockaddr*)&addr, &addr_len);
        // Convert the IP address to a string
        char *client_ip = inet_ntoa(addr.sin_addr);
        // Convert the port number to host byte order
        int client_port = ntohs(addr.sin_port);
        // Print the client information
        printf("Client connected: IP = %s, Port = %d\n", client_ip, client_port);
        read(new_socket, buffer, BUFFER_SIZE);
        char result[BUFFER_SIZE];
        // Get the top CPU processes
        get_ans_processes(result);
        send(new_socket, result, strlen(result), 0);
        printf("Serving client: IP = %s, Port = %d\n", client_ip, client_port);
        close(new_socket);
    }
    close(server_fd);
    return 0;
}
