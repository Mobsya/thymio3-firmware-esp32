//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    es8374.c
//! \brief   This module provides the useful functions to use the audio codec ES8374
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "esp_log.h"
#include "esp_err.h"

#include "es8374.h"

#include "board.h"
#include "i2c.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define ADDR_STATE                       1u  //!< ADDR state used to set the slave address
#define ES8374_ADDRESS                0x10u  //!< Device address

#define SLAVE_ADDRESS                 (ES8374_ADDRESS | ADDR_STATE)  //!< Slave address

// Registers addresses
#define RESET_REG_ADDRESS                   0x00u  //!< Reset register address                 (Read/Write)
#define CLOCK_MANAGER_A_REG_ADDRESS         0x01u  //!< Clock manager register address         (Read/Write)
#define CLOCK_MANAGER_B_REG_ADDRESS         0x02u  //!< Clock manager register address         (Read/Write)
#define CLOCK_MANAGER_C_REG_ADDRESS         0x03u  //!< Clock manager register address         (Read/Write)
#define CLOCK_MANAGER_D_REG_ADDRESS         0x04u  //!< Clock manager register address         (Read/Write)
#define CLOCK_MANAGER_E_REG_ADDRESS         0x05u  //!< Clock manager register address         (Read/Write)
#define CLOCK_MANAGER_F_REG_ADDRESS         0x06u  //!< Clock manager register address         (Read/Write)
#define CLOCK_MANAGER_G_REG_ADDRESS         0x07u  //!< Clock manager register address         (Read/Write)
#define CLOCK_MANAGER_H_REG_ADDRESS         0x08u  //!< Clock manager register address         (Read/Write)
#define CLOCK_MANAGER_I_REG_ADDRESS         0x09u  //!< Clock manager register address         (Read/Write)
#define CLOCK_MANAGER_J_REG_ADDRESS         0x0Au  //!< Clock manager register address         (Read/Write)
#define CLOCK_MANAGER_K_REG_ADDRESS         0x0Bu  //!< Clock manager register address         (Read/Write)
#define CLOCK_MANAGER_L_REG_ADDRESS         0x0Cu  //!< Clock manager register address         (Read/Write)
#define CLOCK_MANAGER_M_REG_ADDRESS         0x0Du  //!< Clock manager register address         (Read/Write)
#define CLOCK_MANAGER_N_REG_ADDRESS         0x0Eu  //!< Clock manager register address         (Read/Write)
#define SDP_A_REG_ADDRESS                   0x0Fu  //!< SDP register address                   (Read/Write)
#define SDP_B_REG_ADDRESS                   0x10u  //!< SDP register address                   (Read/Write)
#define SDP_C_REG_ADDRESS                   0x11u  //!< SDP register address                   (Read/Write)
#define SYSTEM_A_REG_ADDRESS                0x12u  //!< System register address                (Read/Write)
#define SYSTEM_B_REG_ADDRESS                0x13u  //!< System register address                (Read/Write)
#define ANALOG_REF_REG_ADDRESS              0x14u  //!< Analog reference register address      (Read/Write)
#define ANALOG_POWER_DOWN_REG_ADDRESS       0x15u  //!< Analog power down register address     (Read/Write)
#define ANALOG_LOW_POWER_DOWN_REG_ADDRESS   0x16u  //!< Analog low power mode register address (Read/Write)
#define REF_AND_POWER_MODE_REG_ADDRESS      0x17u  //!<
#define BIAS_SELECTION_REG_ADDRESS          0x18u  //!<
// Not implemented                          0x19u
#define MONO_OUT_SEL_REG_ADDRESS            0x1Au  //!<
#define MONO_OUT_GAIN_REG_ADDRESS           0x1Bu  //!<
#define MIXER_REG_ADDRESS                   0x1Cu  //!<
#define MIXER_GAIN_REG_ADDRESS              0x1Du  //!<
#define SPEAKER_A_REG_ADDRESS               0x1Eu  //!<
#define SPEAKER_B_REG_ADDRESS               0x1Fu  //!<
#define SPEAKER_C_REG_ADDRESS               0x20u  //!<
#define PGA_REG_ADDRESS                     0x21u  //!<
#define PGA_GAIN_REG_ADDRESS                0x22u  //!<
// Not implemented                          0x23u
#define ADC_CONTROL_A_REG_ADDRESS           0x24u  //!<
#define ADC_CONTROL_B_REG_ADDRESS           0x25u  //!<
#define ALC_CONTROL_A_REG_ADDRESS           0x26u  //!<
#define ALC_CONTROL_B_REG_ADDRESS           0x27u  //!<
#define ALC_CONTROL_C_REG_ADDRESS           0x28u  //!<
#define ALC_CONTROL_D_REG_ADDRESS           0x29u  //!<
#define ALC_CONTROL_E_REG_ADDRESS           0x2Au  //!<
#define ALC_CONTROL_F_REG_ADDRESS           0x2Bu  //!<
#define ADC_CONTROL_C_REG_ADDRESS           0x2Cu  //!<
#define ADC_CONTROL_D_REG_ADDRESS           0x2Du  //!<
#define ADC_CONTROL_E_REG_ADDRESS           0x2Eu  //!<
#define ADC_CONTROL_F_REG_ADDRESS           0x2Fu  //!<
#define ADC_CONTROL_G_REG_ADDRESS           0x30u  //!<
#define ADC_CONTROL_H_REG_ADDRESS           0x31u  //!<
#define ADC_CONTROL_I_REG_ADDRESS           0x32u  //!<
#define ADC_CONTROL_J_REG_ADDRESS           0x33u  //!<
#define ADC_CONTROL_K_REG_ADDRESS           0x34u  //!<
#define ADC_CONTROL_L_REG_ADDRESS           0x35u  //!<
#define DAC_CONTROL_A_REG_ADDRESS           0x36u  //!<
#define DAC_CONTROL_B_REG_ADDRESS           0x37u  //!<
#define DAC_CONTROL_C_REG_ADDRESS           0x38u  //!<
#define DAC_CONTROL_D_REG_ADDRESS           0x39u  //!<
#define DAC_CONTROL_E_REG_ADDRESS           0x3Au  //!<
#define DAC_CONTROL_F_REG_ADDRESS           0x3Bu  //!<
#define DAC_CONTROL_G_REG_ADDRESS           0x3Cu  //!<
#define DAC_CONTROL_H_REG_ADDRESS           0x3Du  //!<
#define DAC_CONTROL_I_REG_ADDRESS           0x3Eu  //!<
#define DAC_CONTROL_J_REG_ADDRESS           0x3Fu  //!<
#define DAC_CONTROL_K_REG_ADDRESS           0x40u  //!<
#define DAC_CONTROL_L_REG_ADDRESS           0x41u  //!<
#define DAC_CONTROL_M_REG_ADDRESS           0x42u  //!<
#define DAC_CONTROL_N_REG_ADDRESS           0x43u  //!<
#define DAC_CONTROL_O_REG_ADDRESS           0x44u  //!<
#define TWO_BAND_EQ_AA_REG_ADDRESS          0x45u  //!<
#define TWO_BAND_EQ_AB_REG_ADDRESS          0x46u  //!<
#define TWO_BAND_EQ_AC_REG_ADDRESS          0x47u  //!<
#define TWO_BAND_EQ_AD_REG_ADDRESS          0x48u  //!<
#define TWO_BAND_EQ_AE_REG_ADDRESS          0x49u  //!<
#define TWO_BAND_EQ_AF_REG_ADDRESS          0x4Au  //!<
#define TWO_BAND_EQ_AG_REG_ADDRESS          0x4Bu  //!<
#define TWO_BAND_EQ_AH_REG_ADDRESS          0x4Cu  //!<
#define TWO_BAND_EQ_AI_REG_ADDRESS          0x4Du  //!<
#define TWO_BAND_EQ_AJ_REG_ADDRESS          0x4Eu  //!<
#define TWO_BAND_EQ_AK_REG_ADDRESS          0x4Fu  //!<
#define TWO_BAND_EQ_AL_REG_ADDRESS          0x50u  //!<
#define TWO_BAND_EQ_AM_REG_ADDRESS          0x51u  //!<
#define TWO_BAND_EQ_AN_REG_ADDRESS          0x52u  //!<
#define TWO_BAND_EQ_AO_REG_ADDRESS          0x53u  //!<
#define TWO_BAND_EQ_AP_REG_ADDRESS          0x54u  //!<
#define TWO_BAND_EQ_AQ_REG_ADDRESS          0x55u  //!<
#define TWO_BAND_EQ_AR_REG_ADDRESS          0x56u  //!<
#define TWO_BAND_EQ_AS_REG_ADDRESS          0x57u  //!<
#define TWO_BAND_EQ_AT_REG_ADDRESS          0x58u  //!<
#define TWO_BAND_EQ_AU_REG_ADDRESS          0x59u  //!<
#define TWO_BAND_EQ_AV_REG_ADDRESS          0x5Au  //!<
#define TWO_BAND_EQ_AW_REG_ADDRESS          0x5Bu  //!<
#define TWO_BAND_EQ_AX_REG_ADDRESS          0x5Cu  //!<
#define TWO_BAND_EQ_AY_REG_ADDRESS          0x5Du  //!<
#define TWO_BAND_EQ_AZ_REG_ADDRESS          0x5Eu  //!<
#define TWO_BAND_EQ_BA_REG_ADDRESS          0x5Fu  //!<
#define TWO_BAND_EQ_BB_REG_ADDRESS          0x60u  //!<
#define TWO_BAND_EQ_BC_REG_ADDRESS          0x61u  //!<
#define TWO_BAND_EQ_BD_REG_ADDRESS          0x62u  //!<
#define TWO_BAND_EQ_BE_REG_ADDRESS          0x63u  //!<
#define TWO_BAND_EQ_BF_REG_ADDRESS          0x64u  //!<
#define TWO_BAND_EQ_BG_REG_ADDRESS          0x65u  //!<
#define TWO_BAND_EQ_BH_REG_ADDRESS          0x66u  //!<
#define TWO_BAND_EQ_BI_REG_ADDRESS          0x67u  //!<
#define TWO_BAND_EQ_BJ_REG_ADDRESS          0x68u  //!<
#define TWO_BAND_EQ_BK_REG_ADDRESS          0x69u  //!<
#define TWO_BAND_EQ_BL_REG_ADDRESS          0x6Au  //!<
#define TWO_BAND_EQ_BM_REG_ADDRESS          0x6Bu  //!<
#define TWO_BAND_EQ_BN_REG_ADDRESS          0x6Cu  //!<
#define GPIO_AND_INT_CONTROL_REG_ADDRESS    0x6Du  //!<
#define FLAGS_REG_ADDRESS                   0x6Eu  //!<

