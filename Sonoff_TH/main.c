/*
 * Thermostat with buttons
 * 
 * v0.4.1
 * 
 * Copyright 2018 José A. Jiménez (@RavenSystem)
 *  
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_common.h>

#include <etstimer.h>
#include <esplibs/libmain.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>
#include <led_codes.h>

#include <dht/dht.h>
#include <strings.h>

#define MAIN_BUTTON_GPIO        0
#define UPTEMP_BUTTON_GPIO      4
#define DOWNTEMP_BUTTON_GPIO    5
#define LED_GPIO                2
#define RELAY_GPIO              12
#define SENSOR_GPIO             14

#define DEBOUNCE_TIME           500     / portTICK_PERIOD_MS
#define RESET_TIME              10000   / portTICK_PERIOD_MS

#define POLL_PERIOD             10000
uint8_t temp = 0;

uint32_t last_button_event_time, last_reset_event_time;
float old_humidity_value = 0.0, old_temperature_value = 0.0;
static ETSTimer thermostat_timer;

void relay_write(bool on) {
    gpio_write(RELAY_GPIO, on ? 1 : 0);
}

void led_write(bool on) {
    gpio_write(LED_GPIO, on ? 0 : 1);
}

void update_state();

void on_update(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
    update_state();
    printf("*%d\n", temp);
}

void on_target(homekit_characteristic_t *ch, homekit_value_t value, void *context);

void reset_task() {
    sdk_os_timer_disarm(&thermostat_timer);
    
    homekit_server_reset();
    wifi_config_reset();
    
    led_code(LED_GPIO, RESTART_DEVICE);
    
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    
    sdk_system_restart();
    vTaskDelete(NULL);
}

void identify(homekit_value_t _value) {
    led_code(LED_GPIO, IDENTIFY_ACCESSORY);
}

homekit_characteristic_t current_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);
homekit_characteristic_t target_temperature  = HOMEKIT_CHARACTERISTIC_(TARGET_TEMPERATURE, 22, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(on_update));
homekit_characteristic_t units = HOMEKIT_CHARACTERISTIC_(TEMPERATURE_DISPLAY_UNITS, 0);
homekit_characteristic_t current_state = HOMEKIT_CHARACTERISTIC_(CURRENT_HEATING_COOLING_STATE, 0);
homekit_characteristic_t target_state = HOMEKIT_CHARACTERISTIC_(TARGET_HEATING_COOLING_STATE, 1, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(on_target));
homekit_characteristic_t current_humidity = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);

void on_target(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
    switch (target_state.value.int_value) {
        case 1:
            // Heat
            led_code(LED_GPIO, FUNCTION_B);
            break;
            
        case 2:
            // Cool
            led_code(LED_GPIO, FUNCTION_C);
            break;
            
        default:
            // Off
            led_code(LED_GPIO, FUNCTION_A);
            break;
    }
    
    update_state();
}

void update_state() {
    uint8_t state = target_state.value.int_value;
    if (state == 3) {
        state = 0;
        target_state.value = HOMEKIT_UINT8(0);
        homekit_characteristic_notify(&target_state, target_state.value);
    }
    if (state == 1 && current_temperature.value.float_value < target_temperature.value.float_value) {
        if (current_state.value.int_value != 1) {
            current_state.value = HOMEKIT_UINT8(1);
            homekit_characteristic_notify(&current_state, current_state.value);
            
            relay_write(true);
        }
    } else if (state == 2 && current_temperature.value.float_value > target_temperature.value.float_value) {
        if (current_state.value.int_value != 2) {
            current_state.value = HOMEKIT_UINT8(2);
            homekit_characteristic_notify(&current_state, current_state.value);
            
            relay_write(true);
        }
    } else if (current_state.value.int_value != 0) {
        current_state.value = HOMEKIT_UINT8(0);
        homekit_characteristic_notify(&current_state, current_state.value);
            
        relay_write(false);
    }
 temp = target_temperature.value.float_value;

}

void change_temp(bool up_temp) {
    if (up_temp) {
        target_temperature.value.float_value += 1;
    } else {
        target_temperature.value.float_value -= 1;
    }

    led_code(LED_GPIO, FUNCTION_A);
    homekit_characteristic_notify(&target_temperature, target_temperature.value);
    update_state();
}

void change_mode() {
    uint8_t state = target_state.value.int_value + 1;
    switch (state) {
        case 1:
            // Heat
            led_code(LED_GPIO, FUNCTION_B);
            break;

        case 2:
            state = 0;
            // Cool
            led_code(LED_GPIO, FUNCTION_C);
            break;

        default:
            state = 0;
            // Off
            led_code(LED_GPIO, FUNCTION_A);
            break;
    }

    target_state.value = HOMEKIT_UINT8(state);
    homekit_characteristic_notify(&target_state, target_state.value);

    update_state();
}

void button_intr_callback(uint8_t gpio) {
    uint32_t now = xTaskGetTickCountFromISR();
    
    if (((now - last_button_event_time) > DEBOUNCE_TIME) && (gpio_read(gpio) == 1)) {
        if (((now - last_reset_event_time) > RESET_TIME) && (gpio == MAIN_BUTTON_GPIO)) {
            xTaskCreate(reset_task, "Reset", 128, NULL, 1, NULL);
        } else {
            last_button_event_time = now;
            
            switch(gpio) {
                case UPTEMP_BUTTON_GPIO:
                    if (target_temperature.value.float_value <= 37.5) {
                        change_temp(true);
                                           //     printf(">>> Sensor: temperature %i\n",22);

                    }
                    break;
                    
                case DOWNTEMP_BUTTON_GPIO:
                    if (target_temperature.value.float_value >= 10.5) {
                        change_temp(false);

                    }
                    break;
                    
                default:
                    change_mode();
                    break;
            }
        }
    } else if (gpio_read(gpio) == 0) {
        last_reset_event_time = now;
    }
}

void temperature_sensor_task() {
    float humidity_value, temperature_value;
        
    if (dht_read_float_data(DHT_TYPE_DHT22, SENSOR_GPIO, &humidity_value, &temperature_value)) {
       // printf(">>> Sensor: temperature %g, humidity %g\n", temperature_value, humidity_value);
      //  printf("@%g\n",temperature_value);
     //   printf("$%g\n",humidity_value);
     //   printf("*%d\n", temp);
     printf("@%g\n$%g\n*%d\n", temperature_value, humidity_value,temp);
     
        if (temperature_value != old_temperature_value) {
            old_temperature_value = temperature_value;
            current_temperature.value = HOMEKIT_FLOAT(temperature_value);
            homekit_characteristic_notify(&current_temperature, current_temperature.value);
            
            if (humidity_value != old_humidity_value) {
                old_humidity_value = humidity_value;
                current_humidity.value = HOMEKIT_FLOAT(humidity_value);
                homekit_characteristic_notify(&current_humidity, current_humidity.value);
            }
            
            update_state();
        }
    } else {
        printf(">>> Sensor: ERROR\n");
        
        
        led_code(LED_GPIO, SENSOR_ERROR);
        
        if (current_state.value.int_value != 0) {
            current_state.value = HOMEKIT_UINT8(0);
            homekit_characteristic_notify(&current_state, current_state.value);
            
            relay_write(false);
        }
    }
}

void thermostat_init() {
    gpio_enable(LED_GPIO, GPIO_OUTPUT);
    led_write(false);
    
    gpio_set_pullup(MAIN_BUTTON_GPIO, true, true);
    gpio_set_interrupt(MAIN_BUTTON_GPIO, GPIO_INTTYPE_EDGE_ANY, button_intr_callback);
 
    gpio_enable(UPTEMP_BUTTON_GPIO, GPIO_INPUT);
    gpio_set_pullup(UPTEMP_BUTTON_GPIO, true, true);
    gpio_set_interrupt(UPTEMP_BUTTON_GPIO, GPIO_INTTYPE_EDGE_ANY, button_intr_callback);
 
    gpio_enable(DOWNTEMP_BUTTON_GPIO, GPIO_INPUT);
    gpio_set_pullup(DOWNTEMP_BUTTON_GPIO, true, true);
    gpio_set_interrupt(DOWNTEMP_BUTTON_GPIO, GPIO_INTTYPE_EDGE_ANY, button_intr_callback);
    
    gpio_set_pullup(SENSOR_GPIO, false, false);
    
    gpio_enable(RELAY_GPIO, GPIO_OUTPUT);
    relay_write(false);
    
    last_button_event_time = xTaskGetTickCountFromISR();
    
    sdk_os_timer_setfn(&thermostat_timer, temperature_sensor_task, NULL);
    sdk_os_timer_arm(&thermostat_timer, POLL_PERIOD, 1);
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Thermostat");
homekit_characteristic_t serial = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, "TH N/A");

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_thermostat, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            &name,
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Kristian"),
            &serial,
            HOMEKIT_CHARACTERISTIC(MODEL, "Thermostat"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.4.2"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
            NULL
        }),
        HOMEKIT_SERVICE(THERMOSTAT, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Thermostat"),
            &current_temperature,
            &target_temperature,
            &current_state,
            &target_state,
            &units,
            &current_humidity,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

void create_accessory_name() {
    uint8_t macaddr[6];
    sdk_wifi_get_macaddr(STATION_IF, macaddr);
    
    char *name_value = malloc(14);
    snprintf(name_value, 14, "TH %02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);
    
    name.value = HOMEKIT_STRING(name_value);
    serial.value = HOMEKIT_STRING(name_value);
}

void on_wifi_ready() {
    led_code(LED_GPIO, WIFI_CONNECTED);
    
    create_accessory_name();
        
    homekit_server_init(&config);
}

void user_init(void) {
    uart_set_baud(0, 115200);
    
    wifi_config_init("TH", NULL, on_wifi_ready);
    
    thermostat_init();
}