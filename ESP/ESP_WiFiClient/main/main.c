// general header files
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// networking header files
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

// RTOS headerfiles
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

// general ESP headerfiles
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

// i2c headerfiles
#include "driver/i2c.h"

// MPU headerfiles

#include "ahrs.h"
#include "mpu9250.h"
#include "calibrate.h"
#include "common.h"

// macro definitions
#define I2C_MASTER_NUM I2C_NUM_0 /*!< I2C port number for master dev */
#define WIFI_SSID "Phone(2)"
#define WIFI_PASS ".init.py"
#define CHANGE_INSTRUMENT_BTN 2
#define OCTAVE_PLUS_BTN 6
#define OCTAVE_MIN_BTN 3
#define WIFI_CONN_LED 12
#define READY_TO_SEND 13
#define SERVER_IP "192.168.58.133"
#define PORT "8080"
#define MAXLINE 1024
#define MSG_CONFIRM 1

static EventGroupHandle_t s_wifi_event_group;
static const int CONNECTED_BIT = BIT0;
int sockfd;
struct sockaddr_in servaddr;

int roundHead, roundPitch, roundRoll;
static short int currentOctave = 0;     // defaults to octave 0
static short int currentInstrument = 0; // defaults to instrument 0
char mappedNote = 'A';

// calibration values

calibration_t cal = {
    .mag_offset = {.x = 24.343750, .y = 71.121094, .z = -53.583984},
    .mag_scale = {.x = 1.009336, .y = 1.106114, .z = 0.904828},
    .accel_offset = {.x = 0.014293, .y = -0.031169, .z = -0.052805},
    .accel_scale_lo = {.x = 0.998658, .y = 1.005130, .z = 0.993922},
    .accel_scale_hi = {.x = -0.997405, .y = -0.993500, .z = -1.016752},

    .gyro_bias_offset = {.x = -1.032517, .y = -0.334159, .z = 0.085133}};

static void transform_accel_gyro(vector_t *v)
{
    float x = v->x;
    float y = v->y;
    float z = v->z;

    v->x = -x;
    v->y = -z;
    v->z = -y;
}

// mapping for instruments 0 and 1
char getMappedNote(int head, int roll)
{
    if (head > 0 && head < 30)
    {
        if (roll < -20)
        {
            return 'W';
        }
        else
        {
            return 'A';
        }
    }
    else if (head > 30 && head < 60)
    {
        return 'B';
    }
    else if (head > 60 && head < 90)
    {
        if (roll < -20)
        {
            return 'V';
        }
        else
        {
            return 'C';
        }
    }
    else if (head > 90 && head < 120)
    {
        if (roll < -20)
        {
            return 'R';
        }
        else
        {
            return 'D';
        }
    }
    else if (head > 120 && head < 150)
    {
        return 'E';
    }
    else if (head > 150 && head < 180)
    {
        if (roll < -20)
        {
            return 'T';
        }
        else
        {
            return 'F';
        }
    }
    else if (head > 180 && head < 210)
    {
        return 'G';
    }
    else
    {
        return 'A';
    }
}

// mapping for drums
char getDrumNote(int head)
{
    if ((head > 0 && head < 45))
    {
        return 'K';
    }
    else if (head > 45 && head < 90)
    {
        return 'L';
    }
    else if (head > 90 && head < 135)
    {
        return 'J';
    }
    else if (head > 135 && head < 180)
    {
        return 'H';
    }
    else
    {
        return 'L';
    }
}

