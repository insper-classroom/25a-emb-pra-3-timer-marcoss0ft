#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include <string.h>
#define MAX_LEN 10

const int TRIGGER_PIN = 15;
const int ECHO_PIN = 28;
volatile uint64_t dt;
volatile int dt_flag = 0;
volatile bool timer_fired = false;


void echo_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        dt = to_us_since_boot(get_absolute_time());
    } else if (events & GPIO_IRQ_EDGE_FALL) {
        dt = to_us_since_boot(get_absolute_time()) - dt;
        dt_flag = 1;
    }
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    timer_fired = true;
    return 0;
}

int main() {
    stdio_init_all();

    gpio_init(TRIGGER_PIN);
    gpio_set_dir(TRIGGER_PIN, GPIO_OUT);

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_pull_down(ECHO_PIN);

    double distancia;
    int flag_running = 0;

    gpio_set_irq_enabled_with_callback(ECHO_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &echo_callback);

    datetime_t t = {
        .year  = 2024,
        .month = 1,
        .day   = 13,
        .dotw  = 3,
        .hour  = 11,
        .min   = 20,
        .sec   = 0
    };

    rtc_init();
    rtc_set_datetime(&t);

    char buffer[MAX_LEN];
    int caracter;

    while (true) {

        int i = 0;

        while(i < MAX_LEN-1){
            caracter = getchar_timeout_us(1000000);
            if(caracter == PICO_ERROR_TIMEOUT){
                break;
            }
            printf("%c", caracter);
            if(caracter == '\n'){
                buffer[i] = '\0';
                if (strcmp(buffer, "start") == 0) {
                    printf("Inicializando leitura e log\n");
                    flag_running = 1;
                }
                else if (strcmp(buffer, "stop") == 0) {
                    printf("Desligando leitura e log\n");
                    flag_running = 0;
                }
                while (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT);
                break;
            }
            buffer[i] = caracter;
            i++;
        }

        if (!flag_running) {
            sleep_ms(100);
            continue;
        }

        rtc_get_datetime(&t);
        char datetime_buf[256];
        char *datetime_str = &datetime_buf[0];
        datetime_to_str(datetime_str, sizeof(datetime_buf), &t);

        gpio_put(TRIGGER_PIN, 1);
        sleep_us(10);
        gpio_put(TRIGGER_PIN, 0);


        if (!dt_flag) {
            alarm_id_t alarm = add_alarm_in_us(1000000, &alarm_callback, NULL, true);
            if (!alarm) {
                printf("Erro ao criar alarme\n");
            }
        }

        if (dt_flag) {
            distancia = (dt / 1000000.0) * 34000 / 2; 
            printf("%s - %.3f cm \n", datetime_str, distancia);

            dt_flag = 0;
            timer_fired = false;  
        }

        if (timer_fired) {
            printf("%s - Falha \n", datetime_str);
            timer_fired = false;
        }

        sleep_ms(100);
    }
}
