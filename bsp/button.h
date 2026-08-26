/*
 * 按键控制
 *
 * author: lishutong
 * site: https://lishutong1024.cn / https://lishutong1024.github.cn
 */
#ifndef BUTTON_H
#define BUTTON_H

/**
 * 按键号
 */
typedef enum {
    BUTTON_0,
    BUTTON_1,
    BUTTON_2,
    BUTTON_3,
}button_t;

void button_init (void);
int button_pressed (button_t button);

#endif // BTN_H
