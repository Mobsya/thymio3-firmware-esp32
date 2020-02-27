//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    thymio_natives.h
//! \brief   This module provides the useful functions to use natives functions
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef THYMIO_NATIVES_H_
#define THYMIO_NATIVES_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "aseba/vm/natives.h"

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
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

extern AsebaNativeFunctionDescription AsebaNativeDescription_record_wav;
void record_wav(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_create_wav;
void create_wav(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_play_mp3_sys;
void play_mp3_sys(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_play_mp3;
void play_mp3(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_play_wav;
void play_wav(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_pause_mp3_sys;
void pause_mp3_sys(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_pause_mp3;
void pause_mp3(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_pause_wav;
void pause_wav(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_resume_mp3_sys;
void resume_mp3_sys(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_resume_mp3;
void resume_mp3(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_resume_wav;
void resume_wav(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_erase_mp3;
void erase_mp3(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_erase_wav;
void erase_wav(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_get_duration_mp3;
void get_duration_mp3(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_get_duration_wav;
void get_duration_wav(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_volume;
void sound_volume(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_sound_system;
void sound_system(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_duration;
void sound_duration(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_erase_file;
void erase_file(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_set_led;
void set_led(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_set_led_circle;
void set_led_circle(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_set_led_rgb_fl;
void set_rgb_fl(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_set_led_rgb_fr;
void set_rgb_fr(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_set_led_rgb_bl;
void set_rgb_bl(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_set_led_rgb_br;
void set_rgb_br(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_play_freq;
void play_freq(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_set_led_buttons;
void set_buttons_leds(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_set_hprox_leds;
void set_hprox_leds(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_set_vprox_leds;
void set_vprox_leds(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_set_rc_leds;
void set_rc_leds(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_set_sound_leds;
void set_sound_leds(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_set_wave;
void set_wave(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_prox_network;
void prox_network(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_rf_nodeid;
void set_rf_nodeid(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_gyro_reset_angle;
void gyro_reset_angle(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_gyro_reset_calib_angle;
void gyro_reset_calib_angle(AsebaVMState* vm);

extern AsebaNativeFunctionDescription AsebaNativeDescription_gyro_set_offset;
void gyro_set_offset(AsebaVMState* vm);

#define THYMIO_NATIVES_DESCRIPTIONS \
  &AsebaNativeDescription_record_wav, \
  &AsebaNativeDescription_create_wav, \
  &AsebaNativeDescription_play_mp3_sys, \
  &AsebaNativeDescription_play_mp3, \
  &AsebaNativeDescription_play_wav, \
  &AsebaNativeDescription_pause_mp3_sys, \
  &AsebaNativeDescription_pause_mp3, \
  &AsebaNativeDescription_pause_wav, \
  &AsebaNativeDescription_resume_mp3_sys, \
  &AsebaNativeDescription_resume_mp3, \
  &AsebaNativeDescription_resume_wav, \
  &AsebaNativeDescription_erase_mp3, \
  &AsebaNativeDescription_erase_wav, \
  &AsebaNativeDescription_get_duration_mp3, \
  &AsebaNativeDescription_get_duration_wav, \
  &AsebaNativeDescription_volume, \
  &AsebaNativeDescription_sound_system, \
  &AsebaNativeDescription_duration, \
  &AsebaNativeDescription_erase_file, \
  &AsebaNativeDescription_set_led, \
  &AsebaNativeDescription_set_led_circle, \
  &AsebaNativeDescription_set_led_rgb_fl, \
  &AsebaNativeDescription_set_led_rgb_fr, \
  &AsebaNativeDescription_set_led_rgb_bl, \
  &AsebaNativeDescription_set_led_rgb_br, \
  &AsebaNativeDescription_play_freq, \
  &AsebaNativeDescription_set_led_buttons, \
  &AsebaNativeDescription_set_hprox_leds, \
  &AsebaNativeDescription_set_vprox_leds, \
  &AsebaNativeDescription_set_rc_leds, \
  &AsebaNativeDescription_set_sound_leds, \
  &AsebaNativeDescription_set_wave, \
  &AsebaNativeDescription_prox_network, \
  &AsebaNativeDescription_rf_nodeid, \
  &AsebaNativeDescription_gyro_reset_angle, \
  &AsebaNativeDescription_gyro_reset_calib_angle, \
  &AsebaNativeDescription_gyro_set_offset

#define THYMIO_NATIVES_FUNCTIONS \
  record_wav, \
  create_wav, \
  play_mp3_sys, \
  play_mp3, \
  play_wav, \
  pause_mp3_sys, \
  pause_mp3, \
  pause_wav, \
  resume_mp3_sys, \
  resume_mp3, \
  resume_wav, \
  erase_mp3, \
  erase_wav, \
  get_duration_mp3, \
  get_duration_wav, \
  sound_volume, \
  sound_system, \
  sound_duration, \
  erase_file, \
  set_led, \
  set_led_circle, \
  set_rgb_fl, \
  set_rgb_fr, \
  set_rgb_bl, \
  set_rgb_br, \
  play_freq, \
  set_buttons_leds, \
  set_hprox_leds, \
  set_vprox_leds, \
  set_rc_leds, \
  set_sound_leds, \
  set_wave, \
  prox_network, \
  set_rf_nodeid, \
  gyro_reset_angle, \
  gyro_reset_calib_angle, \
  gyro_set_offset

#endif // THYMIO_NATIVES_H_

