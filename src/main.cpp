#include <TcpServer.h>
#include <csignal>
#include <cstdlib>
#include <print>

int main() {
  // Prevent the server from crashing if we write to a closed socket
  std::signal(SIGPIPE, SIG_IGN);

  auto res = tcpListener::create(0, 1234, 10);
  if (!res) {
    std::print(stderr, "Failed to create server.\n");
    exit(EXIT_FAILURE);
  }

  std::print(stderr, "Server listening on port 1234...\n");

  while (true) {
    auto client = res->accept();
    if (!client) {
      std::print(stderr, "Failed to accept client.\n");
      continue;
    }

    std::print(stderr, "Client connected!\n");

    // Inner loop: Keep reading and writing to this specific client
    while (true) {
      auto readval = client->read();

      // 1. Check for success FIRST
      if (!readval) {
        std::print(stderr, "Client disconnected or read error.\n");
        break; // Break inner loop to accept a new client
      }

      // 2. Safe to dereference now
      std::print(stderr, "client says: {}\n", *readval);

      // 3. Respond immediately to the message
      auto writeval = client->write("Message received!");
      if (!writeval) {
        std::print(stderr, "Failed to send acknowledgment.\n");
        break; // Stop trying to talk to a broken connection
      }
    }
  }

  return 0;
}