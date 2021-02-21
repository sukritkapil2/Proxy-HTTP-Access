/* f20180205@hyderabad.bits-pilani.ac.in Sukrit Kapil */

/* Program to download main page and logo of a website through socket connection to a proxy */
/* ... */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int sock_fd;
    struct sockaddr_in server;
    FILE* fptr;

    fptr = fopen("index.html", "w");

    // assign all the command-line arguments
    char* website = argv[1];
    char* proxy_server = argv[2];
    char* proxy_port = argv[3];
    char* username = argv[4];
    char* password = argv[5];
    char* home_page = argv[6];
    char* logo = argv[7];

    if(logo == NULL) printf("\n[INFO] logo will not be downloaded\n");

    char* auth_str = "Y3NmMzAzOmNzZjMwMw==";
    
    // create the socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    // error in socket creation
    if(sock_fd == -1) {
        printf("\n[ERROR] could not create socket\n");
        exit(1);
    }

    // setup the server info
    server.sin_addr.s_addr = inet_addr(proxy_server);
    server.sin_port = htons(atoi(proxy_port));
    server.sin_family = AF_INET;

    // connect to the server
    if(connect(sock_fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        printf("\n[ERROR] could not connect to server\n");
        exit(1);
    }

    printf("\n[SUCCESS] connected to the server\n");

    // request header
    char message[8192] = "GET http://";
    strcat(message, website);
    strcat(message, " HTTP/1.1\r\nHost: ");
    strcat(message, website);
    strcat(message, "\r\nProxy-Authorization: Basic ");
    strcat(message, auth_str);
    strcat(message, "\r\nConnection: close\r\n\r\n");

    // send request to the server
    if(send(sock_fd, message, strlen(message), 0) < 0) {
        printf("\n[ERROR] could not send request to server\n");
        return -1;
    }
    printf("\n[SUCCESS] request sent to the server\n\n[FETCHING] home page is being fetched...\n");

    char* final_response;
    final_response = (char *)malloc(sizeof(char) * 1024);
    int total_size = 1024;
    int current_size, old_size, total_received = 0;
    int index = 0;

    // fetch data in chunks from the server till no response is there
    while((current_size = recv(sock_fd, final_response + index, 1024, 0)) > 0) {
            index += current_size;
            total_received += current_size;
            old_size = total_size;
            total_size += 1024;

            char* temp_response = malloc(total_size);
            memcpy(temp_response, final_response, old_size);
            free(final_response);
            final_response = temp_response;
    }

    fprintf(fptr, "%s\n", final_response);
    free(final_response);

    fclose(fptr);
    close(sock_fd);

    printf("\n[SUCCESS] home page received from the server and saved to file\n");

    return 0;
}