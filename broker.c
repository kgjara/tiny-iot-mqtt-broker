#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 20
#define MAX_SUBSCRIPTIONS 50
#define BUFFER_SIZE 1024

typedef struct {
  int socket;
  int active;
} Client;

typedef struct {
  char topic[256];
  int client_socket;
} Subscription;

Client clients[MAX_CLIENTS];
Subscription subscriptions[MAX_SUBSCRIPTIONS];
int num_subscriptions = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
  int client_sock = *(int *)arg;
  char buffer[BUFFER_SIZE];
  int bytes;

  while (1) {
    bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
      printf("[BROKER] Cliente desconectado\n");
      break;
    }

    buffer[bytes] = '\0';
    char *line = strtok(buffer, "\n");
    
    while (line) {
      if (strncmp(line, "PUB ", 4) == 0) {
        // Publish: PUB <topic> <payload>
        char topic[256], payload[512];
        sscanf(line, "PUB %s %[^\n]", topic, payload);
        printf("[BROKER] PUB en %s: %s\n", topic, payload);

        // Enviar a todos los suscritos
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_subscriptions; i++) {
          if (strcmp(subscriptions[i].topic, topic) == 0) {
            char msg[512];
            snprintf(msg, sizeof(msg), "%s:%s\n", topic, payload);
            send(subscriptions[i].client_socket, msg, strlen(msg), 0);
          }
        }
        pthread_mutex_unlock(&mutex);

      } else if (strncmp(line, "SUB ", 4) == 0) {
        // Subscribe: SUB <topic>
        char topic[256];
        sscanf(line, "SUB %s", topic);
        printf("[BROKER] SUB a %s\n", topic);

        pthread_mutex_lock(&mutex);
        if (num_subscriptions < MAX_SUBSCRIPTIONS) {
          strcpy(subscriptions[num_subscriptions].topic, topic);
          subscriptions[num_subscriptions].client_socket = client_sock;
          num_subscriptions++;
        }
        pthread_mutex_unlock(&mutex);
      }

      line = strtok(NULL, "\n");
    }
  }

  close(client_sock);
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  int port = (argc > 1) ? atoi(argv[1]) : 1883;
  int server_sock, client_sock;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len;
  pthread_t thread_id;

  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    perror("socket");
    return 1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind");
    return 1;
  }

  listen(server_sock, 5);
  printf("[BROKER] Escuchando en puerto %d\n", port);

  while (1) {
    client_len = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
    if (client_sock < 0) {
      perror("accept");
      continue;
    }

    printf("[BROKER] Nuevo cliente conectado\n");
    int *sock_ptr = malloc(sizeof(int));
    *sock_ptr = client_sock;
    pthread_create(&thread_id, NULL, handle_client, sock_ptr);
  }

  close(server_sock);
  return 0;
}
