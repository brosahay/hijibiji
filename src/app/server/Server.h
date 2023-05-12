#include <stdio.h>

#include <HBStatus.h>

#define HB_DEFAULT_HOST "127.0.0.1"
#define HB_DEFAULT_PORT 59090
#define HB_DEBUG        1
#define HB_CM_DEBUG     1
#define HB_BLK_SIZE     4096

using namespace std;

namespace hijibiji {
class Server {
public:
  hb_status_t GetAddress(uint8_t *address);
  hb_status_t SetAddress(const uint8_t *address);
  hb_status_t GetPort(uint8_t *port);
  hb_status_t SetPort(uint8_t port);
  hb_status_t GetServiceName(uint8_t *name);
  hb_status_t SetServiceName(const uint8_t *name);
  hb_status_t GetMaxConnections(uint16_t *limit);
  hb_status_t SetMaxConnections(uint16_t limit);
  hb_status_t Start();
  hb_status_t Stop();
  hb_status_t Pause();
  hb_status_t Accept();
  hb_status_t AdvertiseService(uint16_t timeout);
};
} // namespace hijibiji