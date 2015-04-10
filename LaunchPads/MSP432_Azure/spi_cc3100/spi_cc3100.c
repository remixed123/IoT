/*
 * spi.c - msp430f5529 launchpad spi interface implementation
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

#ifndef SL_IF_TYPE_UART
#include <msp432.h>

#include "simplelink.h"
#include "spi_cc3100.h"
#include "board.h"

//MSP430F5529
//#define ASSERT_CS()          (P2OUT &= ~BIT2)
//#define DEASSERT_CS()        (P2OUT |= BIT2)
//MSP432
#define ASSERT_CS()          (P3OUT &= ~BIT0)
#define DEASSERT_CS()        (P3OUT |= BIT0)

int spi_Close(Fd_t fd)
{
    /* Disable WLAN Interrupt ... */
    CC3100_InterruptDisable();

    return NONOS_RET_OK;
}

Fd_t spi_Open(char *ifName, unsigned long flags)
{
    /* Select the MSP430F5529 SPI lines: MOSI/MISO on P3.0,1 CLK on P3.2 */
    // P3SEL |= (BIT0 + BIT1);
    /* Select the MSP432 SPI lines: MOSI/MISO on P1.6,7 CLK on P3.2 */
    P1SEL0 |= (BIT6 + BIT7);
    P1SEL1 &= ~(BIT6 + BIT7);
    P1REN |= BIT7;
    P1OUT |= BIT7;

    P1SEL0 |= BIT5;
    P1SEL1 &= ~BIT5;

//	MSP430 Code, 8-bit on 16-bit
//    UCB0CTL1 |= UCSWRST; /* Put state machine in reset */
//    UCB0CTL0 = UCMSB + UCMST + UCSYNC + UCCKPH; /* 3-pin, 8-bit SPI master */
//
//    UCB0CTL1 = UCSWRST + UCSSEL_2; /* Use SMCLK, keep RESET */

//    MSP432 code
    UCB0CTLW0 |= UCSWRST; /* Put state machine in reset */
    UCB0CTLW0 = UCMSB | UCMST | UCSYNC | UCCKPH | UCSWRST | UCSSEL__SMCLK; /* 3-pin, 8-bit SPI master */




    /* Set SPI clock */
    UCB0BR0 = 0x08; /* f_UCxCLK = 25MHz/2 */
    UCB0BR1 = 0;
    // previously UCB0CTL1 &= ~UCSWRST;
    UCB0CTLW0 &= ~UCSWRST;


    /* P1.6 on MSP430 - WLAN enable full DS */
    /* P4.1 on MSP432 - WLAN enable full DS */
    P4SEL0 &= ~BIT1;
    P4SEL1 &= ~BIT1;
    P4OUT &= ~BIT1;
    P4DIR |= BIT1;


    /* Configure SPI IRQ line on P2.0 on MSP430 */
    /* Configure SPI IRQ line on P2.5 on MSP432 */
    P2DIR &= ~BIT5;
    P2SEL0 &= ~BIT5;
    P2SEL1 &= ~BIT5;

    P2REN |= BIT5;

    /* Configure the SPI CS to be on P2.2 on MSP430*/
    /* Configure the SPI CS to be on P3.0 on MSP432*/
    P3OUT |= BIT0;
    P3SEL0 &= ~BIT0;
    P3SEL1 &= ~BIT0;
    P3DIR |= BIT0;

    /* 50 ms delay */
    Delay(500);

    /* Enable WLAN interrupt */
    CC3100_InterruptEnable();

    return NONOS_RET_OK;
}


int spi_Write(Fd_t fd, unsigned char *pBuff, int len)
{
        int len_to_return = len;

    ASSERT_CS();
    while (len)
    {
        while (!(UCB0IFG&UCTXIFG));
        UCB0TXBUF = *pBuff;
        while (!(UCB0IFG&UCRXIFG));
        UCB0RXBUF;
        len --;
        pBuff++;
    }

    DEASSERT_CS();

    return len_to_return;
}


int spi_Read(Fd_t fd, unsigned char *pBuff, int len)
{
    int i = 0;

    ASSERT_CS();

    for (i = 0; i < len; i ++)
    {
        while (!(UCB0IFG&UCTXIFG));
        UCB0TXBUF = 0xFF;
        while (!(UCB0IFG&UCRXIFG));
        pBuff[i] = UCB0RXBUF;
    }

    DEASSERT_CS();

    return len;
}
#endif /* SL_IF_TYPE_UART */
