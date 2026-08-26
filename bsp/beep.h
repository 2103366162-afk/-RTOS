/*
 * 蜂鸣器控制
 *
 * author: lishutong
 * site: https://lishutong1024.cn / https://lishutong1024.github.cn
 */
#ifndef BEEP_H
#define BEEP_H

/**
 * 蜂鸣鸣叫的声音类型
 */
typedef enum beep_t {
    BEEP_WARNING,
}beep_t;

void beep_init (void);
void beep_play (beep_t beep);
void beep_stop (void);

#endif // BEEP_H
