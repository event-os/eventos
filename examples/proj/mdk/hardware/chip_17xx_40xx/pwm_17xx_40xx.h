/*
 * pwm_17xx_40xx.h
 *
 *  Created on: Apr 17, 2014
 *      Author: Özen Özkaya
 */

#ifndef PWM_17XX_40XX_H_
#define PWM_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "core_cm4.h"

    typedef struct {                                        /*!< TIMERn Structure       */
            __IO uint32_t IR;                               /*!< Interrupt Register. The IR can be written to clear interrupts. The IR can be read to identify which of eight possible interrupt sources are pending. */
            __IO uint32_t TCR;                              /*!< Timer Control Register. The TCR is used to control the Timer Counter functions. The Timer Counter can be disabled or reset through the TCR. */
            __IO uint32_t TC;                               /*!< Timer Counter. The 32 bit TC is incremented every PR+1 cycles of PCLK. The TC is controlled through the TCR. */
            __IO uint32_t PR;                               /*!< Prescale Register. The Prescale Counter (below) is equal to this value, the next clock increments the TC and clears the PC. */
            __IO uint32_t PC;                               /*!< Prescale Counter. The 32 bit PC is a counter which is incremented to the value stored in PR. When the value in PR is reached, the TC is incremented and the PC is cleared. The PC is observable and controllable through the bus interface. */
            __IO uint32_t MCR;                              /*!< Match Control Register. The MCR is used to control if an interrupt is generated and if the TC is reset when a Match occurs. */
            __IO uint32_t MR0;                    /*!< Match Register. MR can be enabled through the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt every time MR matches the TC. */
            __IO uint32_t MR1;                    /*!< Match Register. MR can be enabled through the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt every time MR matches the TC. */
            __IO uint32_t MR2;                    /*!< Match Register. MR can be enabled through the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt every time MR matches the TC. */
            __IO uint32_t MR3;                    /*!< Match Register. MR can be enabled through the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt every time MR matches the TC. */
            __IO uint32_t CCR;                              /*!< Capture Control Register. The CCR controls which edges of the capture inputs are used to load the Capture Registers and whether or not an interrupt is generated when a capture takes place. */
            __IO uint32_t CR[2];                    /*!< Capture Register. CR is loaded with the value of TC when there is an event on the CAPn.0 input. */
            __IO uint32_t RESERVED[3];
            __IO uint32_t MR4;                    /*!< Match Register. MR can be enabled through the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt every time MR matches the TC. */
            __IO uint32_t MR5;                    /*!< Match Register. MR can be enabled through the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt every time MR matches the TC. */
            __IO uint32_t MR6;                    /*!< Match Register. MR can be enabled through the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt every time MR matches the TC. */
            __IO uint32_t PCR;
            __IO uint32_t LER;
            __IO uint32_t CTCR;
    } LPC_PWM_T;

#define LPC_PWM0                  ((LPC_PWM_T              *) LPC_PWM0_BASE)
#define LPC_PWM1                  ((LPC_PWM_T              *) LPC_PWM1_BASE)


typedef enum
{
	PWM_0 = 0,
	PWM_1 = 1
} en_PWM_unitId;
	

typedef enum 
{
	PWM_MATCH_UPDATE_NOW = 0,			/**< PWM Match Channel Update Now */
	PWM_MATCH_UPDATE_NEXT_RST			/**< PWM Match Channel Update on next
											PWM Counter resetting */
} PWM_MATCH_UPDATE_OPT;

/** TCR register mask */
#define PWM_TCR_BITMASK				((uint32_t)(0x0000000B))

/** Macro to clear interrupt pending */
#define PWM_IR_CLR(n)         _BIT(n)

/** Macro for getting a timer match interrupt bit */
#define PWM_MATCH_INT(n)      (_BIT((n) & 0x0F))
/** Macro for getting a capture event interrupt bit */
#define PWM_CAP_INT(n)        (_BIT((((n) & 0x0F) + 4)))

/** Timer/counter enable bit */
#define PWM_TIM_ENABLE            ((uint32_t) (1 << 0))
/** Timer/counter reset bit */
#define PWM_TIM_RESET             ((uint32_t) (1 << 1))

/** Timer/counter enable bit */
#define PWM_ENABLE            ((uint32_t) (1 << 3))
/** Timer/counter reset bit */
#define PWM_RESET             ((uint32_t) (1 << 3))