#define MIN_DAC_VOLUME                0xC0u  //!< -96dB

#define MANUFACTURER_ID               0xE0u  //!< Manufacturer ID

// DAC_CONTROL_A bits mask
#define DAC_MUTE_BIT_MASK             0xDFu  //!< Mask of bit DAC_MUTE

// SDP_B bits mask
#define ADCWL_BIT_MASK                0xE3u  //!< Mask of bits ADCWL

// SDP_C bits mask
#define DACWL_BIT_MASK                0xE3u  //!< Mask of bits DACWL

// MIXER bits mask
#define LAX2LSPKMX_BIT_MASK           0xBFu  //!< Mask of bits LAX2LSPKMX

// SDP_A bits mask
#define MSC_BIT_MASK                  0x7Fu  //!< Mask of bits MSC

// PGA bits mask
#define DF2SE_10DB_BIT_MASK           0xFBu  //!< Mask of bits DF2SE_10DB

// DAC_CONTROL_A bits position
#define DAC_MUTE_BIT_POS                 5u  //!< Position of bit RGBC_EN

// SDP_A bits position
#define MSC_BIT_POS                      7u  //!< Position of bit MSC

// SDP_B bits position
#define ADCWL_BIT_POS                    2u  //!< Position of LSB bit ADCWL

// SDP_C bits position
#define DACWL_BIT_POS                    2u  //!< Position of LSB bit DACWL

// MONO_OUT_SEL bits position
#define LOUT_MUTE_BIT_POS                3u  //!< Position of bit LOUT_MUTE

// ANALOG_POWER_DOWN bits position
#define PDN_DACL_BIT_POS                 5u  //!< Position of bit PDN_DACL

// SPEAKER_A bits position
#define LM2SPKLOUT_BIT_POS               5u  //!< Position of bit LM2SPKLOUT

// ADC_CONTROL_A bits position
#define ADCHPF_BIT_POS                   3u  //!< Position of bit ADCHPF

// PGA bits position
#define DF2SE_10DB_BIT_POS               2u  //!< Position of bit DF2SE_10DB

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

enum
{
  E_DACMute_Normal,  //!< DAC is activated
  E_DACMute_Mute     //!< DAC is muted
};
typedef uint8_t T_DACMute;  //!< DAC mute configuration

enum
{
  E_BitsPerSample_24bits,  //!< 24-bit serial audio data word length
  E_BitsPerSample_20bits,  //!< 20-bit serial audio data word length
  E_BitsPerSample_18bits,  //!< 18-bit serial audio data word length
  E_BitsPerSample_16bits,  //!< 16-bit serial audio data word length
  E_BitsPerSample_32bits   //!< 32-bit serial audio data word length
};
typedef uint8_t T_BitsPerSample;  //!< Bits per sample selection

typedef enum
{
  MCLK_DIV_MIN = -1,
  MCLK_DIV_1   = 1,
  MCLK_DIV_2   = 2,
  MCLK_DIV_3   = 3,
  MCLK_DIV_4   = 4,
  MCLK_DIV_6   = 5,
  MCLK_DIV_8   = 6,
  MCLK_DIV_9   = 7,
  MCLK_DIV_11  = 8,
  MCLK_DIV_12  = 9,
  MCLK_DIV_16  = 10,
  MCLK_DIV_18  = 11,
  MCLK_DIV_22  = 12,
  MCLK_DIV_24  = 13,
  MCLK_DIV_33  = 14,
  MCLK_DIV_36  = 15,
  MCLK_DIV_44  = 16,
  MCLK_DIV_48  = 17,
  MCLK_DIV_66  = 18,
  MCLK_DIV_72  = 19,
  MCLK_DIV_5   = 20,
  MCLK_DIV_10  = 21,
  MCLK_DIV_15  = 22,
  MCLK_DIV_17  = 23,
  MCLK_DIV_20  = 24,
  MCLK_DIV_25  = 25,
  MCLK_DIV_30  = 26,
  MCLK_DIV_32  = 27,
  MCLK_DIV_34  = 28,
  MCLK_DIV_7   = 29,
  MCLK_DIV_13  = 30,
  MCLK_DIV_14  = 31,
  MCLK_DIV_MAX = 32
} es_sclk_div_t;

typedef enum
{
  LCLK_DIV_MIN  = -1,
  LCLK_DIV_128  = 0,
  LCLK_DIV_192  = 1,
  LCLK_DIV_256  = 2,
  LCLK_DIV_384  = 3,
  LCLK_DIV_512  = 4,
  LCLK_DIV_576  = 5,
  LCLK_DIV_768  = 6,
  LCLK_DIV_1024 = 7,
  LCLK_DIV_1152 = 8,
  LCLK_DIV_1408 = 9,
  LCLK_DIV_1536 = 10,
  LCLK_DIV_2112 = 11,
  LCLK_DIV_2304 = 12,
  LCLK_DIV_125  = 16,
  LCLK_DIV_136  = 17,
  LCLK_DIV_250  = 18,
  LCLK_DIV_272  = 19,
  LCLK_DIV_375  = 20,
  LCLK_DIV_500  = 21,
  LCLK_DIV_544  = 22,
  LCLK_DIV_750  = 23,
  LCLK_DIV_1000 = 24,
  LCLK_DIV_1088 = 25,
  LCLK_DIV_1496 = 26,
  LCLK_DIV_1500 = 27,
  LCLK_DIV_MAX  = 28
} es_lclk_div_t;

typedef struct
{
  es_sclk_div_t sclk_div;    /*!< bits clock divide */
  es_lclk_div_t lclk_div;    /*!< WS clock divide */
} T_I2SClock;  //!< I2S clock configuration

enum
{
  E_I2SFormat_Normal,
  E_I2SFormat_Left,
  E_I2SFormat_Right,
  E_I2SFormat_DSP
};
typedef uint8_t T_I2SFormat;  //!< I2S format configuration

enum
{
  E_Mode_ADC     = 0x01,
  E_Mode_DAC     = 0x02,
  E_Mode_ADC_DAC = 0x03,
  E_Mode_Line    = 0x04
};
typedef uint8_t T_Mode;  // Mode selection

