/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "board.h"
#include "music.h"
#include "fsl_sai.h"
#include "fsl_debug_console.h"

#include "fsl_sgtl5000.h"
#include "pin_mux.h"
#include "clock_config.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* SAI and I2C instance and clock */
#define DEMO_SAI I2S0
#define DEMO_SAI_CHANNEL (0)
#define DEMO_SAI_BITWIDTH (kSAI_WordWidth16bits)
#define DEMO_I2C I2C0
#define DEMO_SAI_CLKSRC kCLOCK_CoreSysClk
#define DEMO_I2C_CLKSRC kCLOCK_BusClk
#define DEMO_SAI_IRQ I2S0_Tx_IRQn
#define SAI_TxIRQHandler I2S0_Tx_IRQHandler

#define OVER_SAMPLE_RATE (384U)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
static size_t g_index = 0;
static volatile bool isFinished = false;
#if defined(DEMO_CODEC_WM8960)
wm8960_handle_t codecHandle = {0};
#else
sgtl_handle_t codecHandle = {0};
#endif

#if defined(FSL_FEATURE_SOC_LPI2C_COUNT) && (FSL_FEATURE_SOC_LPI2C_COUNT)
lpi2c_master_handle_t i2cHandle;
#else
i2c_master_handle_t i2cHandle;
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Main function
 */
int main(void)
{
    sai_config_t config;
    uint32_t mclkSourceClockHz = 0U;
    sai_transfer_format_t format;
#if defined(FSL_FEATURE_SOC_LPI2C_COUNT) && (FSL_FEATURE_SOC_LPI2C_COUNT)
    lpi2c_master_config_t i2cConfig = {0};
#else
    i2c_master_config_t i2cConfig = {0};
#endif
    uint32_t i2cSourceClock;

    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    PRINTF("SAI functional interrupt example started!\n\r");

    /*
     * config.masterSlave = kSAI_Master;
     * config.mclkSource = kSAI_MclkSourceSysclk;
     * config.protocol = kSAI_BusLeftJustified;
     * config.syncMode = kSAI_ModeAsync;
     * config.mclkOutputEnable = true;
     */
    SAI_TxGetDefaultConfig(&config);
    SAI_TxInit(DEMO_SAI, &config);

    /* Configure the audio format */
    format.bitWidth = DEMO_SAI_BITWIDTH;
    format.channel = DEMO_SAI_CHANNEL;
    format.sampleRate_Hz = kSAI_SampleRate16KHz;
#if defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER)
    format.masterClockHz = OVER_SAMPLE_RATE * format.sampleRate_Hz;
#else
    format.masterClockHz = CLOCK_GetFreq(DEMO_SAI_CLKSRC);
#endif
    format.protocol = config.protocol;
    format.stereo = kSAI_Stereo;

    /* Configure Sgtl5000 I2C */
    codecHandle.base = DEMO_I2C;
    codecHandle.i2cHandle = &i2cHandle;
    i2cSourceClock = CLOCK_GetFreq(DEMO_I2C_CLKSRC);

#if defined(FSL_FEATURE_SOC_LPI2C_COUNT) && (FSL_FEATURE_SOC_LPI2C_COUNT)
    /*
     * i2cConfig.debugEnable = false;
     * i2cConfig.ignoreAck = false;
     * i2cConfig.pinConfig = kLPI2C_2PinOpenDrain;
     * i2cConfig.baudRate_Hz = 100000U;
     * i2cConfig.busIdleTimeout_ns = 0;
     * i2cConfig.pinLowTimeout_ns = 0;
     * i2cConfig.sdaGlitchFilterWidth_ns = 0;
     * i2cConfig.sclGlitchFilterWidth_ns = 0;
     */
    LPI2C_MasterGetDefaultConfig(&i2cConfig);
    LPI2C_MasterInit(DEMO_I2C, &i2cConfig, i2cSourceClock);
    LPI2C_MasterTransferCreateHandle(DEMO_I2C, &i2cHandle, NULL, NULL);
#else
    /*
     * i2cConfig.baudRate_Bps = 100000U;
     * i2cConfig.enableHighDrive = false;
     * i2cConfig.enableStopHold = false;
     * i2cConfig.glitchFilterWidth = 0U;
     * i2cConfig.enableMaster = true;
     */
    I2C_MasterGetDefaultConfig(&i2cConfig);
    I2C_MasterInit(DEMO_I2C, &i2cConfig, i2cSourceClock);
    I2C_MasterTransferCreateHandle(DEMO_I2C, &i2cHandle, NULL, NULL);
#endif

#if defined(DEMO_CODEC_WM8960)
    WM8960_Init(&codecHandle, NULL);
    WM8960_ConfigDataFormat(&codecHandle, format.masterClockHz, format.sampleRate_Hz, format.bitWidth);
#else
    /* Use default settings for sgtl5000 */
    SGTL_Init(&codecHandle, NULL);
    /* Configure codec format */
    SGTL_ConfigDataFormat(&codecHandle, format.masterClockHz, format.sampleRate_Hz, format.bitWidth);
#endif
    mclkSourceClockHz = CLOCK_GetFreq(DEMO_SAI_CLKSRC);
    SAI_TxSetFormat(DEMO_SAI, &format, mclkSourceClockHz, format.masterClockHz);

    /*  Enable interrupt */
    EnableIRQ(DEMO_SAI_IRQ);
    SAI_TxEnableInterrupts(DEMO_SAI, kSAI_FIFOWarningInterruptEnable | kSAI_FIFOErrorInterruptEnable);
    SAI_TxEnable(DEMO_SAI, true);

    /* Wait until finished */
    while (isFinished != true)
    {
    }

    PRINTF("\n\r SAI functional interrupt example finished!\n\r ");
    while (1)
    {
    }
}

void SAI_TxIRQHandler(void)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint32_t data = 0;
    uint32_t temp = 0;

    /* Clear the FIFO error flag */
    if (SAI_TxGetStatusFlag(DEMO_SAI) & kSAI_FIFOErrorFlag)
    {
        SAI_TxClearStatusFlags(DEMO_SAI, kSAI_FIFOErrorFlag);
    }

    if (SAI_TxGetStatusFlag(DEMO_SAI) & kSAI_FIFOWarningFlag)
    {
        for (i = 0; i < FSL_FEATURE_SAI_FIFO_COUNT; i++)
        {
            data = 0;
            for (j = 0; j < DEMO_SAI_BITWIDTH / 8U; j++)
            {
                temp = (uint32_t)(music[g_index]);
                data |= (temp << (8U * j));
                g_index++;
            }
            SAI_WriteData(DEMO_SAI, DEMO_SAI_CHANNEL, data);
        }
    }

    if (g_index >= MUSIC_LEN)
    {
        isFinished = true;
        SAI_TxDisableInterrupts(DEMO_SAI, kSAI_FIFOWarningInterruptEnable | kSAI_FIFOErrorInterruptEnable);
        SAI_TxEnable(DEMO_SAI, false);
    }
}
