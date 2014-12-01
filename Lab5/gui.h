#ifndef __GUI_H
#define __GUI_H

typedef __packed struct File
{
    struct File *next;
    struct File *prev;
    char *fname;
} File_t;

#define GUI_STATUS_PLAYING  0
#define GUI_STATUS_STOPPED  1

#define GUI_SIG_NEXT        0xFF

// 50 KHz PWM
#define LCD_PWM_LOAD    320
// 100%
#define LCD_PWM_HIGH    0
// 20%
#define LCD_PWM_LOW     256

void GUI_init(void);

#endif
