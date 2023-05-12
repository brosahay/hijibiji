#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <HBStatus.h>
#include <Server.h>

using namespace std;

namespace hijibiji {
class Server {
private:
  int sockfd;               // socket descriptor
  struct sockaddr_in svr;   // server details
  uint16_t max_connections; // maximum number of concurrect connections
  char svc_name[256];       // service name
  void *requestHandler;     // handler for incoming requests
  // DISCUSS: does this require a singleton?

public:
  Server()
  {
    memset(&svc_name, 0, sizeof(svc_name));
    max_connections = SOMAXCONN;

    memset(&svr, 0, sizeof(svr));
    svr.sin_family      = AF_INET;
    svr.sin_addr.s_addr = inet_addr(HB_DEFAULT_HOST.c_str());
    svr.sin_port        = HB_DEFAULT_PORT;
  }

  hb_status_t GetAddress(uint8_t *address, size_t len)
  {
    // memcpy(address, svr.sin_addr.s_addr, sizeof(svr.sin_addr.s_addr));
    return HB_SUCCESS;
  }

  hb_status_t SetAddress(const uint8_t *address, size_t len)
  {
    svr.sin_addr.s_addr = inet_addr(reinterpret_cast<const char *>(address));
    return HB_SUCCESS;
  }

  hb_status_t GetPort(uint16_t *port)
  {
    // TODO: implement
    return HB_SUCCESS;
  }

  hb_status_t SetPort(uint16_t port)
  {
    svr.sin_port = htons(port);
    return HB_SUCCESS;
  }

  hb_status_t GetServiceName(uint8_t *name, size_t len)
  {
    // TODO: implement
    return HB_SUCCESS;
  }

  hb_status_t SetServiceName(const uint8_t *name, size_t len)
  {
    memccpy(svc_name, name, len);
    return HB_SUCCESS;
  }

  hb_status_t GetMaxConnections(uint16_t *limit)
  {
    // TODO: implement
    return HB_SUCCESS;
  }

  hb_status_t SetMaxConnections(uint16_t limit)
  {
    if (limit > SOMAXCONN) {
      // TODO: print error
      // TODO: invalid argument error
      return HB_SOCKET_ERR;
    }
    max_connections = limit;
    return HB_SUCCESS;
  }

  hb_status_t Start()
  {
    int32_t status;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
      // TODO: print error
      return HB_SOCKET_ERR;
    }
    status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (status == -1) {
      // TODO: print error
      return HB_SOCKET_ERR;
    }
    bind(sockfd, (struct sockaddr *)&svr, sizeof(svr));
    if (0 != status) {
      // TODO: add error log
      return HB_SOCKET_ERR;
    }
    status = listen(sockfd, max_connections);
    if (0 != status) {
      // TODO: add error log
      return HB_SOCKET_ERR;
    }
    return HB_SUCCESS;
  }

  hb_status_t Stop()
  {
    // TODO: implement
    return HB_SUCCESS;
  }

  hb_status_t Pause()
  {
    // TODO: implement
    return HB_SUCCESS;
  }

  hb_status_t Accept(void *requestHandler)
  {
    struct sockaddr_in client;
    socklen_t client_size = sizeof(client);
    accept(sockfd, reinterpret_cast<struct sockaddr *>(&client), &client_size);
    // return &client;
    return HB_SUCCESS;
  }

  hb_status_t Respond()
  {
    return HB_SUCCESS;
  }

  hb_status_t AdvertiseService(uint16_t timeout)
  {
    return HB_SUCCESS;
  }
};
} // namespace hijibiji