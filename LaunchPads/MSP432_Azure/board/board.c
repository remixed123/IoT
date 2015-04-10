/*
 * board.c - msp430f5529 launchpad configuration
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

#include "simplelink.h"
#include "board.h"
#include "driverlib.h"

#define XT1_XT2_PORT_SEL0            PJSEL0
#define XT1_XT2_PORT_SEL1            PJSEL1
#define XT1_ENABLE                  (BIT0 + BIT1)
#define XT2_ENABLE                  (BIT2 + BIT3)
#define XT1HFOFFG   0

P_EVENT_HANDLER                pIraEventHandler = 0;

unsigned char IntIsMasked;


#ifdef SL_IF_TYPE_UART
#define ASSERT_UART(expr) {  if (!(expr)) { while(1) ;}}

unsigned char error_overrun = FALSE;
_uartFlowctrl uartFlowctrl;
_uartFlowctrl *puartFlowctrl = &uartFlowctrl;
#endif







int registerInterruptHandler(P_EVENT_HANDLER InterruptHdl , void* pValue)
{
    pIraEventHandler = InterruptHdl;

    return 0;
}


void CC3100_disable()
{
    // MSP430F5529 P1OUT &= ~BIT6;
	P4OUT &= ~BIT1;
}


void CC3100_enable()
{
	// MSP430F5529 = P1.6 --> P1OUT |= BIT6;
	// MSP432P401R = P4.1
	P4OUT |= BIT1;
}

void CC3100_InterruptEnable(void)
{
	// MSP430F5529 P2.0, MSP432P401R = P2.5
    P2IES &= ~BIT5;
    P2IE |= BIT5;
    Interrupt_enableInterrupt(INT_PORT2);
    Interrupt_enableMaster();

#ifdef SL_IF_TYPE_UART
    UCA0IE |= UCRXIE;
#endif

}

void CC3100_InterruptDisable()
{
	// MSP430F5529 P2.0, MSP432P401R = P2.5
    P2IE &= ~BIT5;
    Interrupt_disableInterrupt(INT_PORT2);
#ifdef SL_IF_TYPE_UART
    UCA0IE &= ~UCRXIE;
#endif
}

void MaskIntHdlr()
{
    IntIsMasked = TRUE;
}

void UnMaskIntHdlr()
{
    IntIsMasked = FALSE;
}

void set_rts()
{
	// MSP430F5529 P1.4, MSP432P401R = P5.6
	P5OUT |= BIT6;
}

void clear_rts()
{
	// MSP430F5529 P1.4, MSP432P401R = P5.6
    P5OUT &= ~BIT6;
}

void initClk()
{
    /* Setup XT1 and XT2 */
    /* Configuring pins for peripheral/crystal usage and LED for output */
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ,
            GPIO_PIN0 | GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Setting the external clock frequency. This API is optional, but will
     * come in handy if the user ever wants to use the getMCLK/getACLK/etc
     * functions
     */
    CS_setExternalClockSourceFrequency(32678,48000000);

    /* Starting LFXT in non-bypass mode without a timeout. */
    //CS_startLFXT(CS_LFXT_DRIVE3);

    /* Initializing MCLK to LFXT (effectively 32khz) */
    //MAP_CS_initClockSignal(CS_BCLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
    //MAP_CS_initClockSignal(CS_ACLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);

    MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
    MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);
    FLCTL_BANK0_RDCTL |= (FLCTL_BANK0_RDCTL_BUFI | FLCTL_BANK0_RDCTL_BUFD );
    FLCTL_BANK1_RDCTL |= (FLCTL_BANK1_RDCTL_BUFI | FLCTL_BANK1_RDCTL_BUFD );
    PCM_setPowerState(PCM_AM_DCDC_VCORE1);

    MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
    CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );

    /* Globally enable interrupts */
    __enable_interrupt();
}

void stopWDT()
{
    WDTCTL = WDTPW + WDTHOLD;
}

void initLEDs()
{
    P1OUT &= ~BIT0;
    P2OUT &= ~BIT2;
    P1DIR |= BIT0;
    P2DIR |= BIT2;
    P1SEL0 &= ~BIT0;
    P1SEL1 &= ~BIT0;
    P2SEL0 &= ~BIT2;
    P2SEL1 &= ~BIT2;
}

void turnLedOn(char ledNum)
{
    switch(ledNum)
    {
      case LED1:
        P1OUT |= BIT0;
        break;
      case LED2:
        P2OUT |= BIT2;
        break;
    }
}

void turnLedOff(char ledNum)
{
    switch(ledNum)
    {
      case LED1:
        P1OUT &= ~BIT0;
        break;
      case LED2:
        P2OUT &= ~BIT2;
        break;
    }
}


void toggleLed(char ledNum)
{
    switch(ledNum)
    {
      case LED1:
        P1OUT ^= BIT0;
        break;
      case LED2:
        P2OUT ^= BIT2;
        break;
    }

}

unsigned char GetLEDStatus()
{
  unsigned char status = 0;

  if(P1OUT & BIT0)
    status |= (1 << 0);
  if(P2OUT & BIT2)
    status |= (1 << 1);

  return status;
}

void initAntSelGPIO()
{
    P2OUT &= ~BIT7;
    P2OUT |=  BIT6; /* Select Antenna 1 */
    P2SEL0 &= ~(BIT6 + BIT7);
    P2SEL1 &= ~(BIT6 + BIT7);
    P2DIR |= (BIT6 + BIT7);
}

void SelAntenna(int antenna)
{
    switch(antenna)
   {
        case ANT1:
            P2OUT &= ~BIT7;
            P2OUT |=  BIT6;
            break;
        case ANT2:
            P2OUT &= ~BIT6;
            P2OUT |=  BIT7;
            break;
   }
}