/** Bit location for interrupt on MRx match, n = 0 to 3 */
#define PWM_INT_ON_MATCH(n)   (_BIT(((n) * 3)))
/** Bit location for reset on MRx match, n = 0 to 3 */
#define PWM_RESET_ON_MATCH(n) (_BIT((((n) * 3) + 1)))
/** Bit location for stop on MRx match, n = 0 to 3 */
#define PWM_STOP_ON_MATCH(n)  (_BIT((((n) * 3) + 2)))

/** Bit location for CAP.n on CRx rising edge, n = 0 to 3 */
#define PWM_CAP_RISING(n)     (_BIT(((n) * 3)))
/** Bit location for CAP.n on CRx falling edge, n = 0 to 3 */
#define PWM_CAP_FALLING(n)    (_BIT((((n) * 3) + 1)))
/** Bit location for CAP.n on CRx interrupt enable, n = 0 to 3 */
#define PWM_INT_ON_CAP(n)     (_BIT((((n) * 3) + 2)))

#define PWM_APPEND_MR_MATCH(x)  (pTMR->MR##x)

typedef enum
{
 PWM_SINGLE_EDGE_CONTROL_MODE=0,
 PWM_DOUBLE_EDGE_CONTROL_MODE=1,
}PWM_EDGE_CONTROL_MODE;

typedef enum
{
    PWM_OUT_DISABLED=0,
    PWM_OUT_ENABLED=1,
}PWM_OUT_CMD;



/**
 * @brief       Initialize a pwm
 * @param       pTMR    : Pointer to pwm IP register address
 * @return      Nothing
 */
void Chip_PWM_Init(LPC_PWM_T *pTMR);

/**
 * @brief       Shutdown a pwm
 * @param       pTMR    : Pointer to pwm IP register address
 * @return      Nothing
 */
void Chip_PWM_DeInit(LPC_PWM_T *pTMR);

/**
 * @brief       Determine if a match interrupt is pending
 * @param       pTMR            : Pointer to pwm IP register address
 * @param       matchnum        : Match interrupt number to check
 * @return      false if the interrupt is not pending, otherwise true
 * @note        Determine if the match interrupt for the passed timer and match
 * counter is pending.
 */
STATIC INLINE bool Chip_PWM_MatchPending(LPC_PWM_T *pTMR, int8_t matchnum)
{
        return (bool) ((pTMR->IR & PWM_MATCH_INT(matchnum)) != 0);
}


/**
 * @brief       Determine if a capture interrupt is pending
 * @param       pTMR    : Pointer to pwm IP register address
 * @param       capnum  : Capture interrupt number to check
 * @return      false if the interrupt is not pending, otherwise true
 * @note        Determine if the capture interrupt for the passed capture pin is
 * pending.
 */
STATIC INLINE bool Chip_PWM_CapturePending(LPC_PWM_T *pTMR, int8_t capnum)
{
        return (bool) ((pTMR->IR & PWM_CAP_INT(capnum)) != 0);
}


/**
 * @brief       Clears a (pending) match interrupt
 * @param       pTMR            : Pointer to pwm IP register address
 * @param       matchnum        : Match interrupt number to clear
 * @return      Nothing
 * @note        Clears a pending timer match interrupt.
 */
STATIC INLINE void Chip_PWM_ClearMatch(LPC_PWM_T *pTMR, int8_t matchnum)
{
        pTMR->IR = PWM_IR_CLR(matchnum);
}


/**
 * @brief       Clears a (pending) capture interrupt
 * @param       pTMR    : Pointer to pwm IP register address
 * @param       capnum  : Capture interrupt number to clear
 * @return      Nothing
 * @note        Clears a pending timer capture interrupt.
 */
STATIC INLINE void Chip_PWM_ClearCapture(LPC_PWM_T *pTMR, int8_t capnum)
{
        pTMR->IR = (0x10 << capnum);
}

/**
 * @brief       Enables the timer (starts count)
 * @param       pTMR    : Pointer to timer IP register address
 * @return      Nothing
 * @note        Enables the timer to start counting.
 */
STATIC INLINE void Chip_PWM_Enable(LPC_PWM_T *pTMR)
{
        pTMR->TCR |= PWM_TIM_ENABLE;
        pTMR->TCR |= PWM_ENABLE;
}