enum
{
  E_MicroGain_0dB,
  E_MicroGain_3dB,
  E_MicroGain_6dB,
  E_MicroGain_9dB,
  E_MicroGain_12dB,
  E_MicroGain_15dB,
  E_MicroGain_18dB,
  E_MicroGain_21dB
};
typedef uint8_t T_MicroGain;  // Microphone gain

enum
{
  E_PGAGain_Disable,
  E_PGAGain_Enable
};
typedef uint8_t T_PGAGain;  // PGA gain configuration


//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

xSemaphoreHandle I2CMutex;

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "es8374";

static bool InitFlag = false;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static esp_err_t Start(T_Mode mode);

static esp_err_t Stop(T_Mode mode);

//! \brief     Update the DAC mute
//! \pre       None
//! \param     config - The DAC mute
//! \return    None
static esp_err_t ConfigureDACMute(T_DACMute config);

static esp_err_t ConfigureI2SClock(T_I2SClock clock);

static esp_err_t ConfigureI2SFormat(T_Mode mode, uint8_t format);

static esp_err_t UpdateBitsPerSample(T_Mode mode, T_BitsPerSample number);

static esp_err_t SetADCDACVolume(T_Mode mode, int16_t volume_dB, int16_t dot);

// TODO static esp_err_t ConfigureDACOutput(es_dac_output_t output);
static esp_err_t ConfigureDACOutput(void);

// TODO static esp_err_t ConfigureADCInput(es_adc_input_t input);

static esp_err_t SetMicrophoneGain(T_MicroGain gain_dB);

static esp_err_t ConfigurePGAGain(T_PGAGain config);

static esp_err_t ConfigureClock(void);

//static esp_err_t InitRegisters(audio_hal_codec_mode_t ms_mode, uint8_t format, T_I2SClock cfg, es_dac_output_t out_channel, es_adc_input_t in_channel);
static esp_err_t InitRegisters(audio_hal_codec_mode_t ms_mode, uint8_t format, T_I2SClock cfg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

esp_err_t ES8374_Init(audio_hal_codec_config_t* cfg)
{
  esp_err_t result = ESP_OK;

  T_I2SClock clkdiv;

  ESP_LOGE(Tag, "ES8374_Init");

  if (!InitFlag)
  {
    clkdiv.lclk_div = LCLK_DIV_256;
    clkdiv.sclk_div = MCLK_DIV_4;

    // I2C shall be initialized in master mode

    result |= Stop(cfg->codec_mode);
    //result |= InitRegisters(cfg->i2s_iface.mode, ((E_BitsPerSample_16bits << 4) | cfg->i2s_iface.fmt), clkdiv,
    //                        cfg->dac_output, cfg->adc_input);
    result |= InitRegisters(cfg->i2s_iface.mode, ((E_BitsPerSample_16bits << 4) | cfg->i2s_iface.fmt), clkdiv);
    result |= SetMicrophoneGain(E_MicroGain_21dB);
    result |= ConfigurePGAGain(E_PGAGain_Enable);
    result |= ConfigureI2SFormat(cfg->codec_mode, cfg->i2s_iface.fmt);
    result |= ES8374_ConfigureI2S(cfg->codec_mode, &(cfg->i2s_iface));

    InitFlag = true;

    ESP_LOGI(Tag, "ES8374 is initialized");
  }
  else
  {
    result = ESP_FAIL;
  }

  return result;
}

//_____________________________________________________________________________

esp_err_t ES8374_Deinit(void)
{
  ESP_LOGE(Tag, "ES8374_Deinit");

  //xSemaphoreTake(I2CMutex, portMAX_DELAY);

  uint8_t data = 0x7Fu;
  I2C_WriteToAddress(SLAVE_ADDRESS, RESET_REG_ADDRESS, &data, 1u);

  //xSemaphoreGive(I2CMutex);

  InitFlag = false;

  return ESP_OK;
}

//_____________________________________________________________________________

esp_err_t ES8374_ConfigureI2S(audio_hal_codec_mode_t mode, audio_hal_codec_i2s_iface_t* iface)
{
  ESP_LOGE(Tag, "ES8374_ConfigureI2S");

  esp_err_t result = ESP_OK;
  T_BitsPerSample bitsPerSample = E_BitsPerSample_32bits;

  result |= ConfigureI2SFormat(mode, iface->fmt);

  if (iface->bits == AUDIO_HAL_BIT_LENGTH_16BITS)
  {
    bitsPerSample = E_BitsPerSample_16bits;
  }
  else if (iface->bits == AUDIO_HAL_BIT_LENGTH_24BITS)
  {
    bitsPerSample = E_BitsPerSample_24bits;
  }
  else
  {
    // Do nothing
  }

  UpdateBitsPerSample(mode, bitsPerSample);

  return result;
}

//_____________________________________________________________________________

esp_err_t ES8374_SetVoiceVolume(int volume)
{
  ESP_LOGE(Tag, "ES8374_SetVoiceVolume");

  uint8_t vol = 0;

  //uint8_t read = 0;

  if (volume < 0)
  {
    vol = MIN_DAC_VOLUME;  // Min volume = -96 [dB]
  }
  else if (volume > (MIN_DAC_VOLUME / 2))
  {
    vol = 0u;  // Max volume = 0 [dB]
  }
  else
  {
    vol = MIN_DAC_VOLUME - (volume * 2);
  }

  ESP_LOGI(Tag, "volume = %d, vol = %d", volume, vol);

  //xSemaphoreTake(I2CMutex, portMAX_DELAY);

  I2C_WriteToAddress(SLAVE_ADDRESS, DAC_CONTROL_C_REG_ADDRESS, &vol, 1u);

  //I2C_ReadFromAddress(SLAVE_ADDRESS, DAC_CONTROL_C_REG_ADDRESS, &read, 1u);
  //ESP_LOGI(Tag, "volume = %d, vol = %d, READ 0x38 VOLUME = %d", volume, vol, read);

  //xSemaphoreGive(I2CMutex);

  return ESP_OK;
}

//_____________________________________________________________________________

esp_err_t ES8374_GetVoiceVolume(int* volume)
{
  ESP_LOGE(Tag, "ES8374_GetVoiceVolume");
  uint8_t data = 0u;

  //xSemaphoreTake(I2CMutex, portMAX_DELAY);

  I2C_ReadFromAddress(SLAVE_ADDRESS, DAC_CONTROL_C_REG_ADDRESS, &data, 1u);

  //xSemaphoreGive(I2CMutex);

  *volume = ((MIN_DAC_VOLUME - data) / 2);

  if (*volume > 96)
  {
    *volume = 100;
  }

  ESP_LOGI(Tag, "VOLUME = %d", *volume);

  return ESP_OK;
}

//_____________________________________________________________________________

esp_err_t ES8374_ControlState(audio_hal_codec_mode_t mode, audio_hal_ctrl_t ctrl_state)
{
  ESP_LOGE(Tag, "ES8374_ControlState");
  esp_err_t result = ESP_OK;
  T_Mode config = E_Mode_DAC;

  switch (mode)
  {
    case AUDIO_HAL_CODEC_MODE_ENCODE:
      config  = E_Mode_ADC;
      break;
    case AUDIO_HAL_CODEC_MODE_LINE_IN:
      config  = E_Mode_Line;
      break;
    case AUDIO_HAL_CODEC_MODE_DECODE:
      config  = E_Mode_DAC;
      break;
    case AUDIO_HAL_CODEC_MODE_BOTH:
      config  = E_Mode_ADC_DAC;
      break;
    default:
      ESP_LOGW(Tag, "Codec mode not supported, default is decode mode");
      break;
  }

  if (AUDIO_HAL_CTRL_STOP == ctrl_state)
  {
    result = Stop(config);
  }
  else
  {
    result = Start(config);
    ESP_LOGD(Tag, "start default is decode mode: %d", config);
  }

  return result;
}

//_____________________________________________________________________________

static esp_err_t Start(T_Mode mode)
{
  esp_err_t result = ESP_OK;
  uint8_t data = 0x00u;
  uint8_t constant = 0x00u;

  ESP_LOGE(Tag, "Start");

  //xSemaphoreTake(I2CMutex, portMAX_DELAY);

  if (mode == E_Mode_Line)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);
    data |= 0x60u;
    data |= 0x20u;
    data &= 0xF7u;
    I2C_WriteToAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);

    I2C_ReadFromAddress(SLAVE_ADDRESS, MIXER_REG_ADDRESS, &data, 1u);
    data |= 0x40u;
    I2C_WriteToAddress(SLAVE_ADDRESS, MIXER_REG_ADDRESS, &data, 1u);

    constant = 0x02u;
    I2C_WriteToAddress(SLAVE_ADDRESS, MIXER_GAIN_REG_ADDRESS, &constant, 1u);

    constant = 0x00u;
    I2C_WriteToAddress(SLAVE_ADDRESS, SPEAKER_B_REG_ADDRESS, &constant, 1u);

    constant = 0xA0u;
    I2C_WriteToAddress(SLAVE_ADDRESS, SPEAKER_A_REG_ADDRESS, &constant, 1u);
  }

  if (mode == E_Mode_ADC || mode == E_Mode_ADC_DAC || mode == E_Mode_Line)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, PGA_REG_ADDRESS, &data, 1u);
    data &= 0x3Fu;
    I2C_WriteToAddress(SLAVE_ADDRESS, PGA_REG_ADDRESS, &data, 1u);

    I2C_ReadFromAddress(SLAVE_ADDRESS, SDP_B_REG_ADDRESS, &data, 1u);
    data &= 0x3Fu;
    I2C_WriteToAddress(SLAVE_ADDRESS, SDP_B_REG_ADDRESS, &data, 1u);
  }

  if (mode == E_Mode_DAC || mode == E_Mode_ADC_DAC || mode == E_Mode_Line)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);
    data |= 0x08u;
    I2C_WriteToAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);
    data &= 0xDFu;
    I2C_WriteToAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);

    constant = 0x12u;
    I2C_WriteToAddress(SLAVE_ADDRESS, MIXER_GAIN_REG_ADDRESS, &constant, 1u);

    constant = 0x20u;
    I2C_WriteToAddress(SLAVE_ADDRESS, SPEAKER_A_REG_ADDRESS, &constant, 1u);

    I2C_ReadFromAddress(SLAVE_ADDRESS, ANALOG_POWER_DOWN_REG_ADDRESS, &data, 1u);
    data &= 0xDFu;
    I2C_WriteToAddress(SLAVE_ADDRESS, ANALOG_POWER_DOWN_REG_ADDRESS, &data, 1u);

    I2C_ReadFromAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);
    data |= 0x20u;
    I2C_WriteToAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);
    data &= 0xF7u;
    I2C_WriteToAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);

    constant = 0x02u;
    I2C_WriteToAddress(SLAVE_ADDRESS, MIXER_GAIN_REG_ADDRESS, &constant, 1u);

    constant = 0xA0u;
    I2C_WriteToAddress(SLAVE_ADDRESS, SPEAKER_A_REG_ADDRESS, &constant, 1u);

    result |= ConfigureDACMute(E_DACMute_Normal);
  }

  //xSemaphoreGive(I2CMutex);

  return result;
}

