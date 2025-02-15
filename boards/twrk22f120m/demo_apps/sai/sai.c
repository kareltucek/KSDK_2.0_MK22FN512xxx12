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
#include "fsl_dmamux.h"
#include "fsl_sai_edma.h"
#include "fsl_debug_console.h"
#include "arm_math.h"

#include "fsl_sgtl5000.h"
#include "pin_mux.h"
#include "clock_config.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* SAI and I2C instance and clock */
#define DEMO_SAI I2S0
#define DEMO_I2C I2C0
#define DEMO_SAI_CLKSRC kCLOCK_CoreSysClk
#define DEMO_I2C_CLKSRC kCLOCK_BusClk
#define EXAMPLE_DMA DMA0
#define EXAMPLE_CHANNEL (0U)
#define EXAMPLE_SAI_TX_SOURCE (13U)
#define OVER_SAMPLE_RATE (384U)
#define BUFFER_SIZE (1024)
#define BUFFER_NUM (2)
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void callback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData);

/*******************************************************************************
 * Variables
 ******************************************************************************/
sai_edma_handle_t txHandle = {0};
edma_handle_t dmaHandle = {0};
static uint8_t audioBuff[BUFFER_SIZE * BUFFER_NUM];
static float32_t ffData[2 * BUFFER_SIZE];
static float32_t ffResult[BUFFER_SIZE];
#if defined(DEMO_CODEC_WM8960)
wm8960_handle_t codecHandle = {0};
#else
sgtl_handle_t codecHandle = {0};
#endif

#if defined(FSL_FEATURE_SOC_LPI2C_COUNT) && (FSL_FEATURE_SOC_LPI2C_COUNT)
lpi2c_master_handle_t i2cHandle = {0};
#else
i2c_master_handle_t i2cHandle = {{0, 0, kI2C_Write, 0, 0, NULL, 0}, 0, 0, NULL, NULL};
#endif
volatile bool isFinished = false;
volatile uint32_t userIndex = 0;

/*******************************************************************************
 * Code
 ******************************************************************************/
static void callback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData)
{
    isFinished = true;
}

