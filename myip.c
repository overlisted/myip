#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int sock_fd = 0;

void sigint_handler(int a) {
  if (sock_fd != 0) {
    close(sock_fd);
  }
}

int run(char *name, char *port, int version) {
  int bind_result;
  if (version == 1) {
    struct sockaddr_in serv_addr;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(port));

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    int reuseaddr = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
               sizeof(reuseaddr));

    bind_result =
        bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  }

  if (version == 2) {
    struct sockaddr_in6 serv_addr;

    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_addr = in6addr_any;
    serv_addr.sin6_port = htons(atoi(port));

    sock_fd = socket(AF_INET6, SOCK_STREAM, 0);

    int reuseaddr = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
               sizeof(reuseaddr));

    bind_result =
        bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  }

  if (bind_result == -1) {
    close(sock_fd);
    printf("%s failed to bind %d\n", name, errno);
    return 1;
  }

  listen(sock_fd, 64);

  printf("%s listening\n", name);
  while (1) {
    char buf[INET6_ADDRSTRLEN + 1];
    int peer_fd;
    struct sockaddr peer_addr;
    socklen_t peer_addr_len;

    peer_fd = accept(sock_fd, &peer_addr, &peer_addr_len);

    if (peer_fd == -1) {
      close(peer_fd);
      printf("%s failed to accept\n", name);
      return 1;
    }

    if (version == 1) {
      struct sockaddr_in v4_addr = *(struct sockaddr_in *)&peer_addr;

      inet_ntop(AF_INET, &v4_addr.sin_addr, buf, peer_addr_len);
    }

    if (version == 2) {
      struct sockaddr_in6 v6_addr = *(struct sockaddr_in6 *)&peer_addr;

      inet_ntop(AF_INET6, &v6_addr.sin6_addr, buf, peer_addr_len);
    }

    unsigned long len = strlen(buf);
    buf[len] = '\n';

    if (write(peer_fd, &buf, len + 1) == -1) {
      printf("%s write error\n", name);
    }

    close(peer_fd);
  }
}

int main(int argc, char **argv) {
  signal(SIGINT, sigint_handler);

  if (argc != 3) {
    puts("usage: myip <v4 port> <v6 port>");
    return 1;
  }

  if (fork() != 0) {
    return run("v4", argv[1], 1);
  } else {
    return run("v6", argv[2], 2);
  }
}
