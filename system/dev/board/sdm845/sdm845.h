#include <ddk/device.h>
#include <ddk/driver.h>
#include <ddk/protocol/platform-bus.h>

typedef struct {
    platform_bus_protocol_t pbus;
    zx_device_t* parent;
} sdm845_t;