float32_t do_fft(sai_transfer_format_t *dataFormat, uint8_t *buffer, float32_t *fftData, float32_t *fftResult)
{
    /* Counter variable for navigating buffers */
    uint32_t counter;

    /* Return value for wav frequency in hertz */
    float32_t wavFreqHz;

    /* CMSIS status & FFT instance */
    arm_status status;                /* ARM status variable */
    arm_cfft_radix2_instance_f32 fft; /* ARM FFT instance */

    /* Frequency analysis variables */
    float32_t maxValue;     /* max value for greatest FFT bin amplitude */
    uint32_t testIndex = 0; /* value for storing the bin location with maxValue */
    uint32_t complexBuffSize = BUFFER_SIZE * 2;
    uint32_t fftSize = BUFFER_SIZE;                  /* FFT bin size */
    uint32_t ifftFlag = 0;                           /* Flag for the selection of CFFT/CIFFT */
    uint32_t doBitReverse = 1;                       /* Flag for selection of normal order or bit reversed order */
    uint32_t sampleRate = dataFormat->sampleRate_Hz; /* Get sample rate from current format */
    float32_t hzPerBin = 2 * ((float32_t)sampleRate / (float32_t)fftSize); /* Calculate hz per FFT bin */

    uint8_t *temp8; /* Point to data for 8 bit samples */
    uint8_t temp8Data;

    uint16_t *temp16; /* Point to data for 16 bit samples */
    int16_t temp16Data;

    uint32_t *temp32; /* Point to data for 32 bit samples */
    int32_t temp32Data;

    /* Set status as success */
    status = ARM_MATH_SUCCESS;

    /* Wav data variables */
    switch (dataFormat->bitWidth)
    {
        case 8:
            temp8 = (uint8_t *)buffer;
            temp8Data = 0;

            /* Copy wav data to fft input array */
            for (counter = 0; counter < complexBuffSize; counter++)
            {
                if (counter % 2 == 0)
                {
                    temp8Data = (uint8_t)*temp8;
                    fftData[counter] = (float32_t)temp8Data;
                    temp8++;
                }
                else
                {
                    fftData[counter] = 0.0;
                }
            }

            /* Set instance for Real FFT */
            status = arm_cfft_radix2_init_f32(&fft, fftSize, ifftFlag, doBitReverse);

            /* Perform Real FFT on fftData */
            arm_cfft_radix2_f32(&fft, fftData);

            /* Populate FFT bins */
            arm_cmplx_mag_f32(fftData, fftResult, fftSize);

            /* Zero out non-audible, low-frequency noise from FFT Results. */
            fftResult[0] = 0.0;

            /* Find max bin and location of max (first half of bins as this is the only valid section) */
            arm_max_f32(fftResult, fftSize, &maxValue, &testIndex);

            break;

        case 16:
            temp16 = (uint16_t *)buffer;
            temp16Data = 0;

            /* Copy wav data to fft input array */
            for (counter = 0; counter < complexBuffSize; counter++)
            {
                if (counter % 2 == 0)
                {
                    temp16Data = (int16_t)*temp16;
                    fftData[counter] = (float32_t)temp16Data;
                    temp16++;
                }
                else
                {
                    fftData[counter] = 0.0;
                }
            }

            /* Set instance for Real FFT */
            status = arm_cfft_radix2_init_f32(&fft, fftSize, ifftFlag, doBitReverse);

            /* Perform Real FFT on fftData */
            arm_cfft_radix2_f32(&fft, fftData);

            /* Populate FFT bins */
            arm_cmplx_mag_f32(fftData, fftResult, fftSize);

            /* Zero out non-audible, low-frequency noise from FFT Results. */
            fftResult[0] = 0.0;

            /* Find max bin and location of max (first half of bins as this is the only valid section) */
            arm_max_f32(fftResult, fftSize, &maxValue, &testIndex);

            break;

        case 32:
            temp32 = (uint32_t *)buffer;
            temp32Data = 0;

            /* Copy wav data to fft input array */
            for (counter = 0; counter < complexBuffSize; counter++)
            {
                if (counter % 2 == 0)
                {
                    temp32Data = (int32_t)*temp32;
                    fftData[counter] = (float32_t)temp32Data;
                    temp32++;
                }
                else
                {
                    fftData[counter] = 0.0;
                }
            }

            /* Set instance for Real FFT */
            status = arm_cfft_radix2_init_f32(&fft, fftSize, ifftFlag, doBitReverse);

            /* Perform Real FFT on fftData */
            arm_cfft_radix2_f32(&fft, fftData);

            /* Populate FFT bins */
            arm_cmplx_mag_f32(fftData, fftResult, fftSize);

            /* Zero out non-audible, low-frequency noise from FFT Results. */
            fftResult[0] = 0.0;

            /* Find max bin and location of max (first half of bins as this is the only valid section) */
            arm_max_f32(fftResult, fftSize, &maxValue, &testIndex);

            break;

        default:
            __asm("NOP");
            break;
    }

    if (status != ARM_MATH_SUCCESS)
    {
        wavFreqHz = 0; /* If an error has occured set frequency of wav data to 0Hz */
        PRINTF("\r\nFFT compuation error.\r\n");
    }
    else
    {
        /* Set wavFreqHz to bin location of max amplitude multiplied by the hz per bin */
        wavFreqHz = testIndex * hzPerBin;
    }

    return wavFreqHz;
}

/*!
 * @brief Main function
 */
