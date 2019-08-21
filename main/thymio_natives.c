//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    thymio_natives.c
//! \brief   This module provides the useful functions to use natives functions
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

//#include <types/types.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "esp_log.h"

#include "aseba_esp32.h"

#include "codec.h"
#include "leds.h"
//#include "sd.h"
//#include "playback.h"
#include "behavior.h"
//#include "tone.h"
//#include "ir_prox.h"
#include "sound.h"
#include "mp3.h"
#include "i2s.h"
#include "stm32_i2c.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "thymio_natives";

const T_Note JamesBond[21];
static T_Melody MelodyAseba;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static char* _prepare_name(unsigned int n, char* buf);

static void prepare_name(unsigned int n, char* buf);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

AsebaNativeFunctionDescription AsebaNativeDescription_set_led =
{
  "_leds.set",
  "Set the led",
  {
    {1, "led"},
    {1, "brightness"},
    {0, 0}
  }
};

void set_led(AsebaVMState* vm)
{
  int led = vm->variables[AsebaNativePopArg(vm)];
  int b = vm->variables[AsebaNativePopArg(vm)];

  if ((led < 0) || (led > 39))
  {
    return;
  }

  Leds_SetSingleBrightness(led, b);
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_play =
{
  "sound.play",
  "Start playback of pN.wav",
  {
    {1, "N"},
    {0, 0},
  }
};

void sound_playback(AsebaVMState* vm)
{
  char name[13] = {'p'};
  //int number = vm->variables[AsebaNativePopArg(vm)];
  int number = vm->variables[AsebaNativePopArg(vm)];

  ESP_LOGE(Tag, "Number = %d", number);

  //uint8_t* const index = &number;
  //static uint8_t index = number;

#if 0 // FIXME
  Behavior_Disable(B_SOUND_BUTTON);
  playback_enable_event();

  if (number == -1)
  {
    play_user_sound(NULL);
  }
  else
  {
    prepare_name(number, &name[1]);
    play_user_sound(name);
  }
#endif

  Codec_StartMP3Player(number);

#if 0
  MelodyAseba.Melody = JamesBond;
  MelodyAseba.Tempo  = E_Tempo_Vivace;
  MelodyAseba.Loop   = 1;
  MelodyAseba.Size   = 21;

  if (number == 1)
  {
    xTaskCreatePinnedToCore(
      Sound_RunPlayerTask,
      "sound",       // Name of the task
      2048,          // Stack size in words
      &MelodyAseba,  // Task input parameter
      2,             // Priority of the task
      NULL,          // Task handle
      0);            // Core where the task should run
  }
#endif
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_record =
{
  "sound.record",
  "Start recording of rN.wav",
  {
    {1, "N"},
    {0, 0},
  }
};

void sound_record(AsebaVMState* vm)
{
  char name[13] = {'r'};
  int number = vm->variables[AsebaNativePopArg(vm)];
#if 0 // FIXME
  if (number == -1)
  {
    sd_stop_record();
    return;
  }
  Behavior_Disable(B_SOUND_BUTTON);

  prepare_name(number, &name[1]);
  sd_start_record(name);
#endif

  //Sound_StartRecording();
  //I2S_Record();

  //MP3_StartRecorder();
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_replay =
{
  "sound.replay",
  "Start playback of rN.wav",
  {
    {1, "N"},
    {0, 0},
  }
};

void sound_replay(AsebaVMState* vm)
{
  char name[13] = {'r'};
  int number = vm->variables[AsebaNativePopArg(vm)];
#if 0 // FIXME
  Behavior_Disable(B_SOUND_BUTTON);
  playback_enable_event();

  if (number == -1)
  {
    play_user_sound(NULL);
  }
  else
  {
    prepare_name(number, &name[1]);
    play_user_sound(name);
  }
#endif

  //Sound_StartReplaying();
  Sound_Replay();
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_duration =
{
  "sound.duration",
  "Give duration in 1/10s of rN.wav",
  {
    {1, "N"},
    {1, "duration"},
    {0, 0},
  }
};

void sound_duration(AsebaVMState* vm)
{
  char name[13] = {'r'};
  int number = vm->variables[AsebaNativePopArg(vm)];
  unsigned int durationIndex = AsebaNativePopArg(vm);
#if 0
  prepare_name(number, &name[1]);
  vm->variables[durationIndex] = sd_read_duration(name);
#endif
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_sound_system =
{
  "sound.system",
  "Start playback of system sound N",
  {
    {1, "N"},
    {0, 0},
  }
};

void sound_system(AsebaVMState* vm)
{
  char name[13] = {'s'};
  int number = vm->variables[AsebaNativePopArg(vm)];
#if 0 // FIXME
  Behavior_Disable(B_SOUND_BUTTON);
  playback_enable_event();

  if (play_sound(number))
  {
    prepare_name(number, &name[1]);
    play_user_sound(name);
  }
#endif
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_set_led_circle =
{
  "leds.circle",
  "Set circular ring leds",
  {
    {1, "l0"},
    {1, "l1"},
    {1, "l2"},
    {1, "l3"},
    {1, "l4"},
    {1, "l5"},
    {1, "l6"},
    {1, "l7"},
    {0, 0},
  }
};

void set_led_circle(AsebaVMState* vm)
{
  int l1 = vm->variables[AsebaNativePopArg(vm)];
  int l2 = vm->variables[AsebaNativePopArg(vm)];
  int l3 = vm->variables[AsebaNativePopArg(vm)];
  int l4 = vm->variables[AsebaNativePopArg(vm)];
  int l5 = vm->variables[AsebaNativePopArg(vm)];
  int l6 = vm->variables[AsebaNativePopArg(vm)];
  int l7 = vm->variables[AsebaNativePopArg(vm)];
  int l8 = vm->variables[AsebaNativePopArg(vm)];

  Behavior_Disable(B_LEDS_ACC);

  Leds_SetCircleBrightness(l1, l2, l3, l4, l5, l6, l7, l8);
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_set_led_rgb_fl =
{
  "leds.front.left",
  "Set RGB front left led",
  {
    {1, "red"},
    {1, "green"},
    {1, "blue"},
    {0, 0},
  }
};

void set_rgb_fl(AsebaVMState* vm)
{
  int r = vm->variables[AsebaNativePopArg(vm)];
  int g = vm->variables[AsebaNativePopArg(vm)];
  int b = vm->variables[AsebaNativePopArg(vm)];

  Leds_SetFrontLeftBrightness(r, g, b);
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_set_led_rgb_fr =
{
  "leds.front.right",
  "Set RGB front right led",
  {
    {1, "red"},
    {1, "green"},
    {1, "blue"},
    {0, 0},
  }
};

void set_rgb_fr(AsebaVMState* vm)
{
  int r = vm->variables[AsebaNativePopArg(vm)];
  int g = vm->variables[AsebaNativePopArg(vm)];
  int b = vm->variables[AsebaNativePopArg(vm)];

  Leds_SetFrontRightBrightness(r, g, b);
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_set_led_rgb_bl =
{
  "leds.back.left",
  "Set RGB back left led",
  {
    {1, "red"},
    {1, "green"},
    {1, "blue"},
    {0, 0},
  }
};

void set_rgb_bl(AsebaVMState* vm)
{
  int r = vm->variables[AsebaNativePopArg(vm)];
  int g = vm->variables[AsebaNativePopArg(vm)];
  int b = vm->variables[AsebaNativePopArg(vm)];

  Leds_SetBackLeftBrightness(r, g, b);
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_set_led_rgb_br =
{
  "leds.back.right",
  "Set RGB back right led",
  {
    {1, "red"},
    {1, "green"},
    {1, "blue"},
    {0, 0},
  }
};

void set_rgb_br(AsebaVMState* vm)
{
  int r = vm->variables[AsebaNativePopArg(vm)];
  int g = vm->variables[AsebaNativePopArg(vm)];
  int b = vm->variables[AsebaNativePopArg(vm)];

  Leds_SetBackRightBrightness(r, g, b);
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_set_led_buttons =
{
  "leds.buttons",
  "Set buttons leds",
  {
    {1, "l0"},
    {1, "l1"},
    {1, "l2"},
    {1, "l3"},
    {0, 0},
  }
};

void set_buttons_leds(AsebaVMState* vm)
{
  int l1 = vm->variables[AsebaNativePopArg(vm)];
  int l2 = vm->variables[AsebaNativePopArg(vm)];
  int l3 = vm->variables[AsebaNativePopArg(vm)];
  int l4 = vm->variables[AsebaNativePopArg(vm)];

  Behavior_Disable(B_LEDS_BUTTON);

  Leds_SetSingleBrightness(E_Led_Button_Forward, l1);
  Leds_SetSingleBrightness(E_Led_Button_Right, l2);
  Leds_SetSingleBrightness(E_Led_Button_Backward, l3);
  Leds_SetSingleBrightness(E_Led_Button_Left, l4);
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_set_hprox_leds =
{
  "leds.prox.h",
  "Set horizontal proximity leds",
  {
    {1, "l0"},
    {1, "l1"},
    {1, "l2"},
    {1, "l3"},
    {1, "l4"},
    {1, "l5"},
    {1, "l6"},
    {1, "l7"},
    {0, 0},
  }
};

void set_hprox_leds(AsebaVMState* vm)
{
  int l1 = vm->variables[AsebaNativePopArg(vm)];
  int l2 = vm->variables[AsebaNativePopArg(vm)];
  int l3 = vm->variables[AsebaNativePopArg(vm)];
  int l4 = vm->variables[AsebaNativePopArg(vm)];
  int l5 = vm->variables[AsebaNativePopArg(vm)];
  int l6 = vm->variables[AsebaNativePopArg(vm)];
  int l7 = vm->variables[AsebaNativePopArg(vm)];
  int l8 = vm->variables[AsebaNativePopArg(vm)];

  Behavior_Disable(B_LEDS_PROX);

  // TODO send to STM32
  //Leds_SetProxIRBrightness(l1, l2, l3, l4, l5, l6, l7, l8);

  // FIXME STM32_UpdateProxIRLedsBrightness(l1, l2, l3, l4, l5, l6, l7, l8);
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_set_vprox_leds =
{
  "leds.prox.v",
  "Set vertical proximity leds",
  {
    {1, "l0"},
    {1, "l1"},
    {0, 0},
  }
};

void set_vprox_leds(AsebaVMState* vm)
{
  int l1 = vm->variables[AsebaNativePopArg(vm)];
  int l2 = vm->variables[AsebaNativePopArg(vm)];

  Behavior_Disable(B_LEDS_PROX);

#if 0 // TODO Send to STM32
  Leds_SetSingleBrightness(E_Led_Ground_IR_0, l1);
  Leds_SetSingleBrightness(E_Led_Ground_IR_1, l2);
#endif

  // FIXME STM32_UpdateGroundIRLedsBrightness(l1, l2);
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_set_rc_leds =
{
  "leds.rc",
  "Set rc led",
  {
    {1, "led"},
    {0, 0},
  }
};

void set_rc_leds(AsebaVMState* vm)
{
  int l1 = vm->variables[AsebaNativePopArg(vm)];
#if 0 // FIXME
  Behavior_Disable(B_LEDS_RC5);
#endif
  Leds_SetSingleBrightness(E_Led_RC5, l1);
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_set_sound_leds =
{
  "leds.sound",
  "Set sound led",
  {
    {1, "led"},
    {0, 0},
  }
};

void set_sound_leds(AsebaVMState* vm)
{
  int l1 = vm->variables[AsebaNativePopArg(vm)];

  Behavior_Disable(B_LEDS_MIC);

#if 0 // TODO Send to STM32
  Leds_SetSingleBrightness(E_Led_Sound, l1);
#endif
  // FIXME STM32_UpdateMicrophoneLedBrightness(l1);
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_set_ntc_leds =
{
  "leds.temperature",
  "Set ntc led",
  {
    {1, "red"},
    {1, "blue"},
    {0, 0},
  }
};

void set_ntc_leds(AsebaVMState* vm)
{
  int l1 = vm->variables[AsebaNativePopArg(vm)];
  int l2 = vm->variables[AsebaNativePopArg(vm)];
#if 0 // FIXME
  Behavior_Disable(B_LEDS_TEMPERATURE);
#endif
  //Leds_SetSingleBrightness(E_Led_Temp_Red, l1);
  //Leds_SetSingleBrightness(E_Led_Temp_Blue, l2);
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_play_freq =
{
  "sound.freq",
  "Play frequency",
  {
    {1, "Hz"},
    {1, "ds"},
    {0, 0},
  }
};

void play_freq(AsebaVMState* vm)
{
  int freq = vm->variables[AsebaNativePopArg(vm)];
  int time = vm->variables[AsebaNativePopArg(vm)];
#if 0 // FIXME
  Behavior_Disable(B_SOUND_BUTTON);

  playback_enable_event();

  play_frequency(freq, time);
#endif
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_set_wave =
{
  "sound.wave",
  "Set the primary wave of the tone generator",
  {
    {142, "wave"},   // FIXME First parameter is WAVEFORM_SIZE instead of 142
    {0, 0},
  }
};

void set_wave(AsebaVMState* vm)
{
  int* wave = (int*) vm->variables + AsebaNativePopArg(vm);
#if 0
  int i;
  for (i = 0; i < 142; i++)   // FIXME i < WAVEFORM_SIZE instead of 142
  {
    if (wave[i] > 127 || wave[i] < -128)
    {
      AsebaVMEmitNodeSpecificError(vm, "Error: samples must be in [-128,127] range");
      return;
    }
  }

  tone_set_waveform(wave);
#endif
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_prox_network =
{
  "prox.comm.enable",
  "Enable or disable the proximity communication",
  {
    {1, "state"},
    {0, 0},
  }
};

void prox_network(AsebaVMState* vm)
{
  int enable = vm->variables[AsebaNativePopArg(vm)];
#if 0
  if (enable)
  {
    prox_enable_network();
  }
  else
  {
    prox_disable_network();
  }
#endif
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_sd_open =
{
  "sd.open",
  "Open a file on the SD card",
  {
    {1, "number"},
    {1, "status"},
    {0, 0},
  }
};

void thymio_native_sd_open(AsebaVMState* vm)
{
  int no = vm->variables[AsebaNativePopArg(vm)];
  unsigned int status = AsebaNativePopArg(vm);
  char name[13] = {'u'};
  char* p;
#if 0 // FIXME

  if (no == -1)
  {
    sd_user_open(NULL);
    vm->variables[status] = 0;
  }
  else
  {
    p = _prepare_name(no, &name[1]);
    *p++ = '.';
    *p++ = 'd';
    *p++ = 'a';
    *p++ = 't';
    *p++ = 0;
    vm->variables[status] = sd_user_open(name);
  }
#endif
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_sd_write =
{
  "sd.write",
  "Write data to the opened file",
  {
    {-1, "data"},
    {1, "written"},
    {0, 0},
  }
};

void thymio_native_sd_write(AsebaVMState* vm)
{
#if 0
  // variable pos
  unsigned char* data = (unsigned char*)(vm->variables + AsebaNativePopArg(vm));
  uint16_t status = AsebaNativePopArg(vm);

  // variable size
  uint16_t length = AsebaNativePopArg(vm) * 2;

  vm->variables[status] = sd_user_write(data, length) / 2;
#endif
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_sd_read =
{
  "sd.read",
  "Read data from the opened file",
  {
    {-1, "data"},
    {1, "read"},
    {0, 0},
  }
};

void thymio_native_sd_read(AsebaVMState* vm)
{
#if 0
  // variable pos
  unsigned char* data = (unsigned char*)(vm->variables + AsebaNativePopArg(vm));
  uint16_t status = AsebaNativePopArg(vm);

  // variable size
  uint16_t length = AsebaNativePopArg(vm) * 2;

  vm->variables[status] = sd_user_read(data, length) / 2;
#endif
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_sd_seek =
{
  "sd.seek",
  "Seek the opened file",
  {
    {1, "position"},
    {1, "status"},
    {0, 0},
  }
};

void thymio_native_sd_seek(AsebaVMState* vm)
{
#if 0
  unsigned long seek =  vm->variables[AsebaNativePopArg(vm)];
  unsigned int status = AsebaNativePopArg(vm);

  vm->variables[status] = sd_user_seek(seek * 2);
#endif
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_rf_nodeid =
{
  "_rf.nodeid",
  "Set Wireless Node ID",
  {
    {1, "nodeID"},
    {0, 0},
  }
};

void set_rf_nodeid(AsebaVMState* vm)
{
#if 0
  unsigned int nodeid = vm->variables[AsebaNativePopArg(vm)];
  rf_set_node_id(nodeid);
  rf_flash_setting();
#endif
}

//_____________________________________________________________________________

static char* _prepare_name(unsigned int n, char* buf)
{
  unsigned int div;
  unsigned int z = 0;
  for (div = 10000; div > 0; div /= 10)
  {
    unsigned int disp = n / div;
    n %= div;
    if ((disp != 0) || (z) || (div == 1))
    {
      z = 1;
      *buf++ = '0' + disp;
    }
  }
  return buf;
}

//_____________________________________________________________________________

static void prepare_name(unsigned int n, char* buf)
{
  buf = _prepare_name(n, buf);

  *buf++ = '.';
  *buf++ = 'w';
  *buf++ = 'a';
  *buf++ = 'v';
  *buf++ = 0;
}
