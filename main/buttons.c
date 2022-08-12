//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    buttons.c
//! \brief   This module provides the useful functions to use the buttons
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "driver/touch_pad.h"

#include "esp_log.h"
#include "soc/rtc.h"

#include "buttons.h"

#include "aseba_esp32.h"
#include "gpio.h"
#include "pins_def.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define TOUCH_THRESH_NO_USE              0u
#define PRESSED_THRESHOLD_PERCENT       98u
#define THRESHOLD_AVERAGE_SIZE          16u
#define DEBOUNCE                         3
#define HIST_SIZE 6
//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "buttons";

static touch_pad_t Buttons_Table[BUTTONS_NUM];

static uint8_t ButtonStatus[BUTTONS_NUM]    = {0u, 0u, 0u, 0u, 0u};
static uint16_t ButtonFiltered[BUTTONS_NUM] = {0u, 0u, 0u, 0u, 0u};
static uint16_t ButtonRaw[BUTTONS_NUM]      = {0u, 0u, 0u, 0u, 0u};
static int16_t Threshold[BUTTONS_NUM]      = {0u, 0u, 0u, 0u, 0u};
static uint16_t Sum[BUTTONS_NUM]            = {0u, 0u, 0u, 0u, 0u};
static int16_t Count[BUTTONS_NUM]           = {-DEBOUNCE, -DEBOUNCE, -DEBOUNCE, -DEBOUNCE, -DEBOUNCE};
//static int16_t count2 = 0;
static uint16_t ButtonRawHist[BUTTONS_NUM][HIST_SIZE] = {0u};
static int16_t ButtonDelta[BUTTONS_NUM]      = {0u, 0u, 0u, 0u, 0u};
static int16_t ThresholdLow[BUTTONS_NUM]      = {0u, 0u, 0u, 0u, 0u};
//static int16_t ThresholdHigh[BUTTONS_NUM]      = {0u, 0u, 0u, 0u, 0u};
int16_t DeltaMin[BUTTONS_NUM] = {0, 0, 0, 0, 0};
//int16_t DeltaMax[BUTTONS_NUM] = {0, 0, 0, 0, 0};
uint8_t init_thr_flag = 1;

static const T_GpioPinConfig PinConfig = {BUTTON_SIDE_PIN, E_GpioMode_Input, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_FallingEdge};

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the touch pad
//! \pre       First initialize the buttons
//! \param     None
//! \return    None
static void InitTouchPad();

//! \brief     Initialize the detection threshold
//! \pre       First initialize the buttons
//! \param     None
//! \return    None
static void InitThresholds(void);

//! \brief     Update the detection threshold
//! \pre       First initialize the buttons
//! \param     button - Button on which the threshold is updated
//! \return    None
static void UpdateThresholds(uint8_t button);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Buttons_Init(void)
{
  // Configure the GPIO of the side button
  Gpio_ConfigurePin(&PinConfig);

  // Initialize touch pad peripheral, it will start a timer to run a filter
  touch_pad_init();
  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER); // To start an hardware timer that automatically read the touch channels.
  	  	  	  	  	  	  	  	  	  	  	  	  // The sleep time between each reading is defined by the function "touch_pad_set_meas_time", by default is about 27 ms.

  // Set reference voltage for charging/discharging: measured in oscilloscope: 0.6V-2.2V
  touch_pad_set_voltage(TOUCH_HVOLT_2V6, TOUCH_LVOLT_0V6, TOUCH_HVOLT_ATTEN_0V5);

  // Init touch pad IO
  InitTouchPad();

  // Set initial threshold. Do not init the thresholds here because the values becomes more noisy after all tasks are started.
  // This is due probably to the measuring time duration that is not precise at each cycle (disturbed by other tasks).
  //InitThresholds();

  //uint16_t sleep_cycle = 0;
  //uint16_t meas_cycle = 0;
  //touch_pad_get_meas_time(&sleep_cycle, &meas_cycle); // By default measure time = 4 ms, sleep time = about 27 ms
  //printf("sleep cycle = %d, meas cycle = %d, freq = %d", sleep_cycle, meas_cycle, rtc_clk_slow_freq_get_hz());
  // The RTC clock is 150 KHz => sleep time: 1000/150000*4096=27 ms
  // Measure time (clock is 8 MHz): 1000/8000000*16383=2ms
  //touch_pad_set_meas_time(0x1000, 0x3FFF); // Set measure time = 2 ms, sleep time = about 27 ms

  /*
  // By default the slope = 7 (fastest) charge/discharge speed.
  touch_cnt_slope_t slope;
  touch_tie_opt_t tie;
  touch_pad_get_cnt_mode(TOUCH_PAD_NUM2, &slope, &tie);
  printf("back: slope=%d, tie=%d\n", slope, tie);
  touch_pad_get_cnt_mode(TOUCH_PAD_NUM5, &slope, &tie);
  printf("left: slope=%d, tie=%d\n", slope, tie);
  touch_pad_get_cnt_mode(TOUCH_PAD_NUM7, &slope, &tie);
  printf("center: slope=%d, tie=%d\n", slope, tie);
  touch_pad_get_cnt_mode(TOUCH_PAD_NUM9, &slope, &tie);
  printf("fw: slope=%d, tie=%d\n", slope, tie);
  touch_pad_get_cnt_mode(TOUCH_PAD_NUM8, &slope, &tie);
  printf("right: slope=%d, tie=%d\n", slope, tie);
  */

  ESP_LOGI(Tag, "Buttons are initialized");
}