//#pragma vector=PORT1_VECTOR
//__interrupt
void Port1_ISR(void)
{
    /* Context save interrupt flag before calling interrupt vector. */
    /* Reading interrupt vector generator will automatically clear IFG flag */
	// P1 interrupt not required.

//    switch (__even_in_range(P1IV, P1IV_P1IFG7))
//    {
//        /* Vector  P1IV_NONE:  No Interrupt pending */
//        case  P1IV_NONE:
//            break;
//
//        /* Vector  P1IV_P1IFG0:  P1IV P1IFG.0 */
//        case  P1IV_P1IFG0:
//            break;
//
//        /* Vector  P1IV_P1IFG1:  P1IV P1IFG.1 */
//        case  P1IV_P1IFG1:
//            break;
//
//        /* Vector  P1IV_P1IFG2:  P1IV P1IFG.2 */
//        case  P1IV_P1IFG2:
//            break;
//
//        /* Vector  P1IV_P1IFG3:  P1IV P1IFG.3 */
//        case  P1IV_P1IFG3:
//            break;
//
//        /* Vector  P1IV_P1IFG4:  P1IV P1IFG.4 */
//        case  P1IV_P1IFG4:
//            break;
//
//        /* Vector  P1IV_P1IFG5:  P1IV P1IFG.5 */
//        case  P1IV_P1IFG5:
//            break;
//
//        /* Vector  P1IV_P1IFG1:  P1IV P1IFG.6 */
//        case  P1IV_P1IFG6:
//            break;
//
//        /* Vector  P1IV_P1IFG7:  P1IV P1IFG.7 */
//        case  P1IV_P1IFG7:
//            break;
//
//        /* Default case */
//        default:
//            break;
//    }
}

void Delay(unsigned long interval)
{
    while(interval > 0)
    {
        __delay_cycles(48000);
        interval--;
    }
}


/*!
    \brief          The IntSpiGPIOHandler interrupt handler

    \param[in]      none

    \return         none

    \note

    \warning
*/
//#pragma vector=PORT2_VECTOR
//__interrupt

void IntSpiGPIOHandler(void)
{
    if (P2IFG & BIT5)
    {

#ifndef SL_IF_TYPE_UART
        if (pIraEventHandler)
        {
            pIraEventHandler(0);
        }
#else
        if(puartFlowctrl->bRtsSetByFlowControl == FALSE)
        {
            clear_rts();
        }

#endif
        P2IFG &= ~ BIT5;
    }

}

/*!
    \brief          The UART A0 interrupt handler

    \param[in]      none

    \return         none

    \note

    \warning
*/
//#pragma vector=USCI_A0_VECTOR
//__interrupt

void CC3100_UART_ISR(void)
{
#if 0
	switch(__even_in_range(UCA0IV,0x08))
    {
        case 0:break;                             /* Vector 0 - no interrupt */
        case 2:                                   /* Vector 2 - RXIF */
#ifdef SL_IF_TYPE_UART
        {
            unsigned char ByteRead;

            while((UCA0IFG & UCRXIFG) != 0);

            if(UCRXERR & UCA1STAT)
            {
                if(UCOE & UCA1STAT)
                {
                    error_overrun = TRUE;
                }
                ASSERT_UART(0);
            }

            ByteRead = UCA0RXBUF;

            if(puartFlowctrl->bActiveBufferIsJitterOne == TRUE)
            {
                if(puartFlowctrl->JitterBufferFreeBytes > 0)
                {
                    puartFlowctrl->JitterBuffer[puartFlowctrl->JitterBufferWriteIdx] = ByteRead;
                    puartFlowctrl->JitterBufferFreeBytes--;
                    puartFlowctrl->JitterBufferWriteIdx++;

                    if((FALSE == IntIsMasked) && (NULL != pIraEventHandler))
                    {
                        pIraEventHandler(0);
                    }
                }
                else
                {
                    if(P1OUT & BIT3)
                    {
                        ASSERT_UART(0);
                    }
                }

                if(puartFlowctrl->JitterBufferFreeBytes <= UART_READ_JITTER_RTS_GUARD)
                {
                    set_rts();
                    puartFlowctrl->bRtsSetByFlowControl = TRUE;
                }

                if(puartFlowctrl->JitterBufferWriteIdx > (UART_READ_JITTER_BUFFER_SIZE - 1))
                {
                    puartFlowctrl->JitterBufferWriteIdx = 0;
                }
            }
            else
            {
                puartFlowctrl->pActiveBuffer[puartFlowctrl->ActiveBufferWriteCounter++] = ByteRead;
            }
        }
#endif
            break;
        case 4:break;                             /* Vector 4 - TXIFG */
        default: break;
    }
#endif
}

/* Catch interrupt vectors that are not initialized. */
#if 0
#pragma vector=WDT_VECTOR, TIMER2_A0_VECTOR, ADC12_VECTOR, USCI_B1_VECTOR, \
    TIMER1_A0_VECTOR, TIMER1_A1_VECTOR, TIMER0_A1_VECTOR, TIMER0_A0_VECTOR, \
    TIMER2_A1_VECTOR, COMP_B_VECTOR, USB_UBM_VECTOR, UNMI_VECTOR,DMA_VECTOR, \
    TIMER0_B0_VECTOR, TIMER0_B1_VECTOR,SYSNMI_VECTOR, USCI_B0_VECTOR, RTC_VECTOR
__interrupt void Trap_ISR(void)
{
    while(1);
}

#endif

