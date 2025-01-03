#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8000            // Define the port number for the server
#define BUFFER_SIZE 8072     // Define the size of the buffer for data
#define INFI 1               // Infinite loop condition for the server

// Structure to hold process information
typedef struct {
    int pid;                // Process ID
    char name[256];        // Process name
    long user_time;        // User CPU time (in jiffies)
    long kernel_time;      // Kernel CPU time (in jiffies)
} process_info;

// Function to get the top two CPU-consuming processes
void get_ans_processes(char *result) {
    DIR *dir; // Directory pointer for /proc
    struct dirent *ent; // Directory entry structure
    char path[BUFFER_SIZE], buffer[BUFFER_SIZE]; // Buffers for file paths and contents
    process_info ans[2] = {{0, "", 0, 0}, {0, "", 0, 0}}; // Array to hold top 2 processes

    // Open the /proc directory to read process information
    if ((dir = opendir("/proc")) != NULL) {
        while ((ent = readdir(dir)) != NULL) { // Read each entry in the directory
            // Check if the directory name is a digit (indicating a PID)
            if (isdigit(ent->d_name[0])) {
                // Construct the path to the process's stat file
                snprintf(path, sizeof(path), "/proc/%s/stat", ent->d_name);
                int fd = open(path, O_RDONLY); // Open the stat file for reading
                if (fd != -1) {
                    read(fd, buffer, sizeof(buffer) - 1); // Read the file contents into the buffer
                    close(fd); // Close the file descriptor
                    int pid; // Variable to hold the process ID
                    char pname[256]; // Variable to hold the process name
                    long user_time, kernel_time; // Variables for CPU time

                    // Parse the process information from the stat file
                    sscanf(buffer, "%d (%[^)]) %*c %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %ld %ld", &pid, pname, &user_time, &kernel_time);
                    long total_time = user_time + kernel_time; // Calculate total CPU time

                    // Check if this process has the highest CPU time so far
                    if (total_time > ans[0].user_time + ans[0].kernel_time) {
                        ans[1] = ans[0]; // Shift previous highest to second
                        ans[0] = (process_info){pid, "", user_time, kernel_time}; // Update top process
                        strcpy(ans[0].name, pname); // Store process name
                    } else if (total_time > ans[1].user_time + ans[1].kernel_time) {
                        ans[1] = (process_info){pid, "", user_time, kernel_time}; // Update second process
                        strcpy(ans[1].name, pname); // Store process name
                    }
                }
            }
        }
        closedir(dir); // Close the directory stream after reading all entries
    }

    // Format the result string with top 2 CPU processes
  snprintf(result, BUFFER_SIZE,
             "Top 2 CPU processes:\n"
             "1. PID: %d, Name: (%s), User Time(Ticks): %ld, Kernel Time(Ticks): %ld\n"
             "2. PID: %d, Name: (%s), User Time(Ticks): %ld, Kernel Time(Ticks): %ld\n",
             ans[0].pid, ans[0].name, ans[0].user_time, ans[0].kernel_time,
             ans[1].pid, ans[1].name, ans[1].user_time, ans[1].kernel_time);
}

// Thread function to handle client requests
void *handle_client(void *client_socket) {
    int sock = *(int *)client_socket; // Get the client socket from the passed argument
    free(client_socket); // Free allocated memory for socket
    struct sockaddr_in addr; // Structure to hold client address information
    socklen_t addr_len = sizeof(addr); // Length of the address structure
    char buffer[BUFFER_SIZE] = {0}; // Buffer for reading client data

    // Get client information (IP address and port)
    getpeername(sock, (struct sockaddr*)&addr, &addr_len);
    char *client_ip = inet_ntoa(addr.sin_addr); // Convert IP address to string
    int client_port = ntohs(addr.sin_port); // Convert port number to host byte order
    printf("Client connected: IP = %s, Port = %d\n", client_ip, client_port);

    // Read data from the client (this could be a request)
    read(sock, buffer, BUFFER_SIZE); // Read client request (not used here, but can be utilized)
    char result[BUFFER_SIZE]; // Buffer to hold the response
    get_ans_processes(result); // Get the top CPU processes
    send(sock, result, strlen(result), 0); // Send the result back to the client
    printf("Serving client: IP = %s, Port = %d\n", client_ip, client_port);
    close(sock); // Close the client socket after serving
    pthread_exit(NULL); // Exit the thread after finishing
}

int main() {
    int server_fd, new_socket; // Server socket and new client socket
    struct sockaddr_in address; // Structure to hold server address information
    int addrlen = sizeof(address); // Length of the address structure
    pthread_t thread_id; // Thread identifier

    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed"); // Error handling for socket creation
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    address.sin_family = AF_INET; // Set address family to IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP address
    address.sin_port = htons(PORT); // Set the server port, converting to network byte order

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed"); // Error handling for binding
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 10) < 0) {  
        perror("Listen failed"); // Error handling for listening
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d\n", PORT); // Inform the server is running

    // Infinite loop to accept and handle client connections
    while (INFI) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed "); // Error handling for accepting a new connection
            exit(EXIT_FAILURE);
        }

        // Allocate memory for the client socket and create a new thread to handle it
        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket; // Store the new socket in the allocated memory
        pthread_create(&thread_id, NULL, handle_client, (void *)client_sock); // Create a new thread for the client
        pthread_detach(thread_id); // Detach the thread to allow independent execution
    }

    return 0; // Return from main (though this line is unreachable due to the infinite loop)
}
