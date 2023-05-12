#include <stdio.h>

#include <HBStatus.h>

#define HB_BUFFER_SIZE_BYTES 256

using namespace std;

namespace hijibiji {
class Client {
public:
  hb_status_t Connect(const uint8_t *addr, uint32_t port);
  hb_status_t Send(uint8_t *data, size_t len);
  hb_status_t Cancel();
};
} // namespace hijibiji