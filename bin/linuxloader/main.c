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

//static uint8_t screen0[320*240*4];
//static uint8_t screen1[320*240*4];
uint8_t * screen0 = (uint8_t*) 0x30700000;
uint8_t * screen1 = (uint8_t*) 0x30700000+320*240*4;
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
void disable_mmu_cache(void)
{
    register volatile int r0 asm("r0");
    asm volatile ("MRC p15,0,r0,c1,c0,0\n\t"
                  "bic R0,R0,#0x7\n\t"
                  "bic R0,R0,#0x1300\n\t"
                  "MCR p15,0,r0,c1,c0,0\n\t":::);
    volatile int f=r0;
    char buf[40];
    buf[0]=0;
    itoa(f, buf,10);
    serial_puts(buf);
    serial_puts("\n");
}


extern uint32_t _binary_bin_linuxloader_zImage_start;
extern uint32_t _binary_bin_linuxloader_zImage_end;
void boot_linux(void)
{
    char buf[40];
    buf[0]=0;
    disable_interrupts();
    disable_mmu_cache();
    {
        serial_puts("mode:\n");
        register int q asm("r0");
        int f;
        asm volatile ("MRS %[res],CPSR":[res]"=r"(q)::);
        f=q;
        itoa(f, buf,10);
        serial_puts(buf);
        serial_puts("\n");
    }
    void * kernel_start=(void *) 0x30000000+32768;
    uint32_t kernel_section_start = (uint32_t) &_binary_bin_linuxloader_zImage_start;
    uint32_t kernel_section_end = (uint32_t) &_binary_bin_linuxloader_zImage_end;
    uint32_t kernel_size = ((uint32_t)&_binary_bin_linuxloader_zImage_end)-((uint32_t)&_binary_bin_linuxloader_zImage_start);
    kernel_size=kernel_section_end-kernel_section_start;
    serial_puts("starting copy");
    buf[0]=0;
    itoa(kernel_size,buf,10);
    serial_puts(buf);
    serial_puts("\n");
    memcpy(kernel_start, (void *) kernel_section_start, kernel_size);
    serial_puts("finished copy");
    setup_tags();
    uint32_t * atags = (uint32_t *) 0x30000100;
    void (*theKernel)(uint32_t zero, uint32_t arch, uint32_t * params);
    theKernel = (void (*)(uint32_t, uint32_t, uint32_t *))kernel_start;
    theKernel(0,4938,atags);
}
void print_int(char *message,int i)
{
    char buf[128]={0};
    char buf2[32]={0};
    strcat(buf,message);
    strcat(buf,itoa(i,buf2,10));
    strcat(buf,"\n");
    serial_puts(buf);
}


int main(void)
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
	rtc_get_time(NULL, NULL, NULL, NULL, NULL, &prev_sec);
	do {
		rtc_get_time(NULL, NULL, NULL, NULL, NULL, &sec);
	} while (prev_sec == sec);

	//memset(screen0, 0xff, sizeof(screen0));
	memset(screen0, 0xff, 320*240*4);
	lcd_set_buffers(screen0, screen1);
	lcd_set_mode(VIDMODE_R8G8B8);
	lcd_set_backlight(1);
    print_int("vc0 ",*(uint32_t*)0x4C800000);
    print_int("vc1 ",*(uint32_t*)0x4C800004);
    print_int("vt0 ",*(uint32_t*)0x4C800008);
    print_int("vt1 ",*(uint32_t*)0x4C80000c);
    print_int("vt2 ",*(uint32_t*)0x4C800010);
    print_int("ubrdivn  ",*(uint32_t*)0x50000028);
    print_int("udivslot ",*(uint32_t*)0x5000002c);
    print_int("INTMSK ",*(uint32_t*)0x4a000008);

	/* Do something pretty. */
	do {
		serial_puts("Changing LEDs...\n");

		led_set(leds);
		leds = (leds + 1) % 8;

		do {
			keypad_scan();

			/* Keep track of time. */
			prev_sec = sec;
			rtc_get_time(NULL, NULL, NULL, NULL, NULL, &sec);

			/* Use double buffer for flicker-free operation. */
			if (lcd_get_active_buffer() == 0)
				screen = screen1;
			else
				screen = screen0;

			memset(screen, 0xFF, 320*240*4);
			//memset(screen, 0xFF, sizeof(screen0));
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
            if ((keypad_get(KEY_SHIFT)==1))
                boot_linux();
		} while (prev_sec == sec);
        delay(100000);
	} while(1);

	__builtin_unreachable();
}
