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

#include "fsl_common.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "pin_mux.h"

/*******************************************************************************
 * Code
 ******************************************************************************/
void BOARD_InitPins(void)
{
    /* Declare and initialise for pull up configuration */
    port_pin_config_t pinConfig = {0};
    pinConfig.pullSelect = kPORT_PullUp;
    pinConfig.openDrainEnable = kPORT_OpenDrainEnable;

    /* Initialize UART1 pins below */
    /* Ungate the port clock */
    CLOCK_EnableClock(kCLOCK_PortE);

    /* Affects PORTE_PCR0 register */
    PORT_SetPinMux(PORTE, 0U, kPORT_MuxAlt3);
    /* Affects PORTE_PCR1 register */
    PORT_SetPinMux(PORTE, 1U, kPORT_MuxAlt3);

    /* Ungate the port clock */
    CLOCK_EnableClock(kCLOCK_PortC);

    /* Release I2C bus */
    BOARD_I2C_ReleaseBus();

    /* I2C1 pull up resistor setting */
    PORT_SetPinConfig(PORTC, 10U, &pinConfig);
    PORT_SetPinConfig(PORTC, 11U, &pinConfig);
    /* I2C1 PIN_MUX Configuration */
    PORT_SetPinMux(PORTC, 10U, kPORT_MuxAlt2);
    PORT_SetPinMux(PORTC, 11U, kPORT_MuxAlt2);
}

void BOARD_I2C_ReleaseBus(void)
{
    port_pin_config_t i2c_pin_config = {0};
    gpio_pin_config_t pin_config;
    uint8_t i = 0;
    uint8_t j = 0;

    /* Config pin mux as gpio */
    i2c_pin_config.pullSelect = kPORT_PullUp;
    i2c_pin_config.mux = kPORT_MuxAsGpio;

    pin_config.pinDirection = kGPIO_DigitalOutput;
    pin_config.outputLogic = 1U;

    PORT_SetPinConfig(PORTC, 10U, &i2c_pin_config);
    PORT_SetPinConfig(PORTC, 11U, &i2c_pin_config);

    GPIO_PinInit(GPIOC, 10U, &pin_config);
    GPIO_PinInit(GPIOC, 11U, &pin_config);

    /* Drive SDA low first to simulate a start */
    GPIO_WritePinOutput(GPIOC, 11U, 0U);

    /* Send 8 pulses on SCL and keep SDA low */
    for (i = 0; i < 8; i++)
    {
        GPIO_WritePinOutput(GPIOC, 10U, 0U);
        for (j = 0; j < 255; j++)
        {
            __asm("nop");
        }
        GPIO_WritePinOutput(GPIOC, 10U, 1U);
        for (j = 0; j < 255; j++)
        {
            __asm("nop");
        }
    }

    /* Drive SDA high to simulate a nak */
    GPIO_WritePinOutput(GPIOC, 10U, 0U);
    GPIO_WritePinOutput(GPIOC, 11U, 1U);
    for (j = 0; j < 255; j++)
    {
        __asm("nop");
    }
    GPIO_WritePinOutput(GPIOC, 10U, 1U);
    for (j = 0; j < 255; j++)
    {
        __asm("nop");
    }
    GPIO_WritePinOutput(GPIOC, 10U, 0U);
    for (j = 0; j < 255; j++)
    {
        __asm("nop");
    }

    /* Send stop */
    GPIO_WritePinOutput(GPIOC, 11U, 0U);
    for (j = 0; j < 255; j++)
    {
        __asm("nop");
    }
    GPIO_WritePinOutput(GPIOC, 10U, 1U);
    for (j = 0; j < 255; j++)
    {
        __asm("nop");
    }
}
