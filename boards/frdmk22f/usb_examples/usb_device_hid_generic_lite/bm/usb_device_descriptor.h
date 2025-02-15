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

#ifndef __USB_DEVICE_DESCRIPTOR_H__
#define __USB_DEVICE_DESCRIPTOR_H__

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define USB_DEVICE_SPECIFIC_BCD_VERSION (0x0200U)
#define USB_DEVICE_DEMO_BCD_VERSION (0x0101U)

#define USB_DEVICE_CLASS (0x00U)
#define USB_DEVICE_SUBCLASS (0x00U)
#define USB_DEVICE_PROTOCOL (0x00U)

#define USB_DEVICE_MAX_POWER (0x32U)

#define USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL (41U)
#define USB_DESCRIPTOR_LENGTH_HID_GENERIC_REPORT (33U)
#define USB_DESCRIPTOR_LENGTH_HID (9U)
#define USB_DESCRIPTOR_LENGTH_STRING0 (4U)
#define USB_DESCRIPTOR_LENGTH_STRING1 (58U)
#define USB_DESCRIPTOR_LENGTH_STRING2 (38U)

#define USB_DEVICE_CONFIGURATION_COUNT (1U)
#define USB_DEVICE_STRING_COUNT (3U)
#define USB_DEVICE_LANGUAGE_COUNT (1U)

#define USB_HID_GENERIC_CONFIGURE_INDEX (1U)
#define USB_HID_GENERIC_INTERFACE_COUNT (1U)

#define USB_HID_GENERIC_IN_BUFFER_LENGTH (8U)
#define USB_HID_GENERIC_OUT_BUFFER_LENGTH (8U)
#define USB_HID_GENERIC_ENDPOINT_COUNT (2U)
#define USB_HID_GENERIC_INTERFACE_INDEX (0U)
#define USB_HID_GENERIC_ENDPOINT_IN (1U)
#define USB_HID_GENERIC_ENDPOINT_OUT (2U)

#define USB_HID_GENERIC_CLASS (0x03U)
#define USB_HID_GENERIC_SUBCLASS (0x00U)
#define USB_HID_GENERIC_PROTOCOL (0x00U)

#define HS_HID_GENERIC_INTERRUPT_OUT_PACKET_SIZE (8U)
#define FS_HID_GENERIC_INTERRUPT_OUT_PACKET_SIZE (8U)
#define HS_HID_GENERIC_INTERRUPT_OUT_INTERVAL (0x04U) /* 2^(4-1) = 1ms */
#define FS_HID_GENERIC_INTERRUPT_OUT_INTERVAL (0x01U)

#define HS_HID_GENERIC_INTERRUPT_IN_PACKET_SIZE (8U)
#define FS_HID_GENERIC_INTERRUPT_IN_PACKET_SIZE (8U)
#define HS_HID_GENERIC_INTERRUPT_IN_INTERVAL (0x04U) /* 2^(4-1) = 1ms */
#define FS_HID_GENERIC_INTERRUPT_IN_INTERVAL (0x01U)

/*******************************************************************************
 * API
 ******************************************************************************/

extern usb_status_t USB_DeviceSetSpeed(uint8_t speed);

#endif /* __USB_DEVICE_DESCRIPTOR_H__ */
