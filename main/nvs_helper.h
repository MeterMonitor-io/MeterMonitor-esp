#ifndef NVS_HELPER_H
#define NVS_HELPER_H

#include <stdint.h>

#include <nvs.h>
#include <nvs_flash.h>

extern nvs_handle persistent_storage_handle;

bool init_nvs(void);
void read_value(const char* key, uint32_t* value);
bool write_value(const char* key, uint32_t value);
void close_nvs(void);

#endif // NVS_HELPER_H