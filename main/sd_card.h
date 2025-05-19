#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdbool.h>

bool mount_sd_card();
bool import_settings_from_file();
bool unmount_sd_card();

#endif //SD_CARD_H
