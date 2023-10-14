#ifndef SMARTCONFIG_H
#define SMARTCONFIG_H

typedef void (*smartconfig_successfully_callback)(void);

void smartconfig_start(void);
void smartconfig_done_set_callback(void* cb);

#endif
