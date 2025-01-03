// importing libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
// defining port number
#define PORT 8000
// defining buffer size
#define BUFFER_SIZE 8072
//  function to start the client
void *start_client(void *arg) {
    // declaring variables
    int client_fd;
    struct sockaddr_in serv_addr;
    // declaring buffer
    char buffer[BUFFER_SIZE] = {0};
    // creating socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed :( ");
        pthread_exit(NULL);
    }
    // setting up server address structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    // checking if the address is valid
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address :( \n");
        pthread_exit(NULL);
    }
    // checking if the connection is successful
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Error :( \n");
        pthread_exit(NULL);
    }
    // sending request to the server
    send(client_fd, "GET CPU INFO", strlen("GET CPU INFO"), 0);
    printf("Request sent from thread %ld\n", pthread_self());
    // receiving response from the server
    read(client_fd, buffer, BUFFER_SIZE);
    printf("Received from server at thread %ld:\n%s\n", pthread_self(), buffer);
    // closing the connection
    close(client_fd);
    pthread_exit(NULL);
}
//  main function
int main(int argc, char const *argv[]) {
    // checking if the input is in the correct format
    if (argc != 2) {
        printf("Input in wrong format buddy ");
        return -1;
    }
    // declaring number of clients
    int noc = atoi(argv[1]);
    pthread_t threads[noc];
    // creating threads 
    for (int i = 0; i < noc; i++) {
        pthread_create(&threads[i], NULL, start_client, NULL);
    }
    // joining threads
    for (int i = 0; i < noc; i++) {
        pthread_join(threads[i], NULL);
    }
     printf("Connection closed\n");
    return 0;
}
