
#include "msp.h"
#include "final_project_lib.h"
#include "ST7735.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <right_arrow.h>
#include <left_arrow.h>

// Macros for LCD in General
#define BG_COLOR 0xFFFF
#define LCD_X_POS_MAX 125
#define CHAR_PIXEL_SIZE 6    // Pixel Size for each char

// Macros for text and images of LCD 1
#define ARROW_WIDTH 50
#define ARROW_HEIGHT 30

#define LEFT_ARROW_X_POS 3  // Positions of left blinker arrow
#define LEFT_ARROW_Y_POS 30

#define RIGHT_ARROW_X_POS 75 // Positions of right blinker arrow
#define RIGHT_ARROW_Y_POS 30

#define SPEED_X_POS 65       // Position of Speed of Motor
#define SPEED_Y_POS 70
#define MPH_X_POS 65         // Position of MPH label
#define MPH_Y_POS 100

#define TEXT_SIZE_LCD1 2          // Text size for both speed and mph label
#define TEXT_COLOR_LCD1_R 63
#define TEXT_COLOR_LCD1_G 100
#define TEXT_COLOR_LCD1_B 245

// Macros for text and images of LCD 2
#define TEXT_SIZE_LCD2 1
#define TEMP_Y_POS 10         // Y position of temp text, x is not needed

#define MENU_X_POS 10
#define MENU_Y_POS 140

#define TEXT_COLOR_LCD2_R 86
#define TEXT_COLOR_LCD2_G 245
#define TEXT_COLOR_LCD2_B 63

// Global Variables
int blink_left_arrow_state = 0;   // LCD 1
int blink_right_arrow_state = 0;

/********************* MSP432 specific functions  ***********************************/

/************************************************************************************
 * This function creates a systick delay in us.
 * Input (time): time that is delay in us
 ***********************************************************************************/
void systick_delay_us_48MHz(int time)
{
    SysTick->CTRL = 0;
    SysTick->LOAD = (time * 48) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = 5;

    while ((SysTick->CTRL & 0x00010000) == 0);
}

/************************************************************************************
 * This function creates a systick delay in ms.
 * Input (time): time that is delay in ms
 ***********************************************************************************/
void systick_delay_ms_48MHz(int time)
{
    SysTick->CTRL = 0;
    SysTick->LOAD = (time * 48000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = 5;

    while ((SysTick->CTRL & 0x00010000) == 0);
}

/************************************************************************************
 * This function creates a systick delay in sec.
 * Input (time): time that is delay in sec
 ***********************************************************************************/
void systick_delay_s_48MHz(int time)
{
    int i = 0;

    for (i = 0; i < (time * 1000); i++){
        systick_delay_ms_48MHz(1);
    }
}

/*************************************************************************************
 * This program lets the user know if there is an error when the clock was set.
 ************************************************************************************/
void error(void)
{
    volatile uint32_t i;

    while (1)
    {
        P1->OUT ^= BIT0;
        for(i = 20000; i > 0; i--);           // Blink LED forever
    }
}

/************************************************************************************
 * This function sets the master clock to 48MHz.
 ***********************************************************************************/
void clock_48MHz_initial_MCLK(void){

    volatile uint32_t i;
    uint32_t currentPowerState;

    WDT_A->CTL = WDT_A_CTL_PW |
                 WDT_A_CTL_HOLD;            // Stop WDT

    P1->DIR |= BIT0;                        // P1.0 set as output

    /* NOTE: This example assumes the default power state is AM0_LDO.
     * Refer to  msp432p401x_pcm_0x code examples for more complete PCM
     * operations to exercise various power state transitions between active
     * modes.
     */

    /* Step 1: Transition to VCORE Level 1: AM0_LDO --> AM1_LDO */

    /* Get current power state, if it's not AM0_LDO, error out */
    currentPowerState = PCM->CTL0 & PCM_CTL0_CPM_MASK;
    if (currentPowerState != PCM_CTL0_CPM_0)
        error();

    while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY));
    PCM->CTL0 = PCM_CTL0_KEY_VAL | PCM_CTL0_AMR_1;
    while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY));
    if (PCM->IFG & PCM_IFG_AM_INVALID_TR_IFG)
        error();                            // Error if transition was not successful
    if ((PCM->CTL0 & PCM_CTL0_CPM_MASK) != PCM_CTL0_CPM_1)
        error();                            // Error if device is not in AM1_LDO mode

    /* Step 2: Configure Flash wait-state to 1 for both banks 0 & 1 */
    FLCTL->BANK0_RDCTL = (FLCTL->BANK0_RDCTL & ~(FLCTL_BANK0_RDCTL_WAIT_MASK)) |
            FLCTL_BANK0_RDCTL_WAIT_1;
    FLCTL->BANK1_RDCTL  = (FLCTL->BANK0_RDCTL & ~(FLCTL_BANK1_RDCTL_WAIT_MASK)) |
            FLCTL_BANK1_RDCTL_WAIT_1;

    /* Step 3: Configure DCO to 48MHz, ensure MCLK uses DCO as source*/
    CS->KEY = CS_KEY_VAL ;                  // Unlock CS module for register access
    CS->CTL0 = 0;                           // Reset tuning parameters
    CS->CTL0 = CS_CTL0_DCORSEL_5;           // Set DCO to 48MHz
    /* Select MCLK = DCO, no divider */
    CS->CTL1 = CS->CTL1 & ~(CS_CTL1_SELM_MASK | CS_CTL1_DIVM_MASK) |
            CS_CTL1_SELM_3;
    CS->KEY = 0;                            // Lock CS module from unintended accesses
}