//_____________________________________________________________________________

static esp_err_t Stop(T_Mode mode)
{
  ESP_LOGE(Tag, "Stop");

  esp_err_t result = ESP_OK;
  uint8_t data = 0x00u;
  uint8_t constant = 0x00;

  //xSemaphoreTake(I2CMutex, portMAX_DELAY);

  if (mode <= E_Mode_Line)
  {
    if (mode == E_Mode_Line)
    {
      I2C_ReadFromAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);
      data |= (1 << LOUT_MUTE_BIT_POS);  // Mute mono output level
      I2C_WriteToAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);
      data &= 0x9Fu;                      // Disable mono output and mixer output to mono output
      I2C_WriteToAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);

      constant = 0x12;
      I2C_WriteToAddress(SLAVE_ADDRESS, MIXER_GAIN_REG_ADDRESS, &constant, 1u);

      constant = 0x20;
      I2C_WriteToAddress(SLAVE_ADDRESS, SPEAKER_A_REG_ADDRESS, &constant, 1u);

      I2C_ReadFromAddress(SLAVE_ADDRESS, MIXER_REG_ADDRESS, &data, 1u);
      data &= LAX2LSPKMX_BIT_MASK;       // Disable
      I2C_WriteToAddress(SLAVE_ADDRESS, MIXER_REG_ADDRESS, &data, 1u);

      constant = 0x00u;
      I2C_WriteToAddress(SLAVE_ADDRESS, SPEAKER_B_REG_ADDRESS, &constant, 1u);
    }

    if ((mode == E_Mode_DAC) || (mode == E_Mode_ADC_DAC))
    {
      result |= ConfigureDACMute(E_DACMute_Mute);

      I2C_ReadFromAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);
      data |= (1u << LOUT_MUTE_BIT_POS);  // Mute mono output level
      I2C_WriteToAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);
      data &= 0xDFu;                      // Disable mono output
      I2C_WriteToAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &data, 1u);

      constant = 0x12u;                   // Mute mixer output level
      I2C_WriteToAddress(SLAVE_ADDRESS, MIXER_GAIN_REG_ADDRESS, &constant, 1u);

      constant = 0x20;
      I2C_WriteToAddress(SLAVE_ADDRESS, SPEAKER_A_REG_ADDRESS, &constant, 1u);


      I2C_ReadFromAddress(SLAVE_ADDRESS, ANALOG_POWER_DOWN_REG_ADDRESS, &data, 1u);
      data |= (1u << PDN_DACL_BIT_POS);   // Power down analog DAC circuits
      I2C_WriteToAddress(SLAVE_ADDRESS, ANALOG_POWER_DOWN_REG_ADDRESS, &data, 1u);
    }

    if ((mode == E_Mode_ADC) || (mode == E_Mode_ADC_DAC))
    {
      I2C_ReadFromAddress(SLAVE_ADDRESS, SDP_B_REG_ADDRESS, &data, 1u);
      data |= 0xC0u;                      // ADC SDP mute L+R
      I2C_WriteToAddress(SLAVE_ADDRESS, SDP_B_REG_ADDRESS, &data, 1u);

      I2C_ReadFromAddress(SLAVE_ADDRESS, PGA_REG_ADDRESS, &data, 1u);
      data |= 0xC0u;                      // Power down analog PGA circuits and analog ADC modulator
      I2C_WriteToAddress(SLAVE_ADDRESS, PGA_REG_ADDRESS, &data, 1u);
    }
  }
  else
  {
    result = ESP_ERR_INVALID_ARG;
  }

  //xSemaphoreGive(I2CMutex);

  return result;
}

//_____________________________________________________________________________

static esp_err_t ConfigureDACMute(T_DACMute config)
{
  ESP_LOGE(Tag, "ConfigureDACMute");

  esp_err_t result = ESP_OK;
  uint8_t data = 0x00u;

  if (config <= E_DACMute_Mute)
  {
	//xSemaphoreTake(I2CMutex, portMAX_DELAY);

    I2C_ReadFromAddress(SLAVE_ADDRESS, DAC_CONTROL_A_REG_ADDRESS, &data, 1u);

    data &= DAC_MUTE_BIT_MASK;
    data |= (config << DAC_MUTE_BIT_POS);

    I2C_WriteToAddress(SLAVE_ADDRESS, DAC_CONTROL_A_REG_ADDRESS, &data, 1u);

    //xSemaphoreGive(I2CMutex);
  }
  else
  {
    result = ESP_ERR_INVALID_ARG;
    ESP_LOGE(Tag, "Invalid DAC mute configuration: %d", config);
  }

  return result;
}

//_____________________________________________________________________________

