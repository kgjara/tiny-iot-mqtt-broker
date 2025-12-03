#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int broker_sock = -1;

void *handle_publisher(void *arg) {
  int pub_sock = *(int *)arg;
  char buffer[BUFFER_SIZE];
  int bytes;

  while (1) {
    bytes = recv(pub_sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
      printf("[GATEWAY] Publisher desconectado\n");
      break;
    }

    buffer[bytes] = '\0';
    printf("[GATEWAY] Recibido: %s", buffer);

    // Reenviar al broker
    if (send(broker_sock, buffer, strlen(buffer), 0) < 0) {
      printf("[GATEWAY] Error enviando al broker\n");
      break;
    }
  }

  close(pub_sock);
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    printf("Uso: %s <broker_ip> <broker_port> <gateway_port>\n", argv[0]);
    return 1;
  }

  const char *broker_ip = argv[1];
  int broker_port = atoi(argv[2]);
  int gateway_port = atoi(argv[3]);

  // Conectar al broker
  broker_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (broker_sock < 0) {
    perror("socket");
    return 1;
  }

  struct sockaddr_in broker_addr;
  broker_addr.sin_family = AF_INET;
  broker_addr.sin_port = htons(broker_port);
  broker_addr.sin_addr.s_addr = inet_addr(broker_ip);

  if (connect(broker_sock, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
    perror("connect");
    return 1;
  }

  printf("[GATEWAY] Conectado al broker en %s:%d\n", broker_ip, broker_port);

  // Crear servidor para publishers
  int server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    perror("socket");
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(gateway_port);

  if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind");
    return 1;
  }

  listen(server_sock, 5);
  printf("[GATEWAY] Escuchando publishers en puerto %d\n", gateway_port);

  struct sockaddr_in client_addr;
  socklen_t client_len;
  int client_sock;
  pthread_t thread_id;

  while (1) {
    client_len = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
    if (client_sock < 0) {
      perror("accept");
      continue;
    }

    printf("[GATEWAY] Publisher conectado\n");
    int *sock_ptr = malloc(sizeof(int));
    *sock_ptr = client_sock;
    pthread_create(&thread_id, NULL, handle_publisher, sock_ptr);
  }

  close(server_sock);
  close(broker_sock);
  return 0;
}
