/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2021/06/06
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 *USART Print debugging routine:
 *USART1_Tx(PA9).
 *This example demonstrates using USART1(PA9) as a print debug port output.
 *
 */

#include "debug.h"
#include "led.h"
#include "button.h"

#include "os_sys.h" 


#if 0
os_task_t task0, task1;
cpu_stack_t task0_stack[200];
cpu_stack_t task1_stack[200];

void os_task_switch_to (os_task_ctx_t * to);
void os_task_switch (os_task_ctx_t * from, os_task_ctx_t * to);
void os_task_switch_from_isr(os_task_ctx_t * from, os_task_ctx_t * to);

void delay (void) {
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 10000; j++) {

        }
    }
}

void os_sched_run(int is_isr)
{
    os_isr_status_t status = os_sched_isr_disable();

    static os_task_t *curren_task = &task0;
    os_task_t *pre_task = curren_task;

    if(curren_task == &task0 )
    {
        curren_task = &task1;
    }else if(curren_task == &task1)
    {
        curren_task = &task0;
    }

    if(is_isr)
    {
        os_task_switch_from_isr(&pre_task->ctx,&curren_task->ctx);
    }else
    {
        os_task_switch(&pre_task->ctx,&curren_task->ctx);
    }
    os_sched_isr_enable(status);
}

uint32_t count = 0;

void led0_task(void * arg)
{
    // uint32_t count = 0;
    // uint32_t number = (uint32_t)arg;
    for(uint32_t i =0;i<1000000;i++)
    {
        os_isr_status_t status_1 = os_sched_isr_disable();
        count++;
        os_sched_isr_enable(status_1);
    }
    os_isr_status_t status_2 = os_sched_isr_disable();
    printf("task0,count:%d\r\n",count);
    os_sched_isr_enable(status_2);
    for(;;)
    {
        led_toggle(LED0);
        //os_sched_run(0);

        delay();
        // printf("task 0 arg: %d\r\n", number);
        // printf("task 0 count: %d\r\n", count++);
        //os_sched_run(0);
    }
}

void led1_task(void * arg)
{
    // uint32_t count = 0;
    // uint32_t number = (uint32_t)arg;

    for(uint32_t i =0;i<1000000;i++)
    {
        os_isr_status_t status_3 = os_sched_isr_disable();
        count--;
        os_sched_isr_enable(status_3);
    }
    os_isr_status_t status_4 = os_sched_isr_disable();
    printf("task1,count:%d\r\n",count);
    os_sched_isr_enable(status_4);

    for(;;)
    {
        led_toggle(LED1);

        delay();
        // printf("task 1 arg: %d\r\n", number);
        // printf("task 1 count: %d\r\n", count++);
        os_sched_run(0);
    }
}

int param_check(int param,uint32_t *error)
{
    //os_param_failed(param>0,-1);
    os_param_failed_exec(param>0,-1,*error = 2);

    return 0;
}
#endif

int main(void)
{   
    os_test();
}