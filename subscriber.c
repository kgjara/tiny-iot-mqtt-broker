#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Uso: %s <broker_ip> <broker_port>\n", argv[0]);
    return 1;
  }

  const char *broker_ip = argv[1];
  int broker_port = atoi(argv[2]);

  // Conectar al broker
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket");
    return 1;
  }

  struct sockaddr_in broker_addr;
  broker_addr.sin_family = AF_INET;
  broker_addr.sin_port = htons(broker_port);
  broker_addr.sin_addr.s_addr = inet_addr(broker_ip);

  if (connect(sock, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
    perror("connect");
    return 1;
  }

  printf("[SUBSCRIBER] Conectado al broker en %s:%d\n", broker_ip, broker_port);
  printf("[SUBSCRIBER] Escriba los tópicos a los que desea suscribirse (SUB <topic>)\n");

  char buffer[BUFFER_SIZE];
  char input[BUFFER_SIZE];

  // Thread para leer input del usuario
  while (1) {
    printf("\nComando> ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0; // Remover newline

    if (strlen(input) == 0) continue;

    // Enviar comando al broker
    if (send(sock, input, strlen(input), 0) < 0) {
      printf("Error enviando comando\n");
      break;
    }

    if (strncmp(input, "SUB", 3) == 0) {
      printf("[SUBSCRIBER] Suscrito. Esperando mensajes...\n");
      
      // Recibir mensajes del broker para este tópico
      while (1) {
        int bytes = recv(sock, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT);
        if (bytes > 0) {
          buffer[bytes] = '\0';
          printf("[MENSAJE] %s", buffer);
        } else if (bytes == 0) {
          printf("Desconectado del broker\n");
          goto finish;
        }
        usleep(100000); // 100ms
      }
    }
  }

finish:
  close(sock);
  return 0;
}
