#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <HBStatus.h>
#include <Client.h>

using namespace std;

namespace hijibiji {
class Client {
private:
  int sockfd;             // socket descriptor
  struct sockaddr_in svr; // destination server
  uint8_t *buffer = static_cast<uint8_t *>(calloc(HB_BUFFER_SIZE_BYTES, sizeof(uint8_t)));

public:
  hb_status_t Connect(const uint8_t *addr, uint32_t port)
  {
    int32_t status;
    memset(&svr, 0, sizeof(sockaddr_in));
    svr.sin_family      = AF_INET;
    svr.sin_addr.s_addr = inet_addr(reinterpret_cast<const char *>(addr));
    svr.sin_port        = htons(port);
    status              = connect(sockfd, reinterpret_cast<const sockaddr *>(&svr), sizeof(sockaddr));
    if (status != HB_SUCCESS) {
      // TODO: add error log
      return HB_SOCKET_ERR;
    }
    return HB_SUCCESS;
  }

  hb_status_t Send(uint8_t *data, size_t len)
  {
    int32_t status;
    status = send(sockfd, reinterpret_cast<void *>(data), len, 0);
    if (-1 == status) {
      // TODO: add error print
      return HB_SOCKET_ERR;
    }
    return HB_SUCCESS;
  }

  hb_status_t Receive()
  {
    int32_t status;
    status = recv(sockfd, buffer, HB_BUFFER_SIZE_BYTES, 0);
    if (-1 == status) {
      // TODO: add error print
      return HB_SOCKET_ERR;
    }
    // TODO: add debug print for response
    return HB_SUCCESS;
  }

  hb_status_t Cancel()
  {
    // TODO: implement later
    return HB_SUCCESS;
  }

  void DebugDetails()
  {
    // TODO: add debug logs for client
  }
};
} // namespace hijibiji