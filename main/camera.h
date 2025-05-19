#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"

esp_err_t init_camera(void);
void set_camera_resolution(framesize_t frameSize);
camera_fb_t* take_picture(void);
esp_err_t free_camera(void);

#endif // CAMERA_H