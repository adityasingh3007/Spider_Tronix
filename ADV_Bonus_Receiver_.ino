/*
   This program recieves the IR Signal which follows the NEC Protocol.
   It uses TSOP 1738 module to detect IR signal, decode the signal received
   and then take the necessary action as per the code.

   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            INSTRUCTIONS
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    1. Firstly, before a user can control the LED, he has to first set a "Timeout Time".
    2. Timeout Time means the duration till when remote will be active and hence will recognise the gesture.
    3. As soon as the setted time expires he has to again set the time.
    4. The max time, that can be setted is 10 mins. (Can be extened by changing the code accrodingly)
    5. After setting the timeout time now he can control the LED.
    
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                REMOTE CODE USED
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Binary Values Used
      Address: 00000000
      ON: 00000011 
      OFF:00001001
      INC_BRIGHT:00001110
      DEC_BRIGHT:00001010

   Hexadecimal Values
      ON: FFC03F
      OFF:FF906F
      INC_BRIGHT:FF708F
      DEC_BRIGHT:FF58A7
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

volatile long int count_on_off = 0;                           //Count variable to calculate time elasped since Timeout Time has been setted.

volatile long int thresh_value_on_off = 14;                 //Threshold value for Timeout Time. (If "count_on_off" met this value then time setted has elasped.)

volatile int change_time_ = 0;                                //To flag when Timeout Time has expired.

int on_off_time_set = 0;                                      //Variable to check whether Timeout Time has been setted or not.

int time_ = 0;                                                //Variable to store the Timeout Time.


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
   This routine will keep an eye on Time out time.
*/
ISR(TIMER1_OVF_vect) {
  if (on_off_time_set == 1) {
    ++count_on_off;
  }
  if (count_on_off > thresh_value_on_off) {
    on_off_time_set = 0;
    count_on_off = 0;
    time_ = 0;
    change_time_ = 1;
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

/*
  Function to alert the user about Timeout Time
*/
void ask_on_off_time() {
  Serial.println("To continue using the Sensor.. ");
  Serial.println("Set the on/off time first :");
}

/*
   Function to Print the Timeout Time
*/
void print_set_time() {
  Serial.print("\tCurrent Setted time: ");
  Serial.print(time_);
  Serial.println(" min(s)");
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~
      MAIN FUNCTION
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
int main() {

  DDRD = 0x00;                                              //PORTD Input.

  setup_external_interrupt();                               //Enable external interrupt INT0

  init_timer0();                                            //Initiallise Timer0

  timer0_start();                                           //Start Timer0

  TCCR1B = (1 << CS12) | (1 << CS10);          //Timer1 Normal mode, 1024 Prescaler
  TCNT1 = 0;
  //Enable timer overflow interrupt for timer 2
  TIMSK1 = (1 << TOIE1);
  sei();

  DDRB = 0xFF;                                              //Port B OUTPUT

  PORTB = 0x00;                                             //Turn all gate close.

  bool LED_stat = false;                                    //Boolean to tell about the status of LED.

  int bright = -1;                                          // To track brightness of the LED (-1 indicates LED OFF)

  Serial.begin(9600);                                       //Initiallise USART

  Serial.println("Reciever INACTIVE");
  Serial.println(" ");
  ask_on_off_time();
  print_set_time();
  
  while (1) {
    if (change_time_ == 1) {                                //Whenever the Timeout Time will expire, the program will enter here
      change_time_ = 0;
      Serial.println("");
      Serial.println("Reciever INACTIVE");
      Serial.println("");
      Serial.println("You time has expired..");
      ask_on_off_time();
      print_set_time();
    }

     if (new_key) {                                                       //If new_key has been detected
      if (on_off_time_set == 1) {                                         //If Timeout Time has been setted then program will go here.
        if (new_key == 0xFFC03F) {                                        // Turning LED ON
          Serial.println("\n New Signal Recieved..");
          LED_stat = true;
          PORTB = 0xFF;
          attach_pwm();
          bright = 1;                                                      //When LED is turned on, make its brightness to 1.
          print_status(LED_stat, bright) ;
        }
        else if (new_key == 0xFF906F) {                                   //Turning LED off
          Serial.println("\n   New Signal Recieved..");
          LED_stat = false;
          PORTB = 0x00;
          remove_pwm();
          OCR2A = 25.5;
          bright = -1;
          print_status(LED_stat, bright) ;
        }
        else if (new_key == 0xFF708F) {                                   //Increasing the brightness (if LED is on)
          if (bright <= 9 & LED_stat == 1) {
            Serial.println("\n New Signal Recieved..");
            bright += 1;
            OCR2A += 25.5;
            print_status(LED_stat, bright) ;
          }
        }
        else if (new_key == 0xFF58A7) {
          if (bright > 1 & LED_stat == 1) {                                 //Decreasing the brightness (if LED is on)
            Serial.println("\n New Signal Recieved..");
            bright -= 1;
            OCR2A -= 25.5;
            print_status(LED_stat, bright) ;
          }
        }
    }
    else {                                                              // Program will come here if Timeout has not been setted or it expires.
        if (new_key == 0xFF708F) {                                      
          ++time_;
          if (time_ >10) {
            time_ = 10;
          }
           print_set_time();
        }
        else if (new_key == 0xFF58A7) {
             --time_;
            if (time_ < 0) {
              time_ = 0;
            }
            print_set_time();
        } 
       else if (new_key == 0xFFC03F) {                                       
            Serial.print("    Timer Setted at ");
            Serial.print(time_);
            Serial.println(" min(s)");
            Serial.print("You can now control the light for the next ");
            Serial.print(time_);
            Serial.println(" min(s)");
            Serial.println("");
            Serial.println("Receiver ACTIVE");
            on_off_time_set = 1;
            thresh_value_on_off *= time_;                                                     //Update the threshold timer variable.
        }
      else if (new_key == 0xFF906F) {                                   
            Serial.println("   Timer Resetted");
            time_ = 0;
            print_set_time();
        } 
    }
    new_key = 0;                                                        //Reset the new_key to 0 and wait for the new key.
   }
  }
  return 0;
}


