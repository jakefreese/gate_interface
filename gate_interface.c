#include <msp430.h>

#define LEDR BIT0
#define LEDG BIT6
#define BUTTON BIT3

const int TICKS_PER_SECOND = 32;
enum {state_waiting, state_hold_high};

unsigned int current_state = state_waiting;
unsigned int hold_timer = 0;

void init(void)
{

  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  // Enable LED outputs and Button pullup
  P1OUT = BUTTON;
  P1DIR = LEDR + LEDG;
  P1REN = BUTTON;



  // Set up 32768Hz crystal
  BCSCTL3 |= XCAP_3;                              // select 12pF caps

  // initialize Timer0_A
  TA0CCR0 = ( 32768 / TICKS_PER_SECOND ) - 1;     // set up timer for 32Hz
  TA0CTL = TASSEL_1 + ID_0 + MC_1;                // configure and start timer

  // enable interrupts
  TA0CCTL0 = CCIE;                                // enable timer CCR0 interrupts
  __enable_interrupt();                           // set GIE in SR
  LPM3;                                           // select low power mode 3
  while(1);
}


/**
* Timer interrupt called at 32Hz (TICKS_PER_SECOND)
*/
#pragma vector = TIMER0_A0_VECTOR
__interrupt void myTimerISR(void)
{

  switch( current_state )
  {
    /* state waiting */
    case state_waiting:
      if( ( P1IN & BUTTON ) == 0 )
      {
        if( hold_timer >= TICKS_PER_SECOND * 2 )
        {
          /* If button held for 2 seconds change state */
          current_state = state_hold_high;
          hold_timer = 0;
          break;
        }
        else
        {
          /* If button pressed, but not for 2 seconds yet inc timer */
          hold_timer++;
        }
      }
      else
      {
        /* Button not pressed, reset timer */
        hold_timer = 0;
      }
      break;

    /* state hold high */
    case state_hold_high:
      /* Set output high */
      P1OUT |= LEDR;

      if( hold_timer >= TICKS_PER_SECOND * 60 )
      {
        /* If timer has elapsed, set output LOW, switch state */
        P1OUT &= ~LEDR;
        hold_timer = 0;
        current_state = state_waiting;
        break;
      }
      else
      {
        hold_timer++;
      }
      break;

    /* return to a idle state */
    default:
      current_state = state_waiting;
      break;
  }
}

void main( void )
{
  init();
}
