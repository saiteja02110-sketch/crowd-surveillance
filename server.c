#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SIZE 10
#define THRESHOLD 100
#define BUFFER_SIZE 4096

int queue[SIZE];
int front = -1, rear = -1;

int isEmpty() { return (front == -1); }
int isFull() { return ((rear + 1) % SIZE == front); }

void enqueue(int crowdCount) {
    if (isFull()) { front = (front + 1) % SIZE; } 
    if (front == -1) front = 0;
    rear = (rear + 1) % SIZE;
    queue[rear] = crowdCount;
}

int getLatestEvent() {
    if (isEmpty()) return 0;
    return queue[rear];
}

void get_telemetry_json(char *buffer) {
    int current_count = (rand() % 70) + 50; 
    enqueue(current_count);
    
    int latest = getLatestEvent();
    char *status = (latest > THRESHOLD) ? "ERRATIC SURGE ALERT" : "CROWD FLOW STABLE";
    char *density = (latest > THRESHOLD) ? "CRITICAL HIGH DENSITY" : "MODERATE DENSITY";
    double entropy = 2.1 + ((double)rand() / RAND_MAX) * 2.5;

    sprintf(buffer, 
            "{\"status\": \"%s\", \"count\": %d, \"density\": \"%s\", \"entropy\": %.2f}",
            status, latest, density, entropy);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    read(client_socket, buffer, BUFFER_SIZE - 1);
    
    if (strstr(buffer, "GET / ") != NULL || strstr(buffer, "GET /index.html") != NULL) {
        FILE *html_file = fopen("index.html", "r");
        if (html_file == NULL) {
            char *msg = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
            write(client_socket, msg, strlen(msg));
            return;
        }
        char response_header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
        write(client_socket, response_header, strlen(response_header));
        
        char file_buffer[BUFFER_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), html_file)) > 0) {
            write(client_socket, file_buffer, bytes_read);
        }
        fclose(html_file);
    } 
    else if (strstr(buffer, "GET /system_metrics") != NULL) {
        char json_data[512];
        get_telemetry_json(json_data);
        
        char response[1024];
        sprintf(response, 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Content-Length: %ld\r\n\r\n%s", 
                strlen(json_data), json_data);
        write(client_socket, response, strlen(response));
    }
    close(client_socket);
}

int main() {
    srand(time(NULL));
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    char *port_env = getenv("PORT");
    int port = port_env ? atoi(port_env) : 8080;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept error");
            continue;
        }
        handle_client(client_socket);
    }
    close(server_fd);
    return 0;
}
