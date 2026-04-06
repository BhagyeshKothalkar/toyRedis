#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

static int32_t netread(int fd, char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = read(fd, buf, n);
    if (rv < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    } else if (rv == 0) {
      return -1;
    }
    assert(size_t(rv) <= n);
    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

static int32_t netwrite(int fd, const char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = write(fd, buf, n);
    if (rv < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    } else if (rv == 0 && n > 0) {
      return -1;
    }
    assert((size_t)rv <= n);
    n -= size_t(rv);
    buf += (size_t)rv;
  }
  return 0;
}

const size_t k_max = 4096;
// protocol: first read the 4 bytes header giving the size of the message and
// then go ahead to read that many bytes;
int32_t one_request(int connfd) {
  char rbuf[4 + k_max];
  errno = 0;
  int32_t err = netread(connfd, rbuf, 4);
  if (err) {
    return err;
  }
  uint32_t len = 0;
  memcpy(&len, rbuf, 4);
  if (len > k_max) {
    return -1;
  }
  err = netread(connfd, &rbuf[4], len);
  if (err)
    return -1;
  printf("client says: %.*s\n", len, &rbuf[4]);
  const char reply[] = "world";
  char wbuf[4 + sizeof(reply)];
  len = (uint32_t)strlen(reply);
  memcpy(wbuf, &len, 4);
  memcpy(&wbuf[4], reply, len);
  return netwrite(connfd, wbuf, 4 + len);
}

int main() {
  int fd = socket(AF_INET, SOCK_STREAM,
                  0); // create a file descriptor for the socket

  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val,
             sizeof(val));      // set variables for the socket
  struct sockaddr_in addr = {}; // define the listening socket on the server
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);
  addr.sin_addr.s_addr = htonl(0);

  int rv = bind(fd, (const struct sockaddr *)(&addr),
                sizeof(addr)); //  here rv stands for return value
  if (rv) {
    fprintf(stderr, "bind\n");
    exit(EXIT_FAILURE);
  }
  rv = listen(fd, SOMAXCONN); // create the socket and start listening
  if (rv) {
    fprintf(stderr, "listen\n");
    exit(EXIT_FAILURE);
  }
  // accept and process connections
  while (1) {
    // processing the next client
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(
        client_addr); // setup the client addr where the listened data is kept
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    if (connfd < 0) {
      continue; // error
    }
    while (true) {
      int32_t err = one_request(connfd);
      if (err)
        break;
    }
    printf("exiting");
    close(connfd);
  }
}
