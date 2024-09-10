#ifndef CAMERA_H
#define CAMERA_H

#include <esp_system.h>
#include "esp_camera.h"

esp_err_t init_camera(void);
camera_fb_t* take_picture(void);

#endif // CAMERA_H