/*
   This program recieves the IR Signal which follows the NEC Protocol.
   It uses TSOP 1738 module to detect IR signal, decode the signal received
   and then take the necessary action as per the code.

   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                REMOTE CODE USED
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Binary Values Used
      Address: 01001101
      ON: 01001111
      OFF:01000110
      INC_BRIGHT:01101010
      DEC_BRIGHT:01001010

   Hexadecimal Values
      ON: B24DF20D
      OFF:B24D629D
      INC_BRIGHT:B24D56A9
      DEC_BRIGHT:B24D52AD
*/

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~
        HEADER FILES
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#include<avr/io.h>
#include<avr/interrupt.h>



/*~~~~~~~~~~~~~~~~~~~~~~~~~~~
       GLOBAL VARIABLES
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
volatile int count = 0;                                       //To count the number of time signal goes from HIGH to LOW

volatile int timer_value = 0;                                 //

volatile int pulse_count = 0;                                 //To count number of pulses recieved out of 32 (since transmitter sends 32 pulse of IR signal which contains data)

volatile uint32_t msg_bit = 0;                                //To temporarily store the data received

volatile uint32_t new_key = 0;                                //To store the complete data recieved.



/*~~~~~~~~~~~~~~~~~~~~~~~~~~~
          ISRs
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*
   This ISR counts the number of time elasped between two consecutive
   HIGH to LOW transition of the pulse.
*/
ISR(TIMER0_COMPA_vect) {
  if (count < 50) {
    ++count;
  }
}

/*
   This ISR is invoked whenever pulse goes from HIGH to LOW
*/
ISR(INT0_vect) {
  timer_value = count;
  count = 0;
  TCNT0 = 0;
  ++pulse_count;

  if (timer_value >= 50) {
    pulse_count = -2;
    msg_bit = 0;
  }
  else if ((pulse_count >= 0) && (pulse_count <= 31)) {
    if (timer_value >= 2) {
      msg_bit |= (uint32_t)1 << (31 - pulse_count);
    }
    else {
    }
    if (pulse_count == 31) {
      EICRA = (1 << ISC00);
    }
  }
  else if (pulse_count >= 32)
  {
    new_key = msg_bit;
    pulse_count = 0;
    EICRA = (1 << ISC01);
  }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~
      OTHER FUNCTIONS
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*
   This fuction enable the external interrupt INT0
*/
void setup_external_interrupt() {
  EICRA = (1 << ISC01);                             //Fire interrupt on falling edge
  EIMSK = (1 << INT0);                              //Enable INT0 interrupt
}

/*
   Ths function initiallise Timer0
*/
void init_timer0() {
  TCCR0A = (1 << WGM01);                      //Clear timer on compare match
  TIMSK0 = (1 << OCIE0A);                     //Enable compare match interrupt
  OCR0A = 250;
}

/*
   This function starts the TIMER0
*/
void timer0_start() {
  TCCR0B = (1 << CS01) | (1 << CS00);               //Prescaling of 64
  TCNT0 = 0;
}

//Function to stop TIMER0
void timer0_stop() {
  TCCR0B = 0x00;
}

/*
   Function to print the current status of LED
*/
void print_status(int stat, int bright) {
  Serial.println("Current Status:");
  Serial.print('\t'); Serial.print('\t');
  Serial.print("LED: ");
  if (stat == 0) {
    Serial.print("OFF");
  }
  else {
    Serial.print("ON ");
  }
  Serial.print('\t');
  Serial.print("Brightness: ");
  if (bright < 0) {
    Serial.println("-");
  }
  else {
    Serial.print(bright * 10);
    Serial.print("% ");
  }
  Serial.print('\n');
}

/*
   Function to attach PWM for adjusting brightness of LED using TIMER2
*/
void attach_pwm() {
  TCCR2A |= (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);        //PWM, fast
  TCCR2B |= (1 << CS20);                                        //No prescaling
  OCR2A = 25.5;
}

/*
   Function to de-attach PWM
*/
void remove_pwm() {
  TCCR2A = 0x00;
  TCCR2B = 0x00;
  OCR2A = 0;
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~
      MAIN FUNCTION
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
int main() {

  DDRD = 0x00;                                              //PORTD Input.

  setup_external_interrupt();                               //Enable external interrupt INT0

  init_timer0();                                            //Initiallise Timer0

  timer0_start();                                           //Start Timer0
  sei();

  DDRB = 0xFF;                                              //Port B OUTPUT

  PORTB = 0x00;                                             //Turn all gate close.

  bool LED_stat = false;                                    //Boolean to tell about the status of LED.

  int bright = -1;                                          // To track brightness of the LED (-1 indicates LED OFF)

  Serial.begin(9600);                                       //Initiallise USART

  Serial.println("Reciever Ready");

  print_status(LED_stat, bright) ;

  while (1) {
    if (new_key) {                                                        //If new_key has been detected
      if (new_key == 0xB24DF20D) {                                        // Turning LED ON
        Serial.println("\n New Signal Recieved..");
        LED_stat = true;
        PORTB = 0xFF;
        attach_pwm();
        bright = 1;                                                       //When LED is turned on, make its brightness to 1.
        print_status(LED_stat, bright) ;
      }
      else if (new_key == 0xB24D629D) {                                   //Turning LED off
        Serial.println("\n   New Signal Recieved..");
        LED_stat = false;
        PORTB = 0x00;
        remove_pwm();
        OCR2A = 25.5;
        bright = -1;
        print_status(LED_stat, bright) ;
      }
      else if (new_key == 0xB24D56A9) {                                   //Increasing the brightness (if LED is on)
        if (bright <= 9 & LED_stat == 1) {
          Serial.println("\n New Signal Recieved..");
          bright += 1;
          OCR2A += 25.5;
          print_status(LED_stat, bright) ;
        }
      }
      else if (new_key == 0xB24D52AD) {
        if (bright > 1 & LED_stat == 1) {                                 //Decreasing the brightness (if LED is on)
          Serial.println("\n New Signal Recieved..");
          bright -= 1;
          OCR2A -= 25.5;
          print_status(LED_stat, bright) ;
        }
      }
      new_key = 0;                                                        //Reset the new_key to 0 and wait for the new key.
    }
  }
  return 0;
}


