/*
 * tcp_server.c
 * This is a tcp server which listents on a specified port and sends a message when a connection is accepted
 */
#include <stdio.h>
#include <string.h> // memset()
#include <sys/socket.h> // socket(), connect(), recv()
#include <unistd.h> // close()
#include <netinet/in.h> // htons(), INADDR_ANY, struct sockaddr_in
#include <arpa/inet.h> // inet_addr()
#include <errno.h>

//function declarations
int send_ok_html(int client_socket, char* http_body) ; //Sends an OK response to client
int deduce_connection_purpose(char *server_response); //Determine what GET request is for
void display_main_html_page(int client_socket) ; //Send main html page
int send_error_html(int client_socket, char* error_message); //Sends a html page with the error message
int read_in_html_page(FILE* html_file, char *buffer, size_t max_buffer_size); //reads in a html file

//Global varriables 
const int MAX_SOCKET_QUEUE_LENGTH = 1;

int main() {


    //create server socket
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    //define the servers address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = inet_addr("0.0.0.0");

    //bind
    if ( bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) != 0 ) {

        printf("Error(no.): %d",errno);
        perror("Binding to socket");
        return -1;
    }
    //listen
    if ( listen(server_socket,MAX_SOCKET_QUEUE_LENGTH) != 0 ) {

        printf("Error(no.): %d",errno);
        perror("Binding to socket");
        return -1;
    }


    //accept conections
    int client_socket;
    char server_response[256];
    while(1) {

        //wait for a connection
        client_socket = accept(server_socket, NULL, NULL);

        recv(client_socket, &server_response, sizeof(server_response), 0); //0 = Option flags paramater
        
        int client_connection_purpose = deduce_connection_purpose(server_response);

        switch (client_connection_purpose) {

            default: 
                display_main_html_page(client_socket);
        }


        printf("The server sent the data:\n%s\n",server_response);
    }

    close(client_socket);
    close(server_socket);
    return 0;
    
}


/*
 * Determine what GET request is for
 */
int deduce_connection_purpose(char *server_response) {

    printf("INFO: Entered deduce_connection_purpose\n");
    return 0; //Always main html page for now
}


/*
 * read in the requested html page to a given buffer
 */
int read_in_html_page(FILE* html_file, char* buffer, size_t max_buffer_size) {
    printf("INFO: Reading in html file");
    memset(buffer,0,max_buffer_size);


    size_t current_buffer_size = 0;
    char local_buffer[1024];
    while (fgets(local_buffer, sizeof(local_buffer), html_file) != NULL) {

        //handle buffer overflow
        if ( (current_buffer_size += strlen(local_buffer)) >= max_buffer_size - 1) { // -1 for \0
            printf("ERROR: Reading in html file size is greater than buffer i.e the file requested by client was to big"); 
            strcpy(buffer, "<!DOCTYPE html> <html lang=\"en\"> <head> <meta charset=\"UTF-8\"> <title></title> </head> <body> <h1>Error fetching page</h1> <p>The html file you requested was to large</p> </body> </html>");
            return -1;
        }

        //printf("INFO: %s",local_buffer); //print file content
        strncat(buffer, local_buffer, max_buffer_size);
    }

    printf("INFO: File sucessfully read in");
    return 0;
}


/*
 * Send main html page
 */
void display_main_html_page(int client_socket){

    //read in main html page
    FILE *html_data;
    html_data = fopen("index.html","r");

    if (html_data == NULL) {
        char* error_msg = strerror(errno);
        
        // Print detailed error information
        fprintf(stderr, "Error: opening file index.html in mode 'r':\n");
        fprintf(stderr, "Error %d: %s\n", errno, error_msg);
        
        // Handle specific error cases
        char error_message[240];
        switch (errno) {
            case ENOENT:
                printf("File not found\n");
                strcpy(error_message, "<!DOCTYPE html> <html lang=\"en\"> <head> <meta charset=\"UTF-8\"> <title></title> </head> <body> <h1>Error requesting page</h1><h2>File not found</h2> </body> </html>");
                break;
            case EACCES:
                printf("Permission denied\n");
                strcpy(error_message, "<!DOCTYPE html> <html lang=\"en\"> <head> <meta charset=\"UTF-8\"> <title></title> </head> <body> <h1>Error requesting page</h1><h2>Permission denied</h2> </body> </html>");
                break;
            case EINVAL:
                printf("Invalid mode\n");
                strcpy(error_message, "<!DOCTYPE html> <html lang=\"en\"> <head> <meta charset=\"UTF-8\"> <title></title> </head> <body> <h1>Error requesting page</h1><h2>Invalid mode</h2> </body> </html>");
                break;
            default:
                printf("Unexpected error occurred\n");
                strcpy(error_message, "<!DOCTYPE html> <html lang=\"en\"> <head> <meta charset=\"UTF-8\"> <title></title> </head> <body> <h1>Error requesting page</h1><h2>Unexpected Error</h2> </body> </html>");
        }

        //Send Error message to client 
        printf("ERROR: Sending error message\n");
        send_error_html(client_socket, error_message);
        return;
    }


    char http_body[1024];
    read_in_html_page(html_data, http_body, sizeof(http_body));


    printf("INFO: Sending OK message\n");
    send_ok_html(client_socket, http_body) ;
}


/*
 * Sends an OK response to client
 */
int send_ok_html(int client_socket, char* http_body) {

    size_t http_body_length = strlen(http_body);

    char http_header[4096];
    sprintf(http_header, 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s", 
        http_body_length, http_body);
    send(client_socket, http_header, sizeof(http_header),0);

    return 0;
}


/*
 * Sends a html page with the error message
 */
int send_error_html(int client_socket, char* error_message) {


    size_t http_body_length = strlen(error_message);

    char http_header[4096];
    sprintf(http_header, 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s", 
        http_body_length, error_message);
    send(client_socket, http_header, sizeof(http_header),0);

    return 0;
}
