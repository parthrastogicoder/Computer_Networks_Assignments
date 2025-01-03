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
#include <sys/select.h>

#define PORT 8000           // Define the port number for the server
#define BUFFER_SIZE 8072     // Define the size of the buffer for data transmission
#define MAX_CLIENTS 100       // Define the maximum number of concurrent clients
#define INFI 1               // Infinite loop condition for the server

// Structure to hold information about a process
typedef struct {
    int pid;                // Process ID
    char name[256];        // Name of the process
    long user_time;        // Amount of CPU time used in user mode IN TICKS 
    long kernel_time;      // Amount of CPU time used in kernel mode IN TICKS
} process_info;

// Function to get the top two CPU-consuming processes and store the result in a string
void get_ans_processes(char *result) {
    DIR *dir; // Directory pointer for /proc
    struct dirent *ent; // Structure for directory entries
    process_info ans[2] = {{0, "", 0, 0}, {0, "", 0, 0}}; // Array to hold the top 2 processes
    char path[BUFFER_SIZE], buffer[BUFFER_SIZE]; // Buffers for file paths and contents
    
    // Open the /proc directory to read process information
    if ((dir = opendir("/proc")) != NULL) {
        while ((ent = readdir(dir)) != NULL) { // Read each entry in the directory
            // Check if the directory name is a digit (indicating a PID)
            if (isdigit(ent->d_name[0])) {
                // Construct the path to the process's stat file
                snprintf(path, sizeof(path), "/proc/%s/stat", ent->d_name);
                int fd = open(path, O_RDONLY); // Open the stat file for reading
                if (fd != -1) {
                    // Read the contents of the stat file into the buffer
                    read(fd, buffer, sizeof(buffer) - 1); // Read and leave space for null-terminator
                    close(fd); // Close the file descriptor
                    int pid; // Variable to hold the process ID
                    char pname[256]; // Variable to hold the process name
                    long user_time, kernel_time; // Variables for CPU time
                    
                    // Parse the process information from the stat file
                    sscanf(buffer, "%d (%[^)]) %*c %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %ld %ld", 
                           &pid, pname, &user_time, &kernel_time);
                    long total_time = user_time + kernel_time; // Calculate total CPU time used

                    // Check if this process has the highest CPU time so far
                    if (total_time > ans[0].user_time + ans[0].kernel_time) {
                        ans[1] = ans[0]; // Shift previous highest to second
                        ans[0] = (process_info){pid, "", user_time, kernel_time}; // Update top process
                        strcpy(ans[0].name, pname); // Store the name of the top process
                    } else if (total_time > ans[1].user_time + ans[1].kernel_time) {
                        ans[1] = (process_info){pid, "", user_time, kernel_time}; // Update second process
                        strcpy(ans[1].name, pname); // Store the name of the second process
                    }
                }
            }
        }
        closedir(dir); // Close the directory stream after reading all entries
    }

    // Format the result string with the top 2 CPU processes
  snprintf(result, BUFFER_SIZE,
             "Top 2 CPU processes:\n"
             "1. PID: %d, Name: (%s), User Time(Ticks): %ld, Kernel Time(Ticks): %ld\n"
             "2. PID: %d, Name: (%s), User Time(Ticks): %ld, Kernel Time(Ticks): %ld\n",
             ans[0].pid, ans[0].name, ans[0].user_time, ans[0].kernel_time,
             ans[1].pid, ans[1].name, ans[1].user_time, ans[1].kernel_time);
}

int main() {
    int noc = 0; // Variable to track the number of connected clients
    // Create a socket for the server
    int mainSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mainSocket < 0) { // Check for socket creation error
        perror("Main Socket Could Not be Created"); 
        exit(1); 
    }

    // Set up the server address structure
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET; // Use IPv4 address family
    serverAddress.sin_port = htons(PORT); // Set the server port, converting to network byte order
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP address

    // Bind the socket to the server address
    int bindResponse = bind(mainSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (bindResponse < 0) { // Check for binding error
        perror("Bind Failed");
        exit(1); 
    }

    // Start listening for incoming connections
    int listenResponse = listen(mainSocket, MAX_CLIENTS);
    if (listenResponse < 0) { // Check for listening error
        perror("Listen Failed"); 
        exit(1); 
    }

    printf("Server listening on port %d\n", PORT); // Inform that the server is up and running

    fd_set allFDs; // Set to hold all file descriptors
    fd_set constantlyUpdatedFDs; // Set for the current state of file descriptors

    // Initialize the set of file descriptors
    FD_ZERO(&allFDs); 
    FD_SET(mainSocket, &allFDs);

    struct sockaddr_in clientData; // Structure to hold client address information
    socklen_t clientDataLength = sizeof(clientData); // Length of the client address structure

    // Infinite loop to accept and handle client connections
    while (INFI) {
        constantlyUpdatedFDs = allFDs; // Copy the main set for select()

        // Check if the maximum number of clients has been reached
        if (noc >= MAX_CLIENTS) {
            printf("Max Connections Reached. Server closed, sorry :)\n"); // Inform about max clients
            break; // Break the loop to stop the server
        }

        // Use select to wait for activity on any of the file descriptors
        int selectResponse = select(FD_SETSIZE, &constantlyUpdatedFDs, NULL, NULL, NULL);
        if (selectResponse < 0) { // Check for select error
            perror("Select Failed"); 
            exit(1); 
        }

        // Iterate through all possible file descriptors
        for (int i = 0; i < FD_SETSIZE; i++) {
            // Check if the file descriptor is ready for reading
            if (FD_ISSET(i, &constantlyUpdatedFDs)) {
                // Check if the ready file descriptor is not the main socket
                if (i != mainSocket) {
                    char receiving_buffer[1000]; // Buffer to hold incoming data
                    bzero(receiving_buffer, 1000); // Clear the buffer
                    int readResponse = read(i, receiving_buffer, 1000); // Read data from the client

                    if (readResponse < 0) { // Check for read error
                        perror("Read Failed"); 
                        exit(1); 
                    }

                    // Check if the client has disconnected (readResponse == 0)
                    if (readResponse == 0) {
                        // Remove the client from the set and close the socket
                        FD_CLR(i, &allFDs); // Clear the file descriptor from the set
                        close(i); 
                        noc--; 
                        continue; 
                    } else {
                        char result[BUFFER_SIZE]; // Buffer to hold the result
                        get_ans_processes(result); // Get the top CPU processes
                        send(i, result, strlen(result), 0); // Send the result back to the client

                        // Retrieve and print client information
                        getpeername(i, (struct sockaddr*)&clientData, &clientDataLength);
                        printf("Client Address: %s, Port: %d, Request received.\n",
                               inet_ntoa(clientData.sin_addr), ntohs(clientData.sin_port));
                    }
                } else {
                    // Accept a new incoming connection
                    int nc = accept(mainSocket, (struct sockaddr*)&clientData, &clientDataLength);
                    if (nc < 0) { // Check for accept error
                        perror("Accept Failed"); 
                        exit(1); 
                    }

                    // Add the new client socket to the set
                    FD_SET(nc, &allFDs); 
                    noc++;
                    printf("New client connected: IP = %s, Port = %d\n",
                           inet_ntoa(clientData.sin_addr), ntohs(clientData.sin_port));
                }
            }
        }
    }

    close(mainSocket); // Close the main socket
    return 0; 
}