/********************* LCD 1 specific functions *************************************/

/************************************************************************************
 * This function displays the main LCD screen without the blinking turn signals
 ***********************************************************************************/
void display_main_screen_LCD1(int speed){

    int i = 0;
    int x_pos = 0;
    int y_pos = 0;
    int speed_len = 0;

    char speed_text[10] = {0};
    char mph_text[10] = "mph";

    uint8_t text_color = ST7735_Color565(TEXT_COLOR_LCD1_R, TEXT_COLOR_LCD1_G,
                                         TEXT_COLOR_LCD1_B);

    // display speed as a text
    sprintf(speed_text,"%-4d", speed);

    // find length of speed text
    if (speed >= 100){

        speed_len = 3;
    }
    else if (speed >= 10){
        speed_len = 2;
    }
    else {

        speed_len = 1;
    }


    // display left arrow
    if (blink_left_arrow_state == 0){

        ST7735_DrawBitmap(LEFT_ARROW_X_POS, LEFT_ARROW_Y_POS,
                          no_blink_arrow_left,
                          ARROW_WIDTH, ARROW_HEIGHT);
    }

    else if(blink_left_arrow_state == 1){

        ST7735_DrawBitmap(LEFT_ARROW_X_POS, LEFT_ARROW_Y_POS,
                          blink_arrow_left,
                          ARROW_WIDTH, ARROW_HEIGHT);
    }

    // display right arrow
    if (blink_right_arrow_state == 0){

        ST7735_DrawBitmap(RIGHT_ARROW_X_POS, RIGHT_ARROW_Y_POS,
                          no_blink_arrow_right,
                          ARROW_WIDTH, ARROW_HEIGHT);
    }

    if(blink_right_arrow_state == 1){

        ST7735_DrawBitmap(RIGHT_ARROW_X_POS, RIGHT_ARROW_Y_POS,
                          blink_arrow_right,
                          ARROW_WIDTH, ARROW_HEIGHT);
    }

    // speed
    for (i = 0; i < speed_len; i++){

        x_pos = center_align(CHAR_PIXEL_SIZE, TEXT_SIZE_LCD1, speed_len, SPEED_X_POS)
                                         +  (i * CHAR_PIXEL_SIZE * TEXT_SIZE_LCD1);
        y_pos = SPEED_Y_POS;

        ST7735_DrawCharS(x_pos, y_pos, speed_text[i],
                         text_color, BG_COLOR, TEXT_SIZE_LCD1);
    }

    for (i = 0; i < strlen(mph_text); i++){

        x_pos = center_align(CHAR_PIXEL_SIZE, TEXT_SIZE_LCD1, 3, MPH_X_POS)
                                         +  (i * CHAR_PIXEL_SIZE * TEXT_SIZE_LCD1);
        y_pos = MPH_Y_POS;

        ST7735_DrawCharS(x_pos, y_pos, mph_text[i],
                         text_color, BG_COLOR, TEXT_SIZE_LCD1);
    }
}

int center_align(int pixel_size, int char_size,int text_len, int x_pos_center){

    int x_pos = 0;
    int length = pixel_size * char_size * text_len;

    x_pos = x_pos_center - (length / 2);

    return x_pos;
}

/************************************************************************************
 * This function toggles the left arrow on and off on lcd number 1.
 ***********************************************************************************/
void toggle_blink_left_arrow(void){

    if (blink_left_arrow_state == 0){

        blink_left_arrow_state = 1;
    }

    else if(blink_left_arrow_state == 1){

        blink_left_arrow_state = 0;
    }
}

/************************************************************************************
 * This function toggles the right arrow on and off on lcd number 1.
 ***********************************************************************************/
void toggle_blink_right_arrow(void){

    if (blink_right_arrow_state == 0){

        blink_right_arrow_state = 1;
    }

    else if(blink_right_arrow_state == 1){

        blink_right_arrow_state = 0;
    }
}

/********************* LCD 2 specific functions *************************************/
void display_main_screen_LCD2(float temp, char temp_unit){


    char temp_str[20] = {0};
    char menu_str[20] = "menu";

    uint8_t text_color = ST7735_Color565(TEXT_COLOR_LCD2_R, TEXT_COLOR_LCD2_G,
                                         TEXT_COLOR_LCD2_B);

    sprintf(temp_str,"%f%c%c",temp,0xBA,temp_unit);

    // temperature
    ST7735_DrawString((LCD_X_POS_MAX - (strlen(temp_str)) * CHAR_PIXEL_SIZE),
                      TEMP_Y_POS, temp_str, text_color);

    // Menu
    ST7735_DrawString(MENU_X_POS, MENU_Y_POS, menu_str, text_color);
}

