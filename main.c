#include "msp.h"
#include "ST7735.h"
#include "final_project_lib.h"

/**
 * main.c
 */
void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

	clock_48MHz_initial_MCLK();
	ST7735_InitR(INITR_REDTAB);
    ST7735_FillScreen(0xFFFF);               // set screen to White

    int speed = 0;
    int i =0;

    while(1){


        display_main_screen_LCD1(speed);
        for (i = 0; i < 100; i++){
        systick_delay_ms_48MHz(1);
        }
        speed++;
        toggle_blink_right_arrow();
        toggle_blink_left_arrow();
    }
}
