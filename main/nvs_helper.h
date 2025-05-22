#ifndef NVS_HELPER_H
#define NVS_HELPER_H

#include <nvs_flash.h>
#include <nvs.h>

extern nvs_handle persistent_storage_handle;

bool init_nvs(void);
void read_value(const char* key, uint32_t* value);
bool write_value(const char* key, uint32_t value);
void close_nvs(void);

#endif // NVS_HELPER_H