static esp_err_t ConfigureI2SClock(T_I2SClock clock)
{
  ESP_LOGE(Tag, "ConfigureI2SClock");

  esp_err_t result = ESP_OK;
  uint8_t data = 0u;

  //xSemaphoreTake(I2CMutex, portMAX_DELAY);

  I2C_ReadFromAddress(SLAVE_ADDRESS, SDP_A_REG_ADDRESS, &data, 1u);
  data &= 0xe0;              // Slave serial port mode

  int divratio = 0;

  switch (clock.sclk_div)
  {
    case MCLK_DIV_1:
      divratio = 1;
      break;
    case MCLK_DIV_2: // = 2,
      divratio = 2;
      break;
    case MCLK_DIV_3: // = 3,
      divratio = 3;
      break;
    case MCLK_DIV_4: // = 4,
      divratio = 4;
      break;
    case MCLK_DIV_5: // = 20,
      divratio = 5;
      break;
    case MCLK_DIV_6: // = 5,
      divratio = 6;
      break;
    case MCLK_DIV_7: //  = 29,
      divratio = 7;
      break;
    case MCLK_DIV_8: // = 6,
      divratio = 8;
      break;
    case MCLK_DIV_9: // = 7,
      divratio = 9;
      break;
    case MCLK_DIV_10: // = 21,
      divratio = 10;
      break;
    case MCLK_DIV_11: // = 8,
      divratio = 11;
      break;
    case MCLK_DIV_12: // = 9,
      divratio = 12;
      break;
    case MCLK_DIV_13: // = 30,
      divratio = 13;
      break;
    case MCLK_DIV_14: // = 31
      divratio = 14;
      break;
    case MCLK_DIV_15: // = 22,
      divratio = 15;
      break;
    case MCLK_DIV_16: // = 10,
      divratio = 16;
      break;
    case MCLK_DIV_17: // = 23,
      divratio = 17;
      break;
    case MCLK_DIV_18: // = 11,
      divratio = 18;
      break;
    case MCLK_DIV_20: // = 24,
      divratio = 19;
      break;
    case MCLK_DIV_22: // = 12,
      divratio = 20;
      break;
    case MCLK_DIV_24: // = 13,
      divratio = 21;
      break;
    case MCLK_DIV_25: // = 25,
      divratio = 22;
      break;
    case MCLK_DIV_30: // = 26,
      divratio = 23;
      break;
    case MCLK_DIV_32: // = 27,
      divratio = 24;
      break;
    case MCLK_DIV_33: // = 14,
      divratio = 25;
      break;
    case MCLK_DIV_34: // = 28,
      divratio = 26;
      break;
    case MCLK_DIV_36: // = 15,
      divratio = 27;
      break;
    case MCLK_DIV_44: // = 16,
      divratio = 28;
      break;
    case MCLK_DIV_48: // = 17,
      divratio = 29;
      break;
    case MCLK_DIV_66: // = 18,
      divratio = 30;
      break;
    case MCLK_DIV_72: // = 19,
      divratio = 31;
      break;
    default:
      result = ESP_ERR_INVALID_ARG;
      break;
  }

  data |= divratio;
  I2C_WriteToAddress(SLAVE_ADDRESS, SDP_A_REG_ADDRESS, &data, 1u);

  int dacratio_l = 0;
  int dacratio_h = 0;

  switch (clock.lclk_div)
  {
    case LCLK_DIV_128:
      dacratio_l = 128 % 256;
      dacratio_h = 128 / 256;
      break;
    case LCLK_DIV_192:
      dacratio_l = 192 % 256;
      dacratio_h = 192 / 256;
      break;
    case LCLK_DIV_256:
      dacratio_l = 256 % 256;
      dacratio_h = 256 / 256;
      break;
    case LCLK_DIV_384:
      dacratio_l = 384 % 256;
      dacratio_h = 384 / 256;
      break;
    case LCLK_DIV_512:
      dacratio_l = 512 % 256;
      dacratio_h = 512 / 256;
      break;
    case LCLK_DIV_576:
      dacratio_l = 576 % 256;
      dacratio_h = 576 / 256;
      break;
    case LCLK_DIV_768:
      dacratio_l = 768 % 256;
      dacratio_h = 768 / 256;
      break;
    case LCLK_DIV_1024:
      dacratio_l = 1024 % 256;
      dacratio_h = 1024 / 256;
      break;
    case LCLK_DIV_1152:
      dacratio_l = 1152 % 256;
      dacratio_h = 1152 / 256;
      break;
    case LCLK_DIV_1408:
      dacratio_l = 1408 % 256;
      dacratio_h = 1408 / 256;
      break;
    case LCLK_DIV_1536:
      dacratio_l = 1536 % 256;
      dacratio_h = 1536 / 256;
      break;
    case LCLK_DIV_2112:
      dacratio_l = 2112 % 256;
      dacratio_h = 2112 / 256;
      break;
    case LCLK_DIV_2304:
      dacratio_l = 2304 % 256;
      dacratio_h = 2304 / 256;
      break;
    case LCLK_DIV_125:
      dacratio_l = 125 % 256;
      dacratio_h = 125 / 256;
      break;
    case LCLK_DIV_136:
      dacratio_l = 136 % 256;
      dacratio_h = 136 / 256;
      break;
    case LCLK_DIV_250:
      dacratio_l = 250 % 256;
      dacratio_h = 250 / 256;
      break;
    case LCLK_DIV_272:
      dacratio_l = 272 % 256;
      dacratio_h = 272 / 256;
      break;
    case LCLK_DIV_375:
      dacratio_l = 375 % 256;
      dacratio_h = 375 / 256;
      break;
    case LCLK_DIV_500:
      dacratio_l = 500 % 256;
      dacratio_h = 500 / 256;
      break;
    case LCLK_DIV_544:
      dacratio_l = 544 % 256;
      dacratio_h = 544 / 256;
      break;
    case LCLK_DIV_750:
      dacratio_l = 750 % 256;
      dacratio_h = 750 / 256;
      break;
    case LCLK_DIV_1000:
      dacratio_l = 1000 % 256;
      dacratio_h = 1000 / 256;
      break;
    case LCLK_DIV_1088:
      dacratio_l = 1088 % 256;
      dacratio_h = 1088 / 256;
      break;
    case LCLK_DIV_1496:
      dacratio_l = 1496 % 256;
      dacratio_h = 1496 / 256;
      break;
    case LCLK_DIV_1500:
      dacratio_l = 1500 % 256;
      dacratio_h = 1500 / 256;
      break;
    default:
      result = ESP_ERR_INVALID_ARG;
      break;
  }

  data = dacratio_h;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_F_REG_ADDRESS, &data, 1u);
  data = dacratio_l;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_G_REG_ADDRESS, &data, 1u);

  //xSemaphoreGive(I2CMutex);

  return result;
}

//_____________________________________________________________________________

static esp_err_t ConfigureI2SFormat(T_Mode mode, uint8_t format)
{
  ESP_LOGE(Tag, "ConfigureI2SFormat");

  esp_err_t result = ESP_OK;
  uint8_t data = 0;
  uint8_t fmt_tmp;
  uint8_t fmt_i2s;

  //if (format <= E_I2SFormat_DSP)
  {
    fmt_tmp = ((format & 0xF0u) >> 4);
    fmt_i2s =  format & 0x0Fu;

    //xSemaphoreTake(I2CMutex, portMAX_DELAY);

    if (mode <= E_Mode_Line)
    {
      if ((mode == E_Mode_ADC) || (mode == E_Mode_ADC_DAC))
      {
        I2C_ReadFromAddress(SLAVE_ADDRESS, SDP_B_REG_ADDRESS, &data, 1u);
        data &= 0xFCu;              // Slave serial port mode
        data |= fmt_i2s;
        I2C_WriteToAddress(SLAVE_ADDRESS, SDP_B_REG_ADDRESS, &data, 1u);
        result |= UpdateBitsPerSample(mode, fmt_tmp);
      }

      if (mode == E_Mode_DAC || mode == E_Mode_ADC_DAC)
      {
        I2C_ReadFromAddress(SLAVE_ADDRESS, SDP_C_REG_ADDRESS, &data, 1u);
        data &= 0xFCu;              // I2S serial audio data format
        data |= fmt_i2s;
        I2C_WriteToAddress(SLAVE_ADDRESS, SDP_C_REG_ADDRESS, &data, 1u);
        result |= UpdateBitsPerSample(mode, fmt_tmp);
      }
    }
    else
    {
      result = ESP_ERR_INVALID_ARG;
      ESP_LOGE(Tag, "Invalid mode: %d", mode);
    }
  }
  //else
  {
    //result = ESP_ERR_INVALID_ARG;
    //ESP_LOGE(Tag, "Invalid format: %d", format);
  }

  //xSemaphoreGive(I2CMutex);

  return result;
}

//_____________________________________________________________________________

static esp_err_t UpdateBitsPerSample(T_Mode mode, T_BitsPerSample number)
{
  ESP_LOGE(Tag, "UpdateBitsPerSample");

  esp_err_t result = ESP_OK;
  uint8_t data = 0x00u;

  //xSemaphoreTake(I2CMutex, portMAX_DELAY);

  if (mode <= E_Mode_Line)
  {
    if ((mode == E_Mode_ADC) || (mode == E_Mode_ADC_DAC))
    {
      if (number <= E_BitsPerSample_32bits)
      {
        I2C_ReadFromAddress(SLAVE_ADDRESS, SDP_B_REG_ADDRESS, &data, 1u);
        data &= ADCWL_BIT_MASK;
        data |= (number << ADCWL_BIT_POS);
        I2C_WriteToAddress(SLAVE_ADDRESS, SDP_B_REG_ADDRESS, &data, 1u);
      }
      else
      {
        result = ESP_ERR_INVALID_ARG;
        ESP_LOGE(Tag, "Invalid number of bit per sample: %d", number);
      }
    }

    if ((mode == E_Mode_DAC) || (mode == E_Mode_ADC_DAC))
    {
      if (number <= E_BitsPerSample_32bits)
      {
        I2C_ReadFromAddress(SLAVE_ADDRESS, SDP_C_REG_ADDRESS, &data, 1u);
        data &= DACWL_BIT_MASK;
        data |= (number << DACWL_BIT_POS);
        I2C_WriteToAddress(SLAVE_ADDRESS, SDP_C_REG_ADDRESS, &data, 1u);
      }
      else
      {
        result = ESP_ERR_INVALID_ARG;
        ESP_LOGE(Tag, "Invalid number of bit per sample: %d", number);
      }
    }
  }
  else
  {
    result = ESP_ERR_INVALID_ARG;
    ESP_LOGE(Tag, "Invalid mode: %d", mode);
  }

  //xSemaphoreGive(I2CMutex);

  return result;
}

