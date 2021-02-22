/* f20180205@hyderabad.bits-pilani.ac.in Sukrit Kapil */

/* Program to download main page and logo of a website through socket connection to a proxy */
/* ... */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// this method searches for the first <img> tag in the main page and returns the logo
void download_logo(char* logo_name, char* proxy_server, char* proxy_port, char* username, char* password, char* auth_str, char* home_page) {
    char search_string[5000] = "<noscript><img";
    char word[5000];
    char logo_url[5000], host_name[5000];

    FILE* fptr;
    fptr = fopen(home_page, "r+b");

    while (fscanf(fptr, "%14s", word) != EOF) {
        if(strcmp(search_string, word) == 0) {
            fscanf(fptr, "%1000s", word);
            strncpy(logo_url, word+5, (int)(strlen(word) - 6));
            fclose(fptr);
            break;
        }
    }

    char* temp_host = strstr(logo_url, "//");
    char final_logo_url[5000] = "http://";
    temp_host += 2;
    strcat(final_logo_url, temp_host);

    for(int i = 0;i < strlen(temp_host); i++) {
        if(temp_host[i] == '/') break;
        host_name[i] = temp_host[i];
    }

    int sock_fd;
    struct sockaddr_in server;

    fptr = fopen(logo_name, "w+b");

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_addr.s_addr = inet_addr(proxy_server);
    server.sin_port = htons(atoi(proxy_port));
    server.sin_family = AF_INET;

    connect(sock_fd, (struct sockaddr *) &server, sizeof(server));

    char message[8192] = "GET ";
    strcat(message, final_logo_url);
    strcat(message, " HTTP/1.1\r\nHost: ");
    strcat(message, host_name);
    strcat(message, "\r\nProxy-Authorization: Basic ");
    strcat(message, auth_str);
    strcat(message, "\r\nConnection: close\r\n\r\n");

    send(sock_fd, message, strlen(message), 0);

    printf("\n[HEADER]\n%s", message);

    char server_response[1024];
    int recv_length;
    int header_flag = 1;

    printf("[FETCHING] logo is being downloaded...\n");

    while((recv_length = recv(sock_fd, server_response, 1024, 0)) > 0) {
        if(header_flag == 1) {
            char* after_remove_header = strstr(server_response, "\r\n\r\n");
            after_remove_header += 4;
            fwrite(after_remove_header, 1, strlen(after_remove_header)-4, fptr);
        }
        else fwrite(server_response, 1, recv_length, fptr);
        fflush(fptr);
        memset(server_response, 0, 1024);
        header_flag = 0;
    }

    close(sock_fd);
    fclose(fptr);

    return;
}

int main(int argc, char* argv[]) {
    // declare the required variables
    int sock_fd;
    struct sockaddr_in server;
    FILE* fptr;

    // assign all the command-line arguments
    char* website = argv[1];
    char* proxy_server = argv[2];
    char* proxy_port = argv[3];
    char* username = argv[4];
    char* password = argv[5];
    char* home_page = argv[6];
    char* logo = argv[7];

    fptr = fopen(home_page, "w+b");

    if(logo == NULL) printf("\n[INFO] logo will not be downloaded!\n");

    // convert the username:password to base64 encoded string
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
    printf("\n[SUCCESS] request sent to the server\n\n[HEADER]\n%s\n[FETCHING] home page is being fetched...\n", message);

    char server_response[1024];
    int recv_length;

    while((recv_length = recv(sock_fd, server_response, 1024, 0)) > 0) {
        fwrite(server_response, 1, recv_length, fptr);
        fflush(fptr);
        memset(server_response, 0, 1024);
    }


    printf("\n[SUCCESS] home page received from the server and saved to file\n");

    if(logo != NULL) {
        download_logo(logo, proxy_server, proxy_port, username, password, auth_str, home_page);
        printf("\n[SUCCESS] logo downloaded successfully\n");
    }

    fclose(fptr);

    return 0;
}