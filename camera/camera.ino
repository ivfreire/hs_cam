#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include <time.h>
#include <NTPClient.h>
#include <WiFi.h>

#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

// Set static IP address
IPAddress local_IP(10, 7, 155, 27);
IPAddress gateway(10, 7, 155, 1);
IPAddress subnet(255, 255, 224, 0);
IPAddress primaryDNS(8, 8, 8, 8); //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

struct Config {
    char ssid[32] = "IFnet";
    char password[64] = "";
} cfg;

void startCameraServer();
void capture(fs::FS &fs);

// Initialize SD card
void initSDcard() {
    Serial.println("Initializing SD card...");
    if (!SD_MMC.begin()) {
        Serial.println("SD card not mounted.");
        while(1);
    }
    Serial.println("SD card initialized successfully.");

    Serial.println("Testing SD card type...");
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD card inserted.");
        while(1);  
    }
    Serial.println("SD card is ready.");
}

// Load camera configuraton
camera_config_t configCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    if(psramFound()){
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }
  
    #if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
    #endif

    return config;
}

// Initialize camera
void initCamera(camera_config_t config) {
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }
  
    sensor_t * s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, -2);
    }
    s->set_framesize(s, FRAMESIZE_QVGA);

    #if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
    #endif  
}

// Initilizes WiFi connection
void initWiFi(const char* ssid, const char* password) {
    Serial.println("Trying to connect to WiFi network " + (String)ssid + "...");

    if(!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.println("STA Failed to configure");
    }
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
}

void syncClock(int gmtOffset_sec, int daylightOffset_sec, const char* ntpServer) {
    Serial.println("Synchronizing clocks...");
    configTime(gmtOffset_sec * 3600, daylightOffset_sec * 3600, ntpServer);
    Serial.println("Clocks sync!");
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    Serial.println("ESP32-CAM module");
    Serial.print("ESP Board MAC Address:  ");
    Serial.println(WiFi.macAddress());
  
    // Load configuration
    camera_config_t config = configCamera();
  
    // Initialize stuff
    initSDcard();
    initCamera(config);
    initWiFi(cfg.ssid, cfg.password);

    // Start HTTP server
    startCameraServer();
  
    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");

    // Sync clock 
    syncClock(-3, 1, "pool.ntp.org");
}

void loop() {
    capture(SD_MMC);
    delay(30 * 1000);
}