/**
 * @brief       Disables the timer (stops count)
 * @param       pTMR    : Pointer to timer IP register address
 * @return      Nothing
 * @note        Disables the timer to stop counting.
 */
STATIC INLINE void Chip_PWM_Disable(LPC_PWM_T *pTMR)
{
        pTMR->TCR &= ~PWM_ENABLE;
        pTMR->TCR &=~PWM_TIM_ENABLE;
}



/**
 * @brief       Returns the current timer count
 * @param       pTMR    : Pointer to timer IP register address
 * @return      Current timer terminal count value
 * @note        Returns the current timer terminal count.
 */
STATIC INLINE uint32_t Chip_PWM_ReadCount(LPC_PWM_T *pTMR)
{
        return pTMR->TC;
}


/**
 * @brief       Returns the current prescale count
 * @param       pTMR    : Pointer to pwm IP register address
 * @return      Current timer prescale count value
 * @note        Returns the current prescale count.
 */
STATIC INLINE uint32_t Chip_PWM_ReadPrescale(LPC_PWM_T *pTMR)
{
        return pTMR->PC;
}


/**
 * @brief       Sets the prescaler value
 * @param       pTMR            : Pointer to pwm IP register address
 * @param       prescale        : Prescale value to set the prescale register to
 * @return      Nothing
 * @note        Sets the prescale count value.
 */
STATIC INLINE void Chip_PWM_PrescaleSet(LPC_PWM_T *pTMR, uint32_t prescale)
{
        pTMR->PR = prescale;
}


/**
 * @brief       Sets a timer match value
 * @param       pTMR            : Pointer to timer IP register address
 * @param       matchnum        : Match timer to set match count for
 * @param       matchval        : Match value for the selected match count
 * @return      Nothing
 * @note        Sets one of the timer match values.
 */


STATIC INLINE void Chip_PWM_SetMatch(LPC_PWM_T *pTMR, int8_t matchnum, uint32_t matchval)
{

//        pTMR->MR[matchnum] = matchval;
    switch (matchnum) {
        case 0:
            pTMR->MR0=matchval;
            break;
        case 1:
            pTMR->MR1=matchval;
            break;
        case 2:
            pTMR->MR2=matchval;
            break;
        case 3:
            pTMR->MR3=matchval;
            break;
        case 4:
            pTMR->MR4=matchval;
            break;
        case 5:
            pTMR->MR5=matchval;
            break;
        case 6:
            pTMR->MR6=matchval;
            break;
        default:
            break;
    }
//    PWM_APPEND_MR_MATCH(matchnum) =matchval;
}


/**
 * @brief       Reads a capture register
 * @param       pTMR    : Pointer to timer IP register address
 * @param       capnum  : Capture register to read
 * @return      The selected capture register value
 * @note        Returns the selected capture register value.
 */
STATIC INLINE uint32_t Chip_PWM_ReadCapture(LPC_PWM_T *pTMR, int8_t capnum)
{
        return pTMR->CR[capnum];
}


/**
 * @brief       Resets the timer terminal and prescale counts to 0
 * @param       pTMR    : Pointer to timer IP register address
 * @return      Nothing
 */
void Chip_PWM_Reset(LPC_PWM_T *pTMR);



/**
 * @brief       Enables a match interrupt that fires when the terminal count
 *                      matches the match counter value.
 * @param       pTMR            : Pointer to timer IP register address
 * @param       matchnum        : Match timer, 0 to 3
 * @return      Nothing
 */
STATIC INLINE void Chip_PWM_MatchEnableInt(LPC_PWM_T *pTMR, int8_t matchnum)
{
        pTMR->MCR |= PWM_INT_ON_MATCH(matchnum);
}



/**
 * @brief       Disables a match interrupt for a match counter.
 * @param       pTMR            : Pointer to timer IP register address
 * @param       matchnum        : Match timer, 0 to 3
 * @return      Nothing
 */
STATIC INLINE void Chip_PWM_MatchDisableInt(LPC_PWM_T *pTMR, int8_t matchnum)
{
        pTMR->MCR &= ~PWM_INT_ON_MATCH(matchnum);
}