//_____________________________________________________________________________

uint8_t* Buttons_GetStatus(void)
{
  return ButtonStatus;
}

//_____________________________________________________________________________

void Buttons_UpdateStatus(void)
{
  static uint8_t oldButtonStatus[BUTTONS_NUM] = {0u, 0u, 0u, 0u, 0u};
  static int i = 0;

  if(init_thr_flag == 1) { // Make the calibration here at the first polling request because the values are more reliable.
	  InitThresholds();
	  init_thr_flag = 0;
  }

  for (uint8_t button = 0u; button < BUTTONS_NUM; button++)
  {
    touch_pad_read(Buttons_Table[button], &ButtonRaw[button]); // This function is blocking: it starts a new measurement and wait until is done (about 2 ms).
    // Apply a low pass filter on the count value: new filt = prev filt * 0.75 + new raw * 0.25
    ButtonFiltered[button] = ButtonFiltered[button] - (ButtonFiltered[button]>>2) + (ButtonRaw[button]>>2);

    // Shift left the history
    for(i=0; i<HIST_SIZE-1; i++) {
    	ButtonRawHist[button][i] = ButtonRawHist[button][i+1];
    }
    ButtonRawHist[button][HIST_SIZE-1] = ButtonFiltered[button]; // Add new value to the history

    // Make the sum of the differences to highlight peaks on count values (used to detect a press):
    // delta = (filt_i - filt_i-1) + (filt_i - filt_i-2) + (filt_i - filt_i-3) + (filt_i - filt_i-4) + (filt_i - filt_i-5)
    ButtonDelta[button] = 0;
    for(i=0; i<HIST_SIZE-1; i++) {
    	ButtonDelta[button] += (ButtonRawHist[button][HIST_SIZE-1] - ButtonRawHist[button][i]);
    }

    // The button is pressed
    if (ButtonDelta[button] < ThresholdLow[button])
    {
      ButtonStatus[button] = 1u;

      // Only if the button was previously not pressed
      if (ButtonStatus[button] != oldButtonStatus[button])
      {
        SET_EVENT(button);
        //printf("Pressed %d: raw=%d, delta=%d, thr=%d\n", button, ButtonRaw[button], ButtonDelta[button], ThresholdLow[button]);
        //printf("Hist: %d,%d,%d,%d,%d,%d\n", ButtonRawHist[button][0], ButtonRawHist[button][1], ButtonRawHist[button][2], ButtonRawHist[button][3], ButtonRawHist[button][4], ButtonRawHist[button][5]);
      }

      Sum[button] = 0u;
      Count[button] = -DEBOUNCE;
    }
    if (ButtonFiltered[button] > Threshold[button])
    {
      ButtonStatus[button] = 0u;

      //if (ButtonStatus[button] != oldButtonStatus[button]) {
    	//  printf("Released %d: raw=%d, delta=%d, thr=%d\n", button, ButtonRaw[button], ButtonDelta[button], ThresholdHigh[button]);
    	//  printf("Hist: %d,%d,%d,%d,%d,%d\n", ButtonRawHist[button][0], ButtonRawHist[button][1], ButtonRawHist[button][2], ButtonRawHist[button][3], ButtonRawHist[button][4], ButtonRawHist[button][5]);
      //}

      // The threshold is updated only when the button is not pressed
      UpdateThresholds(button);
    }

    oldButtonStatus[button] = ButtonStatus[button];

    vmVariables.buttons_state[button]     = (int16_t)ButtonStatus[button];
    vmVariables.buttons[button]           = (int16_t)ButtonRaw[button];
    vmVariables.buttons_mean[button]      = (int16_t)ButtonFiltered[button];
    vmVariables.buttons_threshold[button] = (int16_t)Threshold[button];

  }

  /*
  // print some debug information at a lower frequency than polling frequency
  count2++;
  if(count2 >= 30) {
	  count2 = 0;
	  //printf("raw: %d,%d,%d,%d,%d\n", ButtonRaw[0], ButtonRaw[1], ButtonRaw[2], ButtonRaw[3], ButtonRaw[4]);
	  //printf("filt: %d,%d,%d,%d,%d\n", ButtonFiltered[0], ButtonFiltered[1], ButtonFiltered[2], ButtonFiltered[3], ButtonFiltered[4]);
	  //printf("thrs: %d,%d,%d,%d,%d\n", Threshold[0], Threshold[1], Threshold[2], Threshold[3], Threshold[4]);
	  //printf("thrs: %d,%d,%d,%d,%d\n", ThresholdLow[0], ThresholdLow[1], ThresholdLow[2], ThresholdLow[3], ThresholdLow[4]);
	  //printf("delta: %d,%d,%d,%d,%d\n", ButtonDelta[0], ButtonDelta[1], ButtonDelta[2], ButtonDelta[3], ButtonDelta[4]);
  }
  */

}

