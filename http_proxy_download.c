/* f20180205@hyderabad.bits-pilani.ac.in Sukrit Kapil */

/* Program to download main page and logo of a website through socket connection to a proxy */
/* ... */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

char* base64Encoder(char input_str[], int len_str) {
    // Character set of base64 encoding scheme 
    char char_set[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; 
      
    // Resultant string 
    char *res_str = (char *) malloc(1000 * sizeof(char)); 
      
    int index, no_of_bits = 0, padding = 0, val = 0, count = 0, temp; 
    int i, j, k = 0; 
    
    for (i = 0; i < len_str; i += 3) { 
        val = 0, count = 0, no_of_bits = 0;

        for (j = i; j < len_str && j <= i + 2; j++) { 
            val = val << 8;  
            val = val | input_str[j];
            count++; 
        } 

        no_of_bits = count * 8;
        padding = no_of_bits % 3;  

        while (no_of_bits != 0) { 
            if (no_of_bits >= 6) { 
                temp = no_of_bits - 6;
                index = (val >> temp) & 63;  
                no_of_bits -= 6;          
            } else { 
                temp = 6 - no_of_bits;
                index = (val << temp) & 63;  
                no_of_bits = 0; 
            }
            
            res_str[k++] = char_set[index]; 
        } 
    } 
  
    // padding is done here 
    for (i = 1; i <= padding; i++) { 
        res_str[k++] = '='; 
    } 
    res_str[k] = '\0'; 
    
    return res_str;
}

// this method searches for the first <img> tag in the main page and returns the logo
void download_logo(char* logo_name, char* proxy_server, char* proxy_port, char* username, char* password, char* auth_str, char* home_page, char* website) {
    char search_string[5000] = "<P><IMG";
    char word[5000];
    char logo_url[5000];

    FILE* fptr;
    fptr = fopen(home_page, "r+b");

    while (fscanf(fptr, "%7s", word) != EOF) {
        if(strcmp(search_string, word) == 0) {
            fscanf(fptr, "%1000s", word);
            strncpy(logo_url, word+5, (int)(strlen(word) - 7));
            fclose(fptr);
            break;
        }
    }

    int sock_fd;
    struct sockaddr_in server;

    fptr = fopen(logo_name, "w+b");

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_addr.s_addr = inet_addr(proxy_server);
    server.sin_port = htons(atoi(proxy_port));
    server.sin_family = AF_INET;

    connect(sock_fd, (struct sockaddr *) &server, sizeof(server));

    char message[8192] = "GET http://";
    strcat(message, website);
    strcat(message, "/");
    strcat(message, logo_url);
    strcat(message, " HTTP/1.1\r\nHost: ");
    strcat(message, website);
    strcat(message, "\r\nProxy-Authorization: Basic ");
    strcat(message, auth_str);
    strcat(message, "\r\nConnection: close\r\n\r\n");

    send(sock_fd, message, strlen(message), 0);

    printf("\n[REQUEST HEADER]\n%s", message);

    char server_response[1024];
    int recv_length;
    int header_flag = 1;

    printf("[FETCHING] logo is being downloaded...\n");

    while((recv_length = recv(sock_fd, server_response, 1024, 0)) > 0) {
        if(header_flag == 1) {
            char* after_remove_header = strstr(server_response, "\r\n\r\n");
            int length = recv_length - (after_remove_header - server_response + 4);
            after_remove_header += 4;
            fwrite(after_remove_header, 1, length, fptr);
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

    // convert the username:password to base64 encoded string
    char cred[8192];

    strcat(cred, username);
    strcat(cred, ":");
    strcat(cred, password);

    char* auth_str = base64Encoder(cred, strlen(cred));
    
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
    printf("\n[SUCCESS] request sent to the server\n\n[REQUEST HEADER]\n%s", message);

    char server_response[1024];
    int recv_length;
    int header_flag = 1;

    while((recv_length = recv(sock_fd, server_response, 1024, 0)) > 0) {
        if(header_flag == 1) {
            if(server_response[9] == '3' && server_response[10] == '0') {
                
                char* ptr = strstr(server_response, "Location: ");
                ptr += 10;
                if(strstr(ptr, "http://") != NULL) {
                    ptr += 7;
                }
                char *secptr = strstr(ptr, "\n");
                int length = secptr - ptr;
                char final_url[5000];
                strncpy(final_url, ptr, length-1);
                printf("\n[RESPONSE] Status Code: Redirecting to %s ..\n", final_url);

                close(sock_fd);

                sock_fd = socket(AF_INET, SOCK_STREAM, 0);

                // request header
                char message[8192] = "GET http://";
                strcat(message, final_url);
                strcat(message, " HTTP/1.1\r\nHost: ");
                strcat(message, website);
                strcat(message, "\r\nProxy-Authorization: Basic ");
                strcat(message, auth_str);
                strcat(message, "\r\nConnection: close\r\n\r\n");

                if(connect(sock_fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
                    printf("\n[ERROR] could not connect to server\n");
                    exit(1);
                }

                if(send(sock_fd, message, strlen(message), 0) < 0) {
                    printf("\n[ERROR] could not send request to server\n");
                    return -1;
                }
                printf("\n[SUCCESS] request sent to the server\n\n[REQUEST HEADER]\n%s", message);

                continue;
            }
            printf("\n[RESPONSE] Status Code: OK\n");
            printf("\n[FETCHING] home page is being fetched...\n");
            char* after_remove_header = strstr(server_response, "\r\n\r\n");
            int length = recv_length - (after_remove_header - server_response + 4);
            after_remove_header += 4;
            fwrite(after_remove_header, 1, length, fptr);
        }
        else fwrite(server_response, 1, recv_length, fptr);
        fflush(fptr);
        memset(server_response, 0, 1024);
        header_flag = 0;
    }


    printf("\n[SUCCESS] home page received from the server and saved to file\n");

    if(logo != NULL && strcmp(website, "info.in2p3.fr") == 0) {
        download_logo(logo, proxy_server, proxy_port, username, password, auth_str, home_page, website);
        printf("\n[SUCCESS] logo downloaded successfully\n");
    } else {
        printf("\n[INFO] logo will not be downloaded!\n");
    }

    fclose(fptr);

    return 0;
}