int main(void)
{
    sai_config_t config;
    uint32_t mclkSourceClockHz = 0U;
    sai_transfer_format_t format;
    sai_transfer_t xfer;
    edma_config_t dmaConfig = {0};
#if defined(FSL_FEATURE_SOC_LPI2C_COUNT) && (FSL_FEATURE_SOC_LPI2C_COUNT)
    lpi2c_master_config_t i2cConfig = {0};
#else
    i2c_master_config_t i2cConfig = {0};
#endif
    uint32_t i2cSourceClock;
    float32_t freq = 0;
    uint32_t i = 0;
    char doFFT = '1';

    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    PRINTF("SAI example started!\n\r");
    PRINTF("\n\rPlease choose the option :\n\r");

    PRINTF("\r1. No DSP\r\n");
    PRINTF("\r2. Do FFT\r\n");

    doFFT = GETCHAR();
    PUTCHAR(doFFT);
    PRINTF("\r\n");

    /* Create EDMA handle */
    /*
     * dmaConfig.enableRoundRobinArbitration = false;
     * dmaConfig.enableHaltOnError = true;
     * dmaConfig.enableContinuousLinkMode = false;
     * dmaConfig.enableDebugMode = false;
     */
    EDMA_GetDefaultConfig(&dmaConfig);
    EDMA_Init(EXAMPLE_DMA, &dmaConfig);
    EDMA_CreateHandle(&dmaHandle, EXAMPLE_DMA, EXAMPLE_CHANNEL);

    DMAMUX_Init(DMAMUX0);
    DMAMUX_SetSource(DMAMUX0, EXAMPLE_CHANNEL, EXAMPLE_SAI_TX_SOURCE);
    DMAMUX_EnableChannel(DMAMUX0, EXAMPLE_CHANNEL);

    /* Init SAI module */
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
    format.bitWidth = kSAI_WordWidth16bits;
    format.channel = 0U;
    format.sampleRate_Hz = kSAI_SampleRate16KHz;
#if defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER)
    format.masterClockHz = OVER_SAMPLE_RATE * format.sampleRate_Hz;
#else
    format.masterClockHz = CLOCK_GetFreq(DEMO_SAI_CLKSRC);
#endif
    format.protocol = config.protocol;
    format.stereo = kSAI_Stereo;
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    format.watermark = FSL_FEATURE_SAI_FIFO_COUNT / 2U;
#endif

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

    SAI_TransferTxCreateHandleEDMA(DEMO_SAI, &txHandle, callback, NULL, &dmaHandle);

    mclkSourceClockHz = CLOCK_GetFreq(DEMO_SAI_CLKSRC);
    SAI_TransferTxSetFormatEDMA(DEMO_SAI, &txHandle, &format, mclkSourceClockHz, format.masterClockHz);

    /* First copy the buffer full */
    memcpy(audioBuff, music, BUFFER_SIZE * BUFFER_NUM);

    xfer.dataSize = BUFFER_SIZE;
    /* Start transfer */
    for (i = 0; i < BUFFER_NUM; i++)
    {
        xfer.data = audioBuff + i * BUFFER_SIZE;
        SAI_TransferSendEDMA(DEMO_SAI, &txHandle, &xfer);
    }

    for (i = 0; i < (MUSIC_LEN / BUFFER_SIZE - BUFFER_NUM); i++)
    {
        /* One block finished */
        while (isFinished != true)
        {
        }

        /* Copy data to buffer */
        memcpy(audioBuff + userIndex * BUFFER_SIZE, &music[(i + 2) * BUFFER_SIZE], BUFFER_SIZE);
        userIndex = (userIndex + 1) % BUFFER_NUM;

        if (doFFT == '2')
        {
            freq = do_fft(&format, audioBuff + userIndex * BUFFER_SIZE, ffData, ffResult);
            PRINTF("\r\nThe frequency is %f", freq);
        }

        /* Add another buffer */
        xfer.data = audioBuff + userIndex * BUFFER_SIZE;
        SAI_TransferSendEDMA(DEMO_SAI, &txHandle, &xfer);
        isFinished = false;
    }

#if defined(DEMO_CODEC_WM8960)
    WM8960_Deinit(&codecHandle);
#else
    SGTL_Deinit(&codecHandle);
#endif

#if defined(FSL_FEATURE_SOC_LPI2C_COUNT) && (FSL_FEATURE_SOC_LPI2C_COUNT)
    LPI2C_MasterDeinit(DEMO_I2C);
#else
    I2C_MasterDeinit(DEMO_I2C);
#endif
    PRINTF("\n\r SAI demo finished!\n\r ");
    while (1)
    {
    }
}
