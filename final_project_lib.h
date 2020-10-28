/*
 * final_project_lib.h
 *
 *  Created on: Oct 25, 2020
 *      Author: wayne
 */

#ifndef FINAL_PROJECT_LIB_H_
#define FINAL_PROJECT_LIB_H_

/***** MSP432 Specific functions *****/
void systick_delay_us_48MHz(int time);
void systick_delay_ms_48MHz(int time);
void systick_delay_s_48MHz(int time);
void clock_48MHz_initial_MCLK(void);
void error(void);

/***** LCD number 1 Specific functions *****/
void display_main_screen_LCD1(int speed);
int center_align(int pixel_size, int char_size,int text_len, int x_pos_center);
void toggle_blink_left_arrow(void);
void toggle_blink_right_arrow(void);

/***** LCD number 2 Specific functions *****/
void display_main_screen_LCD2(float temp, char temp_unit);

#endif /* FINAL_PROJECT_LIB_H_ */
