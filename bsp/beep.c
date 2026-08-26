/*
 * 蜂鸣器控制
 *
 * author: lishutong
 * site: https://lishutong1024.cn / https://lishutong1024.github.cn
 */
#include "ch32v20x.h"
#include "beep.h"

/**
 * 蜂鸣器鸣叫初始化
 */
void beep_init (void) {
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM4, ENABLE );

    GPIO_InitTypeDef GPIO_InitStructure={0};
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOB, &GPIO_InitStructure );
    beep_stop();
}

/**
 * 蜂鸣器发出指定的声音
 */
void beep_play (beep_t beep) {
    if (beep == BEEP_WARNING) {
        GPIO_ResetBits(GPIOB, GPIO_Pin_9);
    } 
}

/**
 * 蜂鸣器停止发出声音
 */
void beep_stop (void) {
    GPIO_SetBits(GPIOB, GPIO_Pin_9);
}
