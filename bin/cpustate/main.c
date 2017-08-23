#include "defs.h"
#include "keypad.h"
#include "lcd.h"
#include "led.h"
#include "serial.h"
#include "syscon.h"
#include "rtc.h"

#include "drawing.h"
#include "lib.h"
#include "atag.h"
//#include <stdint.h>

static uint8_t screen0[320*240*4];
static uint8_t screen1[320*240*4];
const char *hello = "Hello world!";

void format_time(char *buf)
{
	int year, month, day, hour, min, sec;
	char buffer[32];

	rtc_get_time(&year, &month, &day, &hour, &min, &sec);
	buf[0] = 0;

	/* Print date with the american style. */
	strcat(buf, itoa(year, buffer, 10));
	strcat(buf, "/");
	strcat(buf, itoa(day, buffer, 10));
	strcat(buf, "/");
	strcat(buf, itoa(month, buffer, 10));
	strcat(buf, " ");

	/* Print time. */
	if (hour < 10)
		strcat(buf, "0");
	strcat(buf, itoa(hour, buffer, 10));
	strcat(buf, ":");

	if (min < 10)
		strcat(buf, "0");
	strcat(buf, itoa(min, buffer, 10));
	strcat(buf, ":");

	if (sec < 10)
		strcat(buf, "0");
	strcat(buf, itoa(sec, buffer, 10));
}

void disable_interrupts(void)
{
    asm volatile ("MSR CPSR_c,0xd3":::);
}

extern uint32_t * _binary_bin_cpustate_zImage_start;
extern uint32_t * _binary_bin_cpustate_zImage_end;
void boot_linux(void)
{
    disable_interrupts();
    void * kernel_start=(void *) 0x30000000+32768;
    void * kernel_section_start = _binary_bin_cpustate_zImage_start;
    uint32_t kernel_size = _binary_bin_cpustate_zImage_end-_binary_bin_cpustate_zImage_start;
    memcpy(kernel_start, kernel_section_start, kernel_size);
    //setup_tags((void *)0x30000100);
    setup_tags();
    void (*theKernel)(uint32_t zero, uint32_t arch, uint32_t * params);
    theKernel = (void (*)(uint32_t, uint32_t, uint32_t *))kernel_start;
    theKernel(0,4938,0x30000100);
}

void main(void)
{
    //disable_interrupts();
	int leds = 0x7, prev_sec, sec, ok_released = 0, ok_pressed = 0;
	char buffer[128];
	uint8_t *screen;
	lcd_init();
	led_init();
	keypad_init();
	serial_init(115200);
	rtc_init();

	serial_puts(hello);
	serial_putc('\n');

	/* Align ourselves on the next second for drama purposes. */
	/*rtc_get_time(NULL, NULL, NULL, NULL, NULL, &prev_sec);
	do {
		rtc_get_time(NULL, NULL, NULL, NULL, NULL, &sec);
	} while (prev_sec == sec);*/

	memset(screen0, 0xff, sizeof(screen0));
	lcd_set_buffers(screen0, screen1);
	lcd_set_mode(VIDMODE_R8G8B8);
	lcd_set_backlight(1);

	/* Do something pretty. */
	do {
		serial_puts("Changing LEDs...\n");

		led_set(leds);
		leds = (leds + 1) % 8;

		do {
			keypad_scan();

			/* Keep track of time. */
			//prev_sec = sec;
			//rtc_get_time(NULL, NULL, NULL, NULL, NULL, &sec);

			/* Use double buffer for flicker-free operation. */
			if (lcd_get_active_buffer() == 0)
				screen = screen1;
			else
				screen = screen0;

			memset(screen, 0xFF, sizeof(screen0));
			/* Print message. */
			font_draw_text_r8g8b8(hello, (320 - (strlen(hello)*9)) / 2, 80, screen, 0x0, 0xFFFFFF);
            buffer[0]=0;
            register int q asm("r0");
            asm volatile ("MRS %[res],CPSR":[res]"=r"(q)::);
			//font_draw_text_r8g8b8(buffer, 0, 80, screen, 0x0, 0xFFFFFF);

			/* Print time. */
			//format_time(buffer);
			//font_draw_text_r8g8b8(buffer, (320 - (strlen(buffer)*9)) / 2, 160, screen, 0x0, 0xFFFFFF);

			/* Print keys. */
			buffer[0] = 0;
			strcat(buffer, "Keys:");
			for (key_id i = KEY_A; i < KEY_LAST; i++) {
				if (keypad_get(i)) {
					strcat(buffer, " ");
					strcat(buffer, keypad_get_name(i));
				}
			}
			font_draw_text_r8g8b8(buffer, 0, 224, screen, 0x0, 0xFFFFFF);

			/* Change buffer to show on screen. */
			if (lcd_get_active_buffer() == 0)
				lcd_set_active_buffer(1);
			else
				lcd_set_active_buffer(0);

			/* Reboot if the ON key is pressed. */
			if (keypad_get(KEY_ON) == 0)
				ok_released = 1;
			if ((keypad_get(KEY_ON) == 1) && ok_released)
				ok_pressed = 1;
			if ((keypad_get(KEY_ON) == 0) && ok_pressed)
				syscon_reset();
            //if ((keypad_get(KEY_SHIFT)==1))
            //    boot_linux();
		} while (prev_sec == sec);
	} while(1);

	__builtin_unreachable();
}
