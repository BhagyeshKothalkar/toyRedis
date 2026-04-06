#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> // by the same person who created C and unix
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

const size_t k_max_msg = 4096;

static int32_t netread(int fd, char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = read(fd, buf, n);
    if (rv <= 0) {
      return -1;
    }
    assert((size_t)rv <= n);
    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

static int32_t netwrite(int fd, const char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = write(fd, buf, n);
    if (rv <= 0) {
      return -1;
    }
    assert((size_t)rv <= n);
    n -= size_t(rv);
    buf += (size_t)rv;
  }
  return 0;
}

static int32_t query(int fd, const char *text) {
  uint32_t len = (uint32_t)strlen(text);
  if (len > k_max_msg) {
    return -1;
  }
  // send request
  char wbuf[4 + k_max_msg];
  memcpy(wbuf, &len, 4); // assume little endian
  memcpy(&wbuf[4], text, len);
  if (int32_t err = netwrite(fd, wbuf, 4 + len)) {
    return err;
  }
  // 4 bytes header
  char rbuf[4 + k_max_msg];
  errno = 0;
  int32_t err = netread(fd, rbuf, 4);
  if (err) {
    return err;
  }
  memcpy(&len, rbuf, 4); // assume little endian
  if (len > k_max_msg) {
    return -1;
  }
  // reply body
  err = netread(fd, &rbuf[4], len);
  if (err) {
    return err;
  }
  // do something
  printf("server says: %.*s\n", len, &rbuf[4]);
  return 0;
}

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    return -1;
    // die("socket()");
  }
  // code omitted ...
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;

  addr.sin_port = htons(1234);
  addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1

  int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv) {
    fprintf(stderr, "error in connecting client");
    exit(EXIT_FAILURE);
  }

  // send multiple requests
  int32_t err = query(fd, "hello1");
  if (err) {
    goto L_DONE;
  }
  err = query(fd, "hello2");
  if (err) {
    goto L_DONE;
  }
L_DONE:
  close(fd);
  return 0;
}