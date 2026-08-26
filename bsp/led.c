/*
 * LED控制
 *
 * author: lishutong
 * site: https://lishutong1024.cn / https://lishutong1024.github.cn
 */
#include "ch32v20x.h"
#include "led.h"

// 管脚映射表
static const uint8_t pin_table[] = {GPIO_Pin_0, GPIO_Pin_1, GPIO_Pin_2, GPIO_Pin_3};

/**
 * 初始化led
 */
void led_init (void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 初始所有的灭
    GPIO_SetBits(GPIOA, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3);
}

/**
 * 设置LED点亮或关闭
 */
void led_set (led_t led, int on) {
    BitAction bit = on ? Bit_RESET : Bit_SET;

    GPIO_WriteBit(GPIOA, pin_table[led], bit);
}

/**
 * 翻转LED灯的显示
 */
void led_toggle (led_t led) {
    uint16_t pin = pin_table[led];
    uint8_t bit = GPIO_ReadInputDataBit(GPIOA, pin);
    GPIO_WriteBit(GPIOA, pin, bit ? Bit_RESET : Bit_SET);
}


/**
 * 读取当前LED的状态，是否处于点亮状态
 */
int led_is_on (led_t led) {
    uint16_t pin = pin_table[led];
    return !GPIO_ReadInputDataBit(GPIOA, pin);
}