// connection LED : for wifi connection indication
void connectionLED()
{
    while (1)
    {
        gpio_set_level(WIFI_CONN_LED, 1);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        gpio_set_level(WIFI_CONN_LED, 0);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

// connection LED : for socket creation indication
void socketCreatedLED()
{
    while (1)
    {
        gpio_set_level(READY_TO_SEND, 1);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        gpio_set_level(READY_TO_SEND, 0);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

/**
 * INCREMENT INSTRUMENT ON BUTTON PRESS
 * 1. Piano [STRING INSTRUMENT]
 * 2. Flute [WIND INSTRUMENT]
 * 3. Drums [PERCUSSION INSTRUMENT]
 * resets to 1 after 3
 */
void changeInstrument()
{
    while (1)
    {
        vTaskDelay(200 / portTICK_PERIOD_MS);
        if (gpio_get_level(CHANGE_INSTRUMENT_BTN) == 1)
        {

            currentInstrument = (currentInstrument + 1) % 3;
            printf("\nInstrument changed to Instrument ID:%d\n", currentInstrument);
        }
    }
    vTaskDelete(NULL);
}

// Decrease octave on button press

void changeOctave()
{
    while (1)
    {
        vTaskDelay(200 / portTICK_PERIOD_MS);
        if (gpio_get_level(OCTAVE_PLUS_BTN) == 1)
        {
            currentOctave += 1;
            if (currentOctave > 5)
            {
                currentOctave = 0;
            }
            printf("\nOctave Increased \n Current Octave: %d \n", currentOctave);
        }
        else if (gpio_get_level(OCTAVE_MIN_BTN) == 1)
        {
            currentOctave = currentOctave - 1;
            if (currentOctave < 0)
                currentOctave = 5;
            printf("\nOctave Decreased \n Current Octave: %d \n", currentOctave);
        }
    }
    vTaskDelete(NULL);
}

// MPU code to get values and convert it to <head, pitch, roll>

static void transform_mag(vector_t *v)
{
    float x = v->x;
    float y = v->y;
    float z = v->z;

    v->x = -y;
    v->y = z;
    v->z = -x;
}

void run_imu(void)
{

    i2c_mpu9250_init(&cal);
    ahrs_init(SAMPLE_FREQ_Hz, 0.8);

    uint64_t i = 0;
    while (true)
    {
        vector_t va, vg, vm;

        // Get the Accelerometer, Gyroscope and Magnetometer values.
        ESP_ERROR_CHECK(get_accel_gyro_mag(&va, &vg, &vm));

        // Transform these values to the orientation of our device.
        transform_accel_gyro(&va);
        transform_accel_gyro(&vg);
        transform_mag(&vm);

        // Apply the AHRS algorithm
        ahrs_update(DEG2RAD(vg.x), DEG2RAD(vg.y), DEG2RAD(vg.z),
                    va.x, va.y, va.z,
                    vm.x, vm.y, vm.z);

        // Print the data out every 10 items
        if (i++ % 10 == 0)
        {
            float temp;
            ESP_ERROR_CHECK(get_temperature_celsius(&temp));

            float heading, pitch, roll;
            ahrs_get_euler_in_degrees(&heading, &pitch, &roll);
            // ESP_LOGI(TAG, "heading: %2.3f°, pitch: %2.3f°, roll: %2.3f°, Temp %2.3f°C", heading, pitch, roll, temp);
            roundPitch = round(pitch);
            roundRoll = round(roll);
            roundHead = round(heading);

            // Make the WDT happy
            vTaskDelay(0);
        }

        pause();
    }
}

static void imu_task(void *arg)
{

#ifdef CONFIG_CALIBRATION_MODE
    calibrate_gyro();
    calibrate_accel();
    calibrate_mag();
#else
    run_imu();
#endif

    // Exit
    vTaskDelay(100 / portTICK_PERIOD_MS);
    i2c_driver_delete(I2C_MASTER_NUM);

    vTaskDelete(NULL);
}

// connect to UDP server

bool connectToServer(int *sockfd, struct sockaddr_in *servaddr, const char *serverIP, const char *port)
{
    // Create the socket
    bool isConnected = false;
    *sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (*sockfd < 0)
    {
        perror("[!][ERR] Socket creation failed!");
        exit(EXIT_FAILURE);
    }

    // Set the server address
    servaddr->sin_family = AF_INET;
    servaddr->sin_port = htons(atoi(port));
    servaddr->sin_addr.s_addr = inet_addr(serverIP);
    isConnected = true;
    return isConnected;
}

// send data to server

void sendData(int sockfd, struct sockaddr_in servaddr, int value)
{
    // printf("%d\n", value);
    printf("ROLL: %d PITCH: %d HEAD: %d\n", roundRoll, roundPitch, roundHead);
    // printf("Sending data\n");
    /**
     * Prepare the data frame
     * frame format:
     * [N O I]
     * N: Note which has to be played
     * O: Current octave
     * I: Current instrument
     */

    if (roundPitch < -15)
    {
        if (currentInstrument == 0 || currentInstrument == 1)
            mappedNote = getMappedNote(roundHead, roundRoll);
        else
            mappedNote = getDrumNote(roundHead);
        char frame[MAXLINE];
        printf("\n-----------ESP DATA------------\n");
        printf("\nSending Frame [%c | %d | %d | %d]\n", mappedNote, currentOctave, currentInstrument, roundPitch);
        sprintf(frame, "%c %d %d %d", mappedNote, currentOctave, currentInstrument, roundPitch);
        sendto(sockfd, frame, strlen(frame), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    else
    {
        printf("\n Note Off : Tilt the device down to play the Note\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// task to run after wifi connection
void intializeOnConnect(void *pvParameters)
{

    bool isConnected = false;

    // printf("Waiting for button press");
    isConnected = connectToServer(&sockfd, &servaddr, SERVER_IP, PORT);
    if (isConnected)
    {
        xTaskCreate(socketCreatedLED, "SOCKET-READY", configMINIMAL_STACK_SIZE * 2, NULL, 5, NULL);

        printf("Connected\n");
    }
    while (isConnected)
    {
        sendData(sockfd, servaddr, roundRoll);
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

// wifi event handler

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        printf("\n-----------ESP DATA------------\n");

        printf("Connecting to Wi-Fi...\n");
        printf("\n-------------------------------\n");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        printf("\n-----------ESP DATA------------\n");

        printf("Wi-Fi disconnected. Retrying...\n");
        printf("\n-------------------------------\n");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
        printf("\n-----------ESP DATA------------\n");

        printf("Wi-Fi connected. Got IP address.\n");
        printf("\n-----------ESP DATA------------\n");
        xTaskCreate(connectionLED, "WIFI_CONNECTED", configMINIMAL_STACK_SIZE * 2, NULL, 5, NULL);
    }
}

void initialise_wifi(void *pvParameters)
{
    ESP_ERROR_CHECK(esp_netif_init());
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    vTaskDelete(NULL);
}

void app_main(void)
{
    gpio_set_direction(WIFI_CONN_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(READY_TO_SEND, GPIO_MODE_OUTPUT);
    ESP_ERROR_CHECK(nvs_flash_init());
    xTaskCreate(&initialise_wifi, "WIFI_INIT", configMINIMAL_STACK_SIZE * 3, NULL, 3, NULL);
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    if (bits & CONNECTED_BIT)
    {
        xTaskCreate(&intializeOnConnect, "CONN_INIT_AFTER", configMINIMAL_STACK_SIZE * 2, NULL, 2, NULL);
        xTaskCreate(imu_task, "imu_task", configMINIMAL_STACK_SIZE * 4, NULL, 10, NULL);
        xTaskCreate(changeOctave, "octave_change", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL);
        xTaskCreate(changeInstrument, "instrument_change", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL);
    }
    else
    {
        printf("Wi-Fi connection error \n");
    }
    // start i2c task
}