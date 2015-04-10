/*
 * uart.h - msp430f5529 launchpad uart interface implementation
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

#ifdef SL_IF_TYPE_UART
#include "simplelink.h"
#include "board.h"
#include <msp430.h>
#include "uart.h"

#define CTS_LINE_IS_HIGH        (P1IN & BIT3)

extern unsigned char IntIsMasked;

extern P_EVENT_HANDLER  pIraEventHandler;

extern _uartFlowctrl *puartFlowctrl;


int uart_Close(Fd_t fd)
{
    /* Disable WLAN Interrupt ... */
    CC3100_InterruptDisable();

    return NONOS_RET_OK;
}


Fd_t uart_Open(char *ifName, unsigned long flags)
{
    unsigned char i;

    IntIsMasked = FALSE;

    puartFlowctrl->JitterBufferFreeBytes = UART_READ_JITTER_BUFFER_SIZE;
    puartFlowctrl->JitterBufferWriteIdx = 0;

    puartFlowctrl->pActiveBuffer = puartFlowctrl->JitterBuffer;
    puartFlowctrl->bActiveBufferIsJitterOne = TRUE;

    puartFlowctrl->JitterBufferReadIdx = 0xFF;

    for(i = 0; i < UART_READ_JITTER_BUFFER_SIZE; i++)
    {
        puartFlowctrl->JitterBuffer[i] = 0xCC;
    }


    /* P1.6 - WLAN enable full DS */
    P1SEL &= ~BIT6;
    P1OUT &= ~BIT6;
    P1DIR |= BIT6;


    /* Configure Host IRQ line on P2.0 */
    P2DIR &= ~BIT0;
    P2SEL &= ~BIT0;

    P2REN |= BIT0;

    /* Configure Pin 3.3/3.4 for RX/TX */
    P3SEL |= BIT3 + BIT4;               /* P4.4,5 = USCI_A0 TXD/RXD */

    UCA0CTL1 |= UCSWRST;                /* Put state machine in reset */
    UCA0CTL0 = 0x00;
    UCA0CTL1 = UCSSEL__SMCLK + UCSWRST; /* Use SMCLK, keep RESET */
    UCA0BR0 = 0xD9;         /* 25MHz/115200= 217.01 =0xD9 (see User's Guide) */
    UCA0BR1 = 0x00;         /* 25MHz/9600= 2604 =0xA2C (see User's Guide) */

    UCA0MCTL = UCBRS_3 + UCBRF_0;       /* Modulation UCBRSx=3, UCBRFx=0 */

    UCA0CTL1 &= ~UCSWRST;               /* Initialize USCI state machine */

    /* Enable RX Interrupt on UART */
    UCA0IFG &= ~ (UCRXIFG | UCRXIFG);
    UCA0IE |= UCRXIE;

    /* Configure Pin 1.3 and 1.4 as RTS as CTS */
    P1SEL &= ~(BIT3 + BIT4);

    P1OUT &= ~BIT4;
    P1DIR |= BIT4;

    P1DIR &= ~BIT3;
    P1REN |= BIT3;
	
	/* 50 ms delay */
    Delay(50);

    /* Enable WLAN interrupt */
    CC3100_InterruptEnable();

    clear_rts();


    return NONOS_RET_OK;
}


int uart_Read(Fd_t fd, unsigned char *pBuff, int len)
{
    /* Disable interrupt to protect reorder of bytes */
    __disable_interrupt();

    puartFlowctrl->pActiveBuffer = pBuff;
    puartFlowctrl->bActiveBufferIsJitterOne = FALSE;
    puartFlowctrl->ActiveBufferWriteCounter = 0;

    /* Copy data received in Jitter buffer to the user buffer */
    while(puartFlowctrl->JitterBufferFreeBytes != UART_READ_JITTER_BUFFER_SIZE)
    {
        if(puartFlowctrl->JitterBufferReadIdx == (UART_READ_JITTER_BUFFER_SIZE - 1))
        {
            puartFlowctrl->JitterBufferReadIdx = 0;
        }
        else
        {
            puartFlowctrl->JitterBufferReadIdx++;
        }

        puartFlowctrl->pActiveBuffer[puartFlowctrl->ActiveBufferWriteCounter++] = puartFlowctrl->JitterBuffer[puartFlowctrl->JitterBufferReadIdx];

        puartFlowctrl->JitterBufferFreeBytes ++;
    }


    puartFlowctrl->bRtsSetByFlowControl = FALSE;
    __enable_interrupt();
    clear_rts();

    /* wait till all remaining bytes are received */
    while(puartFlowctrl->ActiveBufferWriteCounter < len);

    puartFlowctrl->bActiveBufferIsJitterOne = TRUE;

    return len;
}


int uart_Write(Fd_t fd, unsigned char *pBuff, int len)
{
    int len_to_return = len;

    while (len)
    {
        while (!(UCA0IFG & UCTXIFG) || CTS_LINE_IS_HIGH) ;
        UCA0TXBUF = *pBuff;
        len--;
        pBuff++;
    }
    return len_to_return;
}
#endif /* SL_IF_TYPE_UART */
