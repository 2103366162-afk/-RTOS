/*
 * 按键控制
 *
 * author: lishutong
 * site: https://lishutong1024.cn / https://lishutong1024.github.cn
 */
#include "ch32v20x.h"
#include "button.h"

/**
 * 按钮初始化
 */
void button_init (void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/**
 * 判断按钮是否按下
 */
int button_pressed (button_t btn) {
    GPIO_TypeDef * gpio;
    uint16_t pin;
    switch (btn) {
        case BUTTON_0: {
            gpio = GPIOA;
            pin = GPIO_Pin_5;
            break;
        }
        case BUTTON_1:{
            gpio = GPIOA;
            pin = GPIO_Pin_6;
            break;
        }
        case BUTTON_2:{
            gpio = GPIOA;
            pin = GPIO_Pin_7;
            break;
        }
        default: {
            gpio = GPIOC;
            pin = GPIO_Pin_4;
            break;
        }
    }
    return GPIO_ReadInputDataBit(gpio, pin) == RESET;
}