/**
 * @brief       For the specific match counter, enables reset of the terminal count register when a match occurs
 * @param       pTMR            : Pointer to timer IP register address
 * @param       matchnum        : Match timer, 0 to 3
 * @return      Nothing
 */
STATIC INLINE void Chip_PWM_ResetOnMatchEnable(LPC_PWM_T *pTMR, int8_t matchnum)
{
        pTMR->MCR |= PWM_RESET_ON_MATCH(matchnum);
}


/**
 * @brief       For the specific match counter, disables reset of the terminal count register when a match occurs
 * @param       pTMR            : Pointer to timer IP register address
 * @param       matchnum        : Match timer, 0 to 3
 * @return      Nothing
 */
STATIC INLINE void Chip_PWM_ResetOnMatchDisable(LPC_PWM_T *pTMR, int8_t matchnum)
{
        pTMR->MCR &= ~PWM_RESET_ON_MATCH(matchnum);
}




/**
 * @brief       Enable a match timer to stop the terminal count when a
 *                      match count equals the terminal count.
 * @param       pTMR            : Pointer to timer IP register address
 * @param       matchnum        : Match timer, 0 to 3
 * @return      Nothing
 */
STATIC INLINE void Chip_PWM_StopOnMatchEnable(LPC_PWM_T *pTMR, int8_t matchnum)
{
        pTMR->MCR |= PWM_STOP_ON_MATCH(matchnum);
}


/**
 * @brief       Disable stop on match for a match timer. Disables a match timer
 *                      to stop the terminal count when a match count equals the terminal count.
 * @param       pTMR            : Pointer to timer IP register address
 * @param       matchnum        : Match timer, 0 to 3
 * @return      Nothing
 */
STATIC INLINE void Chip_PWM_StopOnMatchDisable(LPC_PWM_T *pTMR, int8_t matchnum)
{
        pTMR->MCR &= ~PWM_STOP_ON_MATCH(matchnum);
}



/**
 * @brief       Enables capture on on rising edge of selected CAP signal for the
 *                      selected capture register, enables the selected CAPn.capnum signal to load
 *                      the capture register with the terminal coount on a rising edge.
 * @param       pTMR    : Pointer to timer IP register address
 * @param       capnum  : Capture signal/register to use
 * @return      Nothing
 */
STATIC INLINE void Chip_PWM_CaptureRisingEdgeEnable(LPC_PWM_T *pTMR, int8_t capnum)
{
        pTMR->CCR |= PWM_CAP_RISING(capnum);
}



/**
 * @brief       Disables capture on on rising edge of selected CAP signal. For the
 *                      selected capture register, disables the selected CAPn.capnum signal to load
 *                      the capture register with the terminal coount on a rising edge.
 * @param       pTMR    : Pointer to timer IP register address
 * @param       capnum  : Capture signal/register to use
 * @return      Nothing
 */
STATIC INLINE void Chip_PWM_CaptureRisingEdgeDisable(LPC_PWM_T *pTMR, int8_t capnum)
{
        pTMR->CCR &= ~PWM_CAP_RISING(capnum);
}



/**
 * @brief       Enables capture on on falling edge of selected CAP signal. For the
 *                      selected capture register, enables the selected CAPn.capnum signal to load
 *                      the capture register with the terminal coount on a falling edge.
 * @param       pTMR    : Pointer to timer IP register address
 * @param       capnum  : Capture signal/register to use
 * @return      Nothing
 */
STATIC INLINE void Chip_PWM_CaptureFallingEdgeEnable(LPC_PWM_T *pTMR, int8_t capnum)
{
        pTMR->CCR |= PWM_CAP_FALLING(capnum);
}


/**
 * @brief       Disables capture on on falling edge of selected CAP signal. For the
 *                      selected capture register, disables the selected CAPn.capnum signal to load
 *                      the capture register with the terminal coount on a falling edge.
 * @param       pTMR    : Pointer to timer IP register address
 * @param       capnum  : Capture signal/register to use
 * @return      Nothing
 */
STATIC INLINE void Chip_PWM_CaptureFallingEdgeDisable(LPC_PWM_T *pTMR, int8_t capnum)
{
        pTMR->CCR &= ~PWM_CAP_FALLING(capnum);
}

