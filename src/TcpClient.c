#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> // by the same person who created C and unix
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    return EXIT_FAILURE;
  }

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234);
  addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1
  int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv) {
    return EXIT_FAILURE;
  }

  char msg[] = "hello";
  write(fd, msg, strlen(msg));

  char rbuf[64] = {};
  ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
  if (n < 0) {
    return EXIT_FAILURE;
  }
  printf("server says: %s\n", rbuf);
  close(fd);
}