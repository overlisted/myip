#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
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

    int v6only = 1;
    setsockopt(sock_fd, SOL_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only));

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
    char buf[256];
    int peer_fd;
    struct sockaddr peer_addr;
    socklen_t peer_addr_len;

    if (version == 1) {
      peer_addr_len = sizeof(struct sockaddr_in);
    }

    if (version == 2) {
      peer_addr_len = sizeof(struct sockaddr_in6);
    }

    memset(buf, '\0', sizeof(buf));
    memset(&peer_addr, '\0', peer_addr_len);

    peer_fd = accept(sock_fd, &peer_addr, &peer_addr_len);

    if (peer_fd == -1) {
      close(peer_fd);
      printf("%s failed to accept\n", name);
      return 1;
    }

    if (version == 1) {
      struct sockaddr_in *addr_in = (struct sockaddr_in *)&peer_addr;
      inet_ntop(AF_INET, &addr_in->sin_addr, buf, peer_addr_len);

      uint32_t val = addr_in->sin_addr.s_addr;
      printf("\nlength\t%d\nnumeric\t%d\nformat\t%s\n", peer_addr_len,
             ntohl(val), buf);
    }

    if (version == 2) {
      struct sockaddr_in6 *addr_in = (struct sockaddr_in6 *)&peer_addr;
      inet_ntop(AF_INET6, &addr_in->sin6_addr, buf, peer_addr_len);

      uint32_t *val = addr_in->sin6_addr.s6_addr32;
      printf("\nlength\t%d\nnumeric\t%d %d %d %d\nformat\t%s\n", peer_addr_len,
             ntohl(val[0]), ntohl(val[1]), ntohl(val[2]), ntohl(val[3]), buf);
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

  // for systemd
  setlinebuf(stdout);

  switch (argc) {
  case 2:
    if (fork() != 0) {
      return run("v4", argv[1], 1);
    } else {
      return run("v6", argv[1], 2);
    }
  case 3:
    if (fork() != 0) {
      return run("v4", argv[1], 1);
    } else {
      return run("v6", argv[2], 2);
    }
  default:
    puts("usage: \tmyip <combined port>\n\tmyip <v4 port> <v6 port>");
    return 1;
  }
}