/**
 * @brief       Enables interrupt on capture of selected CAP signal. For the
 *                      selected capture register, an interrupt will be generated when the enabled
 *                      rising or falling edge on CAPn.capnum is detected.
 * @param       pTMR    : Pointer to timer IP register address
 * @param       capnum  : Capture signal/register to use
 * @return      Nothing
 */
STATIC INLINE void Chip_PWM_CaptureEnableInt(LPC_PWM_T *pTMR, int8_t capnum)
{
        pTMR->CCR |= PWM_INT_ON_CAP(capnum);
}


/**
 * @brief       Disables interrupt on capture of selected CAP signal
 * @param       pTMR    : Pointer to timer IP register address
 * @param       capnum  : Capture signal/register to use
 * @return      Nothing
 */
STATIC INLINE void Chip_PWM_CaptureDisableInt(LPC_PWM_T *pTMR, int8_t capnum)
{
        pTMR->CCR &= ~PWM_INT_ON_CAP(capnum);
}


/**
 * @brief Standard timer initial match pin state and change state
 */
typedef enum IP_PWM_PIN_MATCH_STATE {
    PWM_EXTMATCH_DO_NOTHING = 0,  /*!< Timer match state does nothing on match pin */
    PWM_EXTMATCH_CLEAR      = 1,  /*!< Timer match state sets match pin low */
    PWM_EXTMATCH_SET        = 2,  /*!< Timer match state sets match pin high */
    PWM_EXTMATCH_TOGGLE     = 3   /*!< Timer match state toggles match pin */
} PWM_PIN_MATCH_STATE_T;

/**
 * @brief       Sets external match control (MATn.matchnum) pin control. For the pin
 *          selected with matchnum, sets the function of the pin that occurs on
 *          a terminal count match for the match count.
 * @param       pTMR                    : Pointer to timer IP register address
 * @param       initial_state   : Initial state of the pin, high(1) or low(0)
 * @param       matchState              : Selects the match state for the pin
 * @param       matchnum                : MATn.matchnum signal to use
 * @return      Nothing
 * @note        For the pin selected with matchnum, sets the function of the pin that occurs on
 * a terminal count match for the match count.
 */
void Chip_PWM_ExtMatchControlSet(LPC_PWM_T *pTMR, int8_t initial_state, PWM_PIN_MATCH_STATE_T matchState, int8_t matchnum);


/**
 * @brief Standard timer clock and edge for count source
 */
typedef enum IP_PWM_CAP_SRC_STATE {
        PWM_CAPSRC_RISING_PCLK  = 0,  /*!< Timer ticks on PCLK rising edge */
        PWM_CAPSRC_RISING_CAPN  = 1,  /*!< Timer ticks on CAPn.x rising edge */
        PWM_CAPSRC_FALLING_CAPN = 2,  /*!< Timer ticks on CAPn.x falling edge */
        PWM_CAPSRC_BOTH_CAPN    = 3   /*!< Timer ticks on CAPn.x both edges */
} PWM_CAP_SRC_STATE_T;


/**
 * @brief       Sets timer count source and edge with the selected passed from CapSrc.
 *          If CapSrc selected a CAPn pin, select the specific CAPn pin with the capnum value.
 * @param       pTMR    : Pointer to timer IP register address
 * @param       capSrc  : timer clock source and edge
 * @param       capnum  : CAPn.capnum pin to use (if used)
 * @return      Nothing
 * @note        If CapSrc selected a CAPn pin, select the specific CAPn pin with the capnum value.
 */
STATIC INLINE void Chip_PWM_SetCountClockSrc(LPC_PWM_T *pTMR,PWM_CAP_SRC_STATE_T capSrc,int8_t capnum)
{
        pTMR->CTCR = (uint32_t) capSrc | ((uint32_t) capnum) << 2;
}



void Chip_PWM_SetControlMode(LPC_PWM_T *pTMR, uint8_t pwmChannel,PWM_EDGE_CONTROL_MODE pwmEdgeMode, PWM_OUT_CMD pwmCmd );

void Chip_PWM_LatchEnable(LPC_PWM_T *pTMR, uint8_t pwmChannel, PWM_OUT_CMD pwmCmd );

void PWM_MatchUpdate(uint8_t pwmId, uint8_t MatchChannel,
										uint32_t MatchValue, uint8_t UpdateType);

#ifdef __cplusplus
}
#endif

#endif /* PWM_17XX_40XX_H_ */
