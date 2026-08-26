/*
 * LED控制
 *
 * author: lishutong
 * site: https://lishutong1024.cn / https://lishutong1024.github.cn
 */
#ifndef LED_H
#define LED_H

/**
 * LED类型
 */
typedef enum {
    LED0 = 0x00,
    LED1 = 0x01,
    LED2 = 0x02,
    LED3 = 0x03,
}led_t;

void led_init (void);
void led_set (led_t led, int on);
int led_is_on (led_t led);
void led_toggle (led_t led);

#define led_on(led) led_set(led, 1)
#define led_off(led) led_set(led, 0)

#endif // LED_H
