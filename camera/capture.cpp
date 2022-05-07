/*
	Dept. of Atmospheric Sciences - IAG/USP
	Created by Ícaro Vaz Freire on 24/12/2021.
	Supervisor Prof. Márcia Akemi Yamasoe.
	São Paulo, Brazil.
*/
#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include <NTPClient.h>
#include <Arduino.h>
#include <time.h>

String imageDir;
int today = 0;

void saveCapture(fs::FS &fs, const char* path) {
    camera_fb_t * fb = NULL;

    // Capture 
    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Capture failed.");
        return;
    }

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Unable to save camera capture.");  
    } else {
        file.write(fb->buf, fb->len);
        Serial.println("Saved pic to " + (String)path);
    }
    file.close();
    esp_camera_fb_return(fb);
}

String getImageDir(struct tm* ptm) { 
    char dir[32];
    strftime(dir, 32, "/images/%Y/%m/%d/", ptm);
    return (String)dir;
}

String getImageName(struct tm* ptm) {
    char name[16];
    strftime(name, 16, "%H%M%S.jpg", ptm);
    return (String)name;
}

void createPath(fs::FS &fs, String path) {
    String folder = "/";
    for (int i = 1; i < strlen(path.c_str()); i++) {
        char c = path[i];
        if (c == '/') {
            File file = fs.open(folder);
            if (!file) {
                fs.mkdir(folder);
                Serial.println("Created folder " + folder);  
            } else Serial.println("Folder" + folder + " already exists.");
        }
        folder += c;
    }
}

void capture(fs::FS &fs) {
    // Get current time
    time_t rawtime;
    struct tm* ptm;
    time(&rawtime);
    ptm = localtime(&rawtime);

    // Check if a day has elapsed
    if (today != ptm->tm_yday) {
        imageDir = getImageDir(ptm);
        createPath(SD_MMC, imageDir);
        Serial.println("Image directory is " + imageDir);
        today = ptm->tm_yday;
    }

    // Save capture
    String path = imageDir + getImageName(ptm);
    saveCapture(SD_MMC, path.c_str());
}