//_____________________________________________________________________________

static void InitTouchPad()
{
  Buttons_Table[E_Button_Backward] = TOUCH_PAD_NUM2;  // GPIO2
  Buttons_Table[E_Button_Left]     = TOUCH_PAD_NUM5;  // GPIO12
  Buttons_Table[E_Button_Center]   = TOUCH_PAD_NUM7;  // GPIO27
  Buttons_Table[E_Button_Forward]  = TOUCH_PAD_NUM9;  // GPIO32
  Buttons_Table[E_Button_Right]    = TOUCH_PAD_NUM8;  // GPIO33

  for (uint8_t button = 0u; button < BUTTONS_NUM; button++)
  {
    // Initialize RTC IO and mode for touch pad
    touch_pad_config(Buttons_Table[button], TOUCH_THRESH_NO_USE);
  }
}

//_____________________________________________________________________________

static void InitThresholds(void)
{
  int16_t value;
  uint8_t i = 0, j= 0;
  uint8_t button = 0;

  // Init the filter with the average of the raw count values.
  // Moreover set the release threshold based on this average value.
  for(button=0; button<BUTTONS_NUM; button++) {
	  Sum[button] = 0;
	  for(j=0; j<THRESHOLD_AVERAGE_SIZE; j++) {
		  touch_pad_read(Buttons_Table[button], &ButtonRawHist[button][0]); // This function is blocking: it starts a new measurement and wait until is done (about 2 ms).
		  Sum[button] += ButtonRawHist[button][0];
		  vTaskDelay(1 / portTICK_PERIOD_MS); // Wait a bit to give time to other tasks...needed?
	  }
	  ButtonFiltered[button] = Sum[button]>>4;
	  Threshold[button] = ((ButtonFiltered[button]) * PRESSED_THRESHOLD_PERCENT / 100u);
	  Sum[button] = 0;
  }

  // Init the history with the filtered data for each touch channel.
  for(i=0; i<HIST_SIZE; i++) {
	  touch_pad_read(Buttons_Table[0], &ButtonRaw[0]);
	  touch_pad_read(Buttons_Table[1], &ButtonRaw[1]);
	  touch_pad_read(Buttons_Table[2], &ButtonRaw[2]);
	  touch_pad_read(Buttons_Table[3], &ButtonRaw[3]);
	  touch_pad_read(Buttons_Table[4], &ButtonRaw[4]);
	  ButtonFiltered[0] = ButtonFiltered[0] - (ButtonFiltered[0]>>2) + (ButtonRaw[0]>>2); // Low pass filter on the count value: new filt = prev filt * 0.75 + new raw * 0.25
	  ButtonFiltered[1] = ButtonFiltered[1] - (ButtonFiltered[1]>>2) + (ButtonRaw[1]>>2); // Low pass filter on the count value: new filt = prev filt * 0.75 + new raw * 0.25
	  ButtonFiltered[2] = ButtonFiltered[2] - (ButtonFiltered[2]>>2) + (ButtonRaw[2]>>2); // Low pass filter on the count value: new filt = prev filt * 0.75 + new raw * 0.25
	  ButtonFiltered[3] = ButtonFiltered[3] - (ButtonFiltered[3]>>2) + (ButtonRaw[3]>>2); // Low pass filter on the count value: new filt = prev filt * 0.75 + new raw * 0.25
	  ButtonFiltered[4] = ButtonFiltered[4] - (ButtonFiltered[4]>>2) + (ButtonRaw[4]>>2); // Low pass filter on the count value: new filt = prev filt * 0.75 + new raw * 0.25
	  ButtonRawHist[0][i] = ButtonFiltered[0];
	  ButtonRawHist[1][i] = ButtonFiltered[1];
	  ButtonRawHist[2][i] = ButtonFiltered[2];
	  ButtonRawHist[3][i] = ButtonFiltered[3];
	  ButtonRawHist[4][i] = ButtonFiltered[4];
	  vTaskDelay(1 / portTICK_PERIOD_MS); // Wait a bit to give time to other tasks...needed?
  }

  //for (button = 0u; button < BUTTONS_NUM; button++) {
  //  printf("Hist %d: %d,%d,%d,%d,%d,%d\n", button, ButtonRawHist[button][0], ButtonRawHist[button][1], ButtonRawHist[button][2], ButtonRawHist[button][3], ButtonRawHist[button][4], ButtonRawHist[button][5]);
  //}

  // Measure the noise in the data to set then the threshold for the button press detection.
  for(j=0; j<10; j++) {
	  for (button = 0u; button < BUTTONS_NUM; button++) {
		  value = 0;
		  for(i=0; i<HIST_SIZE-1; i++) {
			value += (ButtonRawHist[button][HIST_SIZE-1] - ButtonRawHist[button][i]);
		  }
		  if(value < DeltaMin[button]) {
			  DeltaMin[button] = value;
		  }
		  //if(value > DeltaMax[button]) {
		  //  DeltaMax[button] = value;
		  //}
		  //printf("Hist %d: %d,%d,%d,%d,%d,%d\n", button, ButtonRawHist[button][0], ButtonRawHist[button][1], ButtonRawHist[button][2], ButtonRawHist[button][3], ButtonRawHist[button][4], ButtonRawHist[button][5]);
	  }
	  touch_pad_read(Buttons_Table[0], &ButtonRaw[0]);
	  touch_pad_read(Buttons_Table[1], &ButtonRaw[1]);
	  touch_pad_read(Buttons_Table[2], &ButtonRaw[2]);
	  touch_pad_read(Buttons_Table[3], &ButtonRaw[3]);
	  touch_pad_read(Buttons_Table[4], &ButtonRaw[4]);
	  for(i=0; i<HIST_SIZE-1; i++) {
	  	ButtonRawHist[0][i] = ButtonRawHist[0][i+1];
	  	ButtonRawHist[1][i] = ButtonRawHist[1][i+1];
	  	ButtonRawHist[2][i] = ButtonRawHist[2][i+1];
	  	ButtonRawHist[3][i] = ButtonRawHist[3][i+1];
	  	ButtonRawHist[4][i] = ButtonRawHist[4][i+1];
	  }
	  ButtonFiltered[0] = ButtonFiltered[0] - (ButtonFiltered[0]>>2) + (ButtonRaw[0]>>2); // New filt = prev filt * 0.75 + new raw * 0.25
	  ButtonFiltered[1] = ButtonFiltered[1] - (ButtonFiltered[1]>>2) + (ButtonRaw[1]>>2); // New filt = prev filt * 0.75 + new raw * 0.25
	  ButtonFiltered[2] = ButtonFiltered[2] - (ButtonFiltered[2]>>2) + (ButtonRaw[2]>>2); // New filt = prev filt * 0.75 + new raw * 0.25
	  ButtonFiltered[3] = ButtonFiltered[3] - (ButtonFiltered[3]>>2) + (ButtonRaw[3]>>2); // New filt = prev filt * 0.75 + new raw * 0.25
	  ButtonFiltered[4] = ButtonFiltered[4] - (ButtonFiltered[4]>>2) + (ButtonRaw[4]>>2); // New filt = prev filt * 0.75 + new raw * 0.25
	  ButtonRawHist[0][HIST_SIZE-1] = ButtonFiltered[0];
	  ButtonRawHist[1][HIST_SIZE-1] = ButtonFiltered[1];
	  ButtonRawHist[2][HIST_SIZE-1] = ButtonFiltered[2];
	  ButtonRawHist[3][HIST_SIZE-1] = ButtonFiltered[3];
	  ButtonRawHist[4][HIST_SIZE-1] = ButtonFiltered[4];
	  vTaskDelay(1 / portTICK_PERIOD_MS); // Wait a bit to give time to other tasks...needed?
  }

  // Remove 3% of the average count value from the max noise (negative value) to get the press threshold.
  ThresholdLow[0] = DeltaMin[0] - (ButtonRawHist[0][HIST_SIZE-1]*3/100);
  ThresholdLow[1] = DeltaMin[1] - (ButtonRawHist[1][HIST_SIZE-1]*3/100);
  ThresholdLow[2] = DeltaMin[2] - (ButtonRawHist[2][HIST_SIZE-1]*3/100);
  ThresholdLow[3] = DeltaMin[3] - (ButtonRawHist[3][HIST_SIZE-1]*3/100);
  ThresholdLow[4] = DeltaMin[4] - (ButtonRawHist[4][HIST_SIZE-1]*3/100);

  // Add 3% of the average count value to the max noise (positive value) to get the release threshold.
  //ThresholdHigh[0] = DeltaMax[0] + (ButtonRawHist[0][HIST_SIZE-1]*3/100);
  //ThresholdHigh[1] = DeltaMax[1] + (ButtonRawHist[1][HIST_SIZE-1]*3/100);
  //ThresholdHigh[2] = DeltaMax[2] + (ButtonRawHist[2][HIST_SIZE-1]*3/100);
  //ThresholdHigh[3] = DeltaMax[3] + (ButtonRawHist[3][HIST_SIZE-1]*3/100);
  //ThresholdHigh[4] = DeltaMax[4] + (ButtonRawHist[4][HIST_SIZE-1]*3/100);
  // This threshold for detecting the release is not working well, thus we use the 98% of the average count value to detect a release.

  //printf("delta min: %d,%d,%d,%d,%d\n", DeltaMin[0], DeltaMin[1], DeltaMin[2], DeltaMin[3], DeltaMin[4]);
  //printf("ThrLow: %d,%d,%d,%d,%d\n", ThresholdLow[0], ThresholdLow[1], ThresholdLow[2], ThresholdLow[3], ThresholdLow[4]);
  //printf("ThrHigh: %d,%d,%d,%d,%d\n", ThresholdHigh[0], ThresholdHigh[1], ThresholdHigh[2], ThresholdHigh[3], ThresholdHigh[4]);

}

//_____________________________________________________________________________

static void UpdateThresholds(uint8_t button)
{
  Count[button]++;

  // The first 3 samples are skipped before calculating the new threshold
  if (Count[button] > 0)
  {
    Sum[button] += ButtonFiltered[button];

    if (Count[button] == THRESHOLD_AVERAGE_SIZE)
    {
      //Threshold[button] = ((Sum[button] / THRESHOLD_AVERAGE_SIZE) * PRESSED_THRESHOLD_PERCENT / 100u);
      Threshold[button] = ((Sum[button] >> 4) * PRESSED_THRESHOLD_PERCENT / 100u);

      Sum[button] = 0u;
      Count[button] = 0u;
    }
  }

}