//_____________________________________________________________________________

static esp_err_t SetADCDACVolume(T_Mode mode, int16_t volume_dB, int16_t dot)
{
  ESP_LOGE(Tag, "SetADCDACVolume");

  esp_err_t result = ESP_OK;
  uint8_t data[2] = {0u, 0u};
  int16_t vol = volume_dB;

  if ((volume_dB < -96) || (volume_dB > 0))
  {
    ESP_LOGW(Tag, "Volume < -96! or > 0: %d", volume_dB);

    if (volume_dB < -96)
    {
      vol = -96;
    }
    else
    {
      vol = 0;
    }
  }

  if (mode <= E_Mode_Line)
  {
    dot = (dot >= 5 ? 1 : 0);
    vol = (-vol << 1) + dot;

    data[0] = vol & 0x00FF;
    data[1] = ((vol & 0xFF00) >> 8);

    //xSemaphoreTake(I2CMutex, portMAX_DELAY);

    if ((mode == E_Mode_ADC) || (mode == E_Mode_ADC_DAC))
    {
      ESP_LOGE(Tag, "Volume_dB = %d, data = %d %d", vol, data[0], data[1]);
      I2C_WriteToAddress(SLAVE_ADDRESS, ADC_CONTROL_B_REG_ADDRESS, data, 2u);
    }

    if (mode == E_Mode_DAC || mode == E_Mode_ADC_DAC)
    {
      I2C_WriteToAddress(SLAVE_ADDRESS, DAC_CONTROL_C_REG_ADDRESS, data, 2u);
    }

    //xSemaphoreGive(I2CMutex);
  }
  else
  {
    result = ESP_ERR_INVALID_ARG;
    ESP_LOGE(Tag, "Invalid mode: %d", mode);
  }

  return result;
}

//_____________________________________________________________________________
//#if 0  // TODO
//static esp_err_t ConfigureDACOutput(es_dac_output_t output)
static esp_err_t ConfigureDACOutput(void)
{
  ESP_LOGE(Tag, "ConfigureDACOutput");

  esp_err_t result = ESP_OK;
  uint8_t data = 0x02u;
  uint8_t constant = 0x00u;

  //xSemaphoreTake(I2CMutex, portMAX_DELAY);

  constant = 0x02u;
  I2C_WriteToAddress(SLAVE_ADDRESS, MIXER_GAIN_REG_ADDRESS, &constant, 1u);

  I2C_ReadFromAddress(SLAVE_ADDRESS, MIXER_REG_ADDRESS, &data, 1u);
  data |= 0x80u;
  I2C_WriteToAddress(SLAVE_ADDRESS, MIXER_REG_ADDRESS, &data, 1u);

  constant = 0x02u;
  I2C_WriteToAddress(SLAVE_ADDRESS, MIXER_GAIN_REG_ADDRESS, &constant, 1u);

  constant = 0x00u;
  I2C_WriteToAddress(SLAVE_ADDRESS, SPEAKER_B_REG_ADDRESS, &constant, 1u);

  constant = 0xA0u;
  I2C_WriteToAddress(SLAVE_ADDRESS, SPEAKER_A_REG_ADDRESS, &constant, 1u);

  //xSemaphoreGive(I2CMutex);

  return result;
}
//#endif
//_____________________________________________________________________________
//#if 0  // TODO
//static esp_err_t ConfigureADCInput(es_adc_input_t input)
static esp_err_t ConfigureADCInput(void)
{
  ESP_LOGE(Tag, "ConfigureADCInput");

  esp_err_t result = ESP_OK;
  uint8_t data = 0x00u;

  //xSemaphoreTake(I2CMutex, portMAX_DELAY);

  I2C_ReadFromAddress(SLAVE_ADDRESS, PGA_REG_ADDRESS, &data, 1u);
  data = (data & 0xCFu) | 0x24u;
  I2C_WriteToAddress(SLAVE_ADDRESS, PGA_REG_ADDRESS, &data, 1u);

  //xSemaphoreGive(I2CMutex);

  return result;
}
//#endif
//_____________________________________________________________________________

static esp_err_t SetMicrophoneGain(T_MicroGain gain_dB)
{
  ESP_LOGE(Tag, "SetMicrophoneGain");

  esp_err_t result = ESP_OK;
  uint8_t data = 0x00u;

  //xSemaphoreTake(I2CMutex, portMAX_DELAY);

  if (gain_dB <= E_MicroGain_21dB)
  {
    data = (gain_dB | (gain_dB << 4));
    I2C_WriteToAddress(SLAVE_ADDRESS, PGA_GAIN_REG_ADDRESS, &data, 1u);
  }
  else
  {
    result = ESP_FAIL;
    ESP_LOGE(Tag, "Invalid microphone gain: %d", gain_dB);
  }

  //xSemaphoreGive(I2CMutex);

  return result;
}

//_____________________________________________________________________________

static esp_err_t ConfigurePGAGain(T_PGAGain config)
{
  ESP_LOGE(Tag, "ConfigurePGAGain");

  esp_err_t result = ESP_OK;
  uint8_t data = 0x00u;

  //xSemaphoreTake(I2CMutex, portMAX_DELAY);

  if (config <= E_PGAGain_Enable)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, PGA_REG_ADDRESS, &data, 1u);
    data &= DF2SE_10DB_BIT_MASK;
    data |= (config << DF2SE_10DB_BIT_POS);
    I2C_WriteToAddress(SLAVE_ADDRESS, PGA_REG_ADDRESS, &data, 1u);
  }
  else
  {
    result = ESP_ERR_INVALID_ARG;
    ESP_LOGE(Tag, "Invalid configuration: %d", config);
  }

  //xSemaphoreGive(I2CMutex);

  return result;
}

