#include "esp_camera.h"
#include <WiFi.h>
#include "FS.h"
#include "SD_MMC.h"
#include "esp_http_server.h"
#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

#define ENROLL_DIR "/faces"
static face_id_list id_list = {0};
httpd_handle_t stream_httpd = NULL;

void initSDCard() {
    if(!SD_MMC.begin("/sdcard", true)){
        Serial.println("SD_MMC Card Mount Failed");
        return;
    }
    if(SD_MMC.cardType() == CARD_NONE){
        Serial.println("No SD_MMC card attached");
        return;
    }
    if(!SD_MMC.exists(ENROLL_DIR)){
        SD_MMC.mkdir(ENROLL_DIR);
    }
}

void loadFacesFromSD() {
    int face_id = 0;
    char path[32];
    alloc_id_list(&id_list, 10); 

    while(true) {
        sprintf(path, "%s/face_%d.bin", ENROLL_DIR, face_id);
        if(!SD_MMC.exists(path)) break;
        
        File file = SD_MMC.open(path, FILE_READ);
        if(file) {
            face_id_node *new_face = (face_id_node *)malloc(sizeof(face_id_node));
            new_face->id_vec = (float *)malloc(sizeof(float) * FLASH_144_LEN);
            file.read((uint8_t *)new_face->id_vec, sizeof(float) * FLASH_144_LEN);
            file.close();
            
            new_face->next = id_list.head;
            id_list.head = new_face;
            id_list.count++;
            face_id++;
        }
    }
    Serial.printf("Loaded %d faces from SD Card.\n", face_id);
}

void processFaceRecognition(camera_fb_t * fb) {
    dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
    if (!image_matrix) return;

    if(!fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item)){
        dl_matrix3du_free(image_matrix);
        return;
    }

    box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);
    if (net_boxes){
        int matched_id = recognize_face(&id_list, image_matrix, net_boxes);
        if (matched_id >= 0) {
            Serial.println("MATCH_FOUND");
        }
        free(net_boxes->box);
        free(net_boxes->landmark);
        free(net_boxes);
    }
    dl_matrix3du_free(image_matrix);
}

static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];

    res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=123456789000000000000987654321");
    if(res != ESP_OK) return res;

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) { delay(20); continue; }
        
        processFaceRecognition(fb);

        if(fb->format != PIXFMT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            esp_camera_fb_return(fb);
            fb = NULL;
            if(!jpeg_converted){ res = ESP_FAIL; break; }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, "\r\n--123456789000000000000987654321\r\n", 36);
        }
        
        if(fb){ esp_camera_fb_return(fb); fb = NULL; _jpg_buf = NULL; } 
        else if(_jpg_buf){ free(_jpg_buf); _jpg_buf = NULL; }
        if(res != ESP_OK) break;
    }
    return res;
}

void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 81;

    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}

void setup() {
    Serial.begin(115200);
    
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
    config.pixel_format = PIXFMT_JPEG;
    config.frame_size = FRAMESIZE_QVGA; 
    config.jpeg_quality = 12;
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) { return; }

    initSDCard();
    loadFacesFromSD();

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    startCameraServer();
}

void loop() {
    delay(1); 
}
