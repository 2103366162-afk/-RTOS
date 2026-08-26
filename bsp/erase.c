/*
 *  擦除控制
 *
 * author: lishutong
 * site: https://lishutong1024.cn / https://lishutong1024.github.cn
 */
#include "ch32v20x.h"

/**
 * 清除整个芯片
 */
void erase_chip_check (void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = GPIO_Pin_8;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &gpio);

    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8) == SET) {
        FLASH_Unlock();
        FLASH_EraseAllPages();
    }
}