//_____________________________________________________________________________
#if 0
static esp_err_t ConfigureClock(void)
{
  esp_err_t result = ESP_OK;
  uint8_t constant = 0x00u;

  // FIXME unknown register
  constant = 0xA0u;
  I2C_WriteToAddress(SLAVE_ADDRESS, 0x6F, &constant, 1u);

  // FIXME unknown register
  constant = 0x41u;
  I2C_WriteToAddress(SLAVE_ADDRESS, 0x72, &constant, 1u);

  // CLOCK MANAGER I Register
  // --------------------------------------------------------------------
  // PLL_PDN     = 0... ....  Enable PLL analog
  // PLL_RB      = .0.. ....  Reset PLL digital
  // PLLDITH_MAG = ...0 00..  Dither off
  // PLLOUT_SEL  = .... ..11  VCO out divide by 2
  //               ---------
  //               0000 0011 = 0x03
  // --------------------------------------------------------------------
  constant = 0x03u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_I_REG_ADDRESS, &constant, 1u);

  // Set PLL_K[21:16]
  constant = 0x00u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_L_REG_ADDRESS, &constant, 1u);

  // Set PLL_K[15:8]
  constant = 0x00u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_M_REG_ADDRESS, &constant, 1u);

  // Set PLL_K[7:0]
  constant = 0x00u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_N_REG_ADDRESS, &constant, 1u);

  // CLOCK MANAGER J Register
  // --------------------------------------------------------------------
  // PLL_LP     = 1... ....  PLL low power mode
  // PLL_CP     = .000 ....  PLL cp gain0
  // PLL_SUPSEL = .... 10..  VDDD =3.3v
  // PLL_KVCO   = .... ..10  VCO gain2
  //              ---------
  //              1000 1010 = 0x8A
  // --------------------------------------------------------------------
  constant = 0x8Au;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_J_REG_ADDRESS, &constant, 1u);

  // CLOCK MANAGER K Register
  // --------------------------------------------------------------------
  // PLL_CAL_SHORT = 0... ....  PLL calibration 64 data
  // PLL_VCO_WAIT  = .00. ....  Wait 2 MCLK for vcoout stable when calibration
  // PLL_N         = .... 1100  Integer part of PLL frequency ratio = 12
  //              ------------
  // CLOCK_MANAGER = 0000 1100 = 0x0C
  // --------------------------------------------------------------------
  constant = 0x0Cu;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_K_REG_ADDRESS, &constant, 1u);

  // CLOCK MANAGER I Register
  // --------------------------------------------------------------------
  // PLL_PDN     = 0... ....  Enable PLL analog
  // PLL_RB      = .1.. ....  PLL digital on
  // PLLDITH_MAG = ...0 00..  Dither off
  // PLLOUT_SEL  = .... ..11  VCO out divide by 2
  //               ---------
  //               0100 0011 = 0x43
  // --------------------------------------------------------------------
  constant = 0x43u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_I_REG_ADDRESS, &constant, 1u);

  // Set ADC_OSR = 32 (0x20)
  constant = 0x20u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_C_REG_ADDRESS, &constant, 1u);

  // CLOCK MANAGER E Register
  // --------------------------------------------------------------------
  // CLK_ADC_DIV = 0001 ....  CLK_ADC_DIV = 1
  // CLK_DAC_DIV = .... 0001  CLK_DAC_DIV = 1
  //               ---------
  //               0001 0001 = 0x11
  // --------------------------------------------------------------------
  constant = 0x11u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_E_REG_ADDRESS, &constant, 1u);


  // Set Class D speaker clock divider = 32 (0x20)
  constant = 0x20u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_H_REG_ADDRESS, &constant, 1u);

  // CLOCK MANAGER B Register
  // --------------------------------------------------------------------
  // CLK_ADC_CONT   = .0.. ....  CLK_ADC flex
  // CLK_ADC_DOUBLE = ..0. ....  clk_adc control normal
  // CLK_DAC_DOUBLE = ...0 ....  clk_dac control normal
  // PLL_SEL        = .... 0...  PLL disable
  // SYNCMODE       = .... ...0  Sync mode normal
  //                  ---------
  //                  0000 0000 = 0x00
  // --------------------------------------------------------------------
  constant = 0x00u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_B_REG_ADDRESS, &constant, 1u);

  return result;
}
#endif
//#if 0
static esp_err_t ConfigureClock(void)
{
  ESP_LOGE(Tag, "ConfigureClock");

  esp_err_t result = ESP_OK;
  uint8_t constant = 0x00u;

  //xSemaphoreTake(I2CMutex, portMAX_DELAY);

  // FIXME unknown register
  constant = 0xA0u;
  I2C_WriteToAddress(SLAVE_ADDRESS, 0x6F, &constant, 1u);

  // FIXME unknown register
  constant = 0x41u;
  I2C_WriteToAddress(SLAVE_ADDRESS, 0x72, &constant, 1u);

  // CLOCK MANAGER I Register
  // --------------------------------------------------------------------
  // PLL_PDN     = 0... ....  Enable PLL analog
  // PLL_RB      = .0.. ....  Reset PLL digital
  // PLLDITH_MAG = ...0 00..  Dither off
  // PLLOUT_SEL  = .... ..01  VCO out divide by 8
  //               ---------
  //               0000 0001 = 0x01
  // --------------------------------------------------------------------
  constant = 0x01u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_I_REG_ADDRESS, &constant, 1u);

  // Set PLL_K[21:16]
  constant = 0x01u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_L_REG_ADDRESS, &constant, 1u);

  // Set PLL_K[15:8]
  constant = 0x55u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_M_REG_ADDRESS, &constant, 1u);

  // Set PLL_K[7:0]
  constant = 0x33u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_N_REG_ADDRESS, &constant, 1u);

  // CLOCK MANAGER J Register
  // --------------------------------------------------------------------
  // PLL_LP     = 1... ....  PLL low power mode
  // PLL_CP     = .000 ....  PLL cp gain0
  // PLL_SUPSEL = .... 10..  VDDD =3.3v
  // PLL_KVCO   = .... ..10  VCO gain2
  //              ---------
  //              1000 1010 = 0x8A
  // --------------------------------------------------------------------
  constant = 0x8Au;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_J_REG_ADDRESS, &constant, 1u);

  // CLOCK MANAGER K Register
  // --------------------------------------------------------------------
  // PLL_CAL_SHORT = 0... ....  PLL calibration 64 data
  // PLL_VCO_WAIT  = .00. ....  Wait 2 MCLK for vcoout stable when calibration
  // PLL_N         = .... 1001  Integer part of PLL frequency ratio = 9
  //              ------------
  // CLOCK_MANAGER = 0000 1001 = 0x09
  // --------------------------------------------------------------------
  constant = 0x09u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_K_REG_ADDRESS, &constant, 1u);

  // CLOCK MANAGER I Register
  // --------------------------------------------------------------------
  // PLL_PDN     = 0... ....  Enable PLL analog
  // PLL_RB      = .1.. ....  PLL digital on
  // PLLDITH_MAG = ...0 00..  Dither off
  // PLLOUT_SEL  = .... ..01  VCO out divide by 8
  //               ---------
  //               0100 0001 = 0x41
  // --------------------------------------------------------------------
  constant = 0x41u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_I_REG_ADDRESS, &constant, 1u);

  // Set ADC_OSR = 32 (0x20)
  constant = 0x20u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_C_REG_ADDRESS, &constant, 1u);

  // CLOCK MANAGER E Register
  // --------------------------------------------------------------------
  // CLK_ADC_DIV = 0001 ....  CLK_ADC_DIV = 1
  // CLK_DAC_DIV = .... 0001  CLK_DAC_DIV = 1
  //               ---------
  //               0001 0001 = 0x11
  // --------------------------------------------------------------------
  constant = 0x11u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_E_REG_ADDRESS, &constant, 1u);


  // Set Class D speaker clock divider = 32 (0x20)
  constant = 0x20u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_H_REG_ADDRESS, &constant, 1u);

  // CLOCK MANAGER B Register
  // --------------------------------------------------------------------
  // CLK_ADC_CONT   = .0.. ....  CLK_ADC flex
  // CLK_ADC_DOUBLE = ..0. ....  clk_adc control normal
  // CLK_DAC_DOUBLE = ...0 ....  clk_dac control normal
  // PLL_SEL        = .... 1...  PLL enable
  // SYNCMODE       = .... ...0  Sync mode normal
  //                  ---------
  //                  0000 1000 = 0x08
  // --------------------------------------------------------------------
  constant = 0x08u;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_B_REG_ADDRESS, &constant, 1u);

  //xSemaphoreGive(I2CMutex);

  return result;
}
//#endif
//_____________________________________________________________________________

//static esp_err_t InitRegisters(audio_hal_codec_mode_t ms_mode, uint8_t format, T_I2SClock cfg, es_dac_output_t out_channel, es_adc_input_t in_channel)
static esp_err_t InitRegisters(audio_hal_codec_mode_t ms_mode, uint8_t format, T_I2SClock cfg)
{
  ESP_LOGE(Tag, "InitRegisters");

  esp_err_t result = ESP_OK;
  uint8_t data = 0x00u;
  uint8_t constant = 0x00u;

  uint8_t read = 0x00u;

  //xSemaphoreTake(I2CMutex, portMAX_DELAY);

  // Reset DAC digital block, ADC digital block, master block, all registers, digital reset
  constant = 0x3Fu;
  I2C_WriteToAddress(SLAVE_ADDRESS, RESET_REG_ADDRESS, &constant, 1u);
  constant = 0x03u;                      // Reset DAC digital block, ADC digital block
  I2C_WriteToAddress(SLAVE_ADDRESS, RESET_REG_ADDRESS, &constant, 1u);

  // CLOCK MANAGER Register
  // --------------------------------------------------------------------
  // MCLK_DIV2     = 1... ....  MCLK divide by 2
  // MCLK_ON       = .1.. ....  MCLK on
  // BCLK_ON       = ..1. ....  BCLK on
  // CLKD_ON       = ...1 ....  Class D clock on
  // CLK_ADC_ON    = .... 1...  ADC digital clock on
  // CLK_DAC_ON    = .... .1..  DAC digital clock on
  // ANACLK_ADC_ON = .... ..1.  ADC analog clock on
  // ANACLK_DAC_ON = .... ...1  DAC analog clock on
  //              -------------------
  // CLOCK_MANAGER = 1111 1111 = 0xFF
  // --------------------------------------------------------------------
  constant = 0xFFu;
  I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_A_REG_ADDRESS, &constant, 1u);

  I2C_ReadFromAddress(SLAVE_ADDRESS, SDP_A_REG_ADDRESS, &data, 1u);
  data &= MSC_BIT_MASK;              // Slave serial port mode
  data |= (ms_mode << MSC_BIT_POS);
  I2C_WriteToAddress(SLAVE_ADDRESS, SDP_A_REG_ADDRESS, &data, 1u);

