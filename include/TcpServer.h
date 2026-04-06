#pragma once

#include <expected>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <system_error>

struct systemError {
  systemError(const std::string_view sc);
  std::error_code e;
  std::string sys_call;
};

class clientHandle;
class tcpListener {
  int fd;
  struct sockaddr_in addr;

  tcpListener(uint32_t ipv4_Address = 0, uint16_t port = 1234);

public:
  tcpListener(tcpListener &&other) noexcept;
  static std::expected<tcpListener, systemError>
  create(uint32_t ipv4_Address = 0, uint16_t port = 1234,
         int max_backlog = SOMAXCONN);
  std::expected<clientHandle, systemError> accept();
  ~tcpListener();
};

class clientHandle {
  int fd;
  struct sockaddr_in addr;
  socklen_t addr_len;
  clientHandle();
  std::expected<void, systemError> read(char *buf, size_t n);
  std::expected<void, systemError> write(const char *buf, size_t n);

public:
  clientHandle(clientHandle &&other) noexcept;
  friend tcpListener;
  std::expected<std::string, systemError> read();
  std::expected<void, systemError> write(std::string_view);
  ~clientHandle();
};