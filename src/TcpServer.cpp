#include <TcpServer.h>
#include <cerrno>
#include <cstring>
#include <expected>
#include <netinet/in.h>
#include <print>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>

template <typename T> using expected = std::expected<T, systemError>;
using std::unexpected;

systemError::systemError(std::string_view sc)
    : e(std::error_code(errno, std::generic_category())), sys_call(sc) {
  std::print(stderr, "error {}:\n{}\n when calling {}", e.value(), e.message(),
             sc);
}

clientHandle::clientHandle() : fd(-1), addr_len(sizeof(sockaddr_in)) {}
clientHandle::clientHandle(clientHandle &&other) noexcept
    : fd(other.fd), addr(other.addr), addr_len((other.addr_len)) {
  other.fd = -1;
}

// TODO: check memcpy result
expected<void> clientHandle::read(char *buf, size_t n) {
  while (n > 0) {
    errno = 0;
    ssize_t ret = ::read(this->fd, buf, n);
    if (ret < 0) {
      if (errno == EINTR)
        continue;
      return unexpected(systemError("read"));
    } else if (ret == 0) {
      return unexpected(systemError("read"));
    }
    size_t len = static_cast<size_t>(ret);
    if (len > n) {
      errno = -1;
      return unexpected(systemError("read"));
    }
    n -= len;
    buf += len;
  }
  return {};
}

/*
protocol: first four bytes give the length of the string incoming.
next, the string of that length follows
*/
expected<std::string> clientHandle::read() {
  char rbuf[4];
  return read(rbuf, 4).and_then([&rbuf, this](void) {
    uint32_t len;
    std::memcpy(&len, rbuf, 4);
    std::string s(len, 0);
    return this->read(s.data(), len).transform([s = std::move(s)]() mutable {
      return std::move(s);
    });
  });
}

expected<void> clientHandle::write(const char *buf, size_t n) {
  while (n > 0) {
    errno = 0;
    ssize_t ret = ::write(this->fd, buf, n);
    if (ret < 0) {
      if (errno == EINTR)
        continue;
      return unexpected(systemError("write"));
    } else if (ret == 0 && n > 0) {
      return unexpected(systemError("write"));
    }
    size_t len = static_cast<size_t>(ret);
    if (len > n) {
      errno = -1;
      return unexpected(systemError("write"));
    }
    n -= len;
    buf += len;
  }
  return {};
}

expected<void> clientHandle::write(std::string_view msg) {
  uint32_t len = msg.size();
  return write(reinterpret_cast<const char *>(&len), 4)
      .and_then([msg, this](void) { return write(msg.data(), msg.size()); });
}

clientHandle::~clientHandle() {
  if (this->fd != -1)
    ::close(this->fd);
}

tcpListener::tcpListener(uint32_t ipv4_address, uint16_t port)
    : fd(-1), addr(AF_INET, htons(port), in_addr(htonl(ipv4_address)), {}) {}
tcpListener::tcpListener(tcpListener &&other) noexcept
    : fd(other.fd), addr(other.addr) {
  other.fd = -1;
}

expected<tcpListener> tcpListener::create(uint32_t ipv4_address, uint16_t port,
                                          int max_backlog) {
  tcpListener t(ipv4_address, port);
  auto check = [](int rv, const char *msg) -> expected<int> {
    if (rv >= 0)
      return rv;
    else
      return unexpected(systemError(msg));
  };
  expected<int> ret =
      check(::socket(AF_INET, SOCK_STREAM, 0), "socket")
          .and_then([&t, &check](int fd) -> expected<int> {
            t.fd = fd;
            int optval = SO_REUSEADDR;
            return check(::setsockopt(t.fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                                      sizeof(optval)),
                         "setsockopt");
          })
          .and_then([&t, &check](int /* setsockopt_ret */) -> expected<int> {
            return check(::bind(t.fd,
                                reinterpret_cast<const sockaddr *>(&t.addr),
                                sizeof(t.addr)),
                         "bind");
          })
          .and_then(
              [&t, &check, max_backlog](int /* bind_ret */) -> expected<int> {
                return check(::listen(t.fd, max_backlog), "listen");
              });
  if (!ret) {
    return unexpected(ret.error());
  }
  return t;
}

expected<clientHandle> tcpListener::accept() {
  clientHandle client;
  client.addr_len = sizeof(client.addr);
  int ret = ::accept(this->fd, reinterpret_cast<sockaddr *>(&client.addr),
                     &client.addr_len);

  if (ret < 0) {
    return unexpected(systemError("accept"));
  } else {
    client.fd = ret;
    return client;
  }
}

tcpListener::~tcpListener() {
  if (this->fd != -1) {
    ::close(this->fd);
  }
}