//  I2C_ReadFromAddress(SLAVE_ADDRESS, SDP_A_REG_ADDRESS, &read, 1u);
//  ESP_LOGI(Tag, "data = %d, ms_mode = %d, READ 0x0F = %d", data, ms_mode, read);

  result |= ConfigureClock();

  //constant = 0x01u;                      // Enable PLL analog, vcoout divide by 8
  //I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_I_REG_ADDRESS, &constant, 1u);

//  I2C_ReadFromAddress(SLAVE_ADDRESS, 0x09, &read, 1u);
//  ESP_LOGI(Tag, "READ 0x09 = %d", read);

  //constant = 0x22u;
  //I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_L_REG_ADDRESS, &constant, 1u);

//  I2C_ReadFromAddress(SLAVE_ADDRESS, 0x0C, &read, 1u);
//  ESP_LOGI(Tag, "READ 0x0C = %d", read);

  //constant = 0x2Eu;
  //I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_M_REG_ADDRESS, &constant, 1u);

  //constant = 0xC6u;
  //I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_N_REG_ADDRESS, &constant, 1u);

  //constant = 0x8Au;  // 0x3A FIXME In the user guide 0x8Au
  //I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_J_REG_ADDRESS, &constant, 1u);

  //constant = 0x07u;
  //I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_K_REG_ADDRESS, &constant, 1u);

  //constant = 0x41u;
  //I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_I_REG_ADDRESS, &constant, 1u);

  result |= ConfigureI2SClock(cfg);

  constant = 0x08u;  //(1u << ADCHPF_BIT_POS);     // Enable ADC left channel high pass filter
  I2C_WriteToAddress(SLAVE_ADDRESS, ADC_CONTROL_A_REG_ADDRESS, &constant, 1u);

  constant = 0x00;
  I2C_WriteToAddress(SLAVE_ADDRESS, DAC_CONTROL_A_REG_ADDRESS, &constant, 1u);

  constant = 0x30;
  I2C_WriteToAddress(SLAVE_ADDRESS, SYSTEM_A_REG_ADDRESS, &constant, 1u);

  constant = 0x20;
  I2C_WriteToAddress(SLAVE_ADDRESS, SYSTEM_B_REG_ADDRESS, &constant, 1u);

  result |= ConfigureI2SFormat(E_Mode_ADC, format);
  result |= ConfigureI2SFormat(E_Mode_DAC, format);

  // PGA Register
  // --------------------------------------------------------------------
  // PDN_ALINL  = 0... ....  Enable analog PGA circuits
  // PDN_MODE   = .1.. ....  Power down analog ADC modulator
  // LINSEL     = ..10 ....  Lin2-Rin2
  // LDCM       = .... 0...  Disable DC measurement
  // DF2SE_15DB = .... .0..  0dB gain for input diff circuits
  //              ---------
  //              0110 0000 = 0x60
  // --------------------------------------------------------------------
  constant = 0x60;
  I2C_WriteToAddress(SLAVE_ADDRESS, PGA_REG_ADDRESS, &constant, 1u);

  constant = 0x0F;  // PGA gain = -3.5 [dB]
  I2C_WriteToAddress(SLAVE_ADDRESS, PGA_GAIN_REG_ADDRESS, &constant, 1u);

  // PGA Register
  // --------------------------------------------------------------------
  // PDN_ALINL  = 0... ....  Enable analog PGA circuits
  // PDN_MODE   = .0.. ....  Enable analog ADC modulator
  // LINSEL     = ..10 ....  Lin2-Rin2
  // LDCM       = .... 0...  Disable DC measurement
  // DF2SE_15DB = .... .0..  0dB gain for input diff circuits
  //              ---------
  //              0010 0000 = 0x20
  // --------------------------------------------------------------------
  constant = 0x20; //0x14;
  I2C_WriteToAddress(SLAVE_ADDRESS, PGA_REG_ADDRESS, &constant, 1u);

  //constant = 0x55;
  //I2C_WriteToAddress(SLAVE_ADDRESS, PGA_GAIN_REG_ADDRESS, &constant, 1u);

  //constant = 0x21;    // Set class D divider = 33, to avoid the high frequency tone on laudspeaker
  //I2C_WriteToAddress(SLAVE_ADDRESS, CLOCK_MANAGER_H_REG_ADDRESS, &constant, 1u);

  constant = 0x80;  // IC START
  I2C_WriteToAddress(SLAVE_ADDRESS, RESET_REG_ADDRESS, &constant, 1u);

  result |= SetADCDACVolume(E_Mode_ADC, 0, 0);      // 0db
  result |= SetADCDACVolume(E_Mode_DAC, 0, 0);      // 0db

  constant = 0x8A;
  I2C_WriteToAddress(SLAVE_ADDRESS, ANALOG_REF_REG_ADDRESS, &constant, 1u);

  constant = 0x40;
  I2C_WriteToAddress(SLAVE_ADDRESS, ANALOG_POWER_DOWN_REG_ADDRESS, &constant, 1u);

  constant = 0xA0;
  I2C_WriteToAddress(SLAVE_ADDRESS, MONO_OUT_SEL_REG_ADDRESS, &constant, 1u);

  constant = 0x19;
  I2C_WriteToAddress(SLAVE_ADDRESS, MONO_OUT_GAIN_REG_ADDRESS, &constant, 1u);

  constant = 0x90;
  I2C_WriteToAddress(SLAVE_ADDRESS, MIXER_REG_ADDRESS, &constant, 1u);

  constant = 0x02; //0x01;
  I2C_WriteToAddress(SLAVE_ADDRESS, MIXER_GAIN_REG_ADDRESS, &constant, 1u);

  constant = 0x00;
  I2C_WriteToAddress(SLAVE_ADDRESS, SPEAKER_B_REG_ADDRESS, &constant, 1u);

  constant = 0xA0; //0x20;
  I2C_WriteToAddress(SLAVE_ADDRESS, SPEAKER_A_REG_ADDRESS, &constant, 1u);

  constant = 0x00;
  I2C_WriteToAddress(SLAVE_ADDRESS, ALC_CONTROL_C_REG_ADDRESS, &constant, 1u);

  constant = 0x00;
  I2C_WriteToAddress(SLAVE_ADDRESS, ADC_CONTROL_B_REG_ADDRESS, &constant, 1u);

  constant = 0x00;
  I2C_WriteToAddress(SLAVE_ADDRESS, DAC_CONTROL_C_REG_ADDRESS, &constant, 1u);

  constant = 0x30;
  I2C_WriteToAddress(SLAVE_ADDRESS, DAC_CONTROL_B_REG_ADDRESS, &constant, 1u);

  constant = 0x60;
  I2C_WriteToAddress(SLAVE_ADDRESS, GPIO_AND_INT_CONTROL_REG_ADDRESS, &constant, 1u);

  // It's for testing
  constant = 0x0C;
  I2C_WriteToAddress(SLAVE_ADDRESS, SDP_C_REG_ADDRESS, &constant, 1u);

  // FIXME unknown register
  //constant = 0x05u;
  //I2C_WriteToAddress(SLAVE_ADDRESS, 0x71, &constant, 1u);

  // FIXME unknown register
  //constant = 0x70u;
  //I2C_WriteToAddress(SLAVE_ADDRESS, 0x73, &constant, 1u);

  // TODO res |= ConfigureDACOutput(out_channel);  //0x3c Enable DAC and Enable Lout/Rout/1/2
  result |= ConfigureDACOutput();
  // TODO res |= ConfigureADCInput(in_channel);  //0x00 LINSEL & RINSEL, LIN1/RIN1 as ADC Input; DSSEL,use one DS Reg11; DSR, LINPUT1-RINPUT1
  result |= ConfigureADCInput();
  result |= ES8374_SetVoiceVolume(0);

  constant = 0x30;
  I2C_WriteToAddress(SLAVE_ADDRESS, DAC_CONTROL_B_REG_ADDRESS, &constant, 1u);

  //xSemaphoreGive(I2CMutex);

  return result;
}
