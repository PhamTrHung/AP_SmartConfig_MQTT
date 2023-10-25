#ifndef __GATT_SERVER_H
#define __GATT_SERVER_H

#include "stdint.h"

typedef void (*gatt_server_handle_callback)(uint8_t* data, uint16_t length);


void gatt_server_init(void);
void gatts_read_event(uint8_t* data, uint16_t length);
void gatt_server_set_write_event_callback(void* cb);
void gatts_notify_event(uint8_t* data, uint16_t length);
void gatt_server_stop(void);

#endif
