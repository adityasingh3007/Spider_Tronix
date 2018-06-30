/*
   The following program reads the accelerometer value(X axis and Y axis only) of sensor MPU 6050
   and executes the corresponding command i.e to emit an IR signal according to gesture.
   IR signal generated follows NEC protocol.

   MPU 6050 is communicated with the microcontroller using I2C communication protocol.

   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            INSTRUCTIONS
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    1. Firstly, before a user can control the LED, he has to first set a "Timeout Time".
    2. Timeout Time means the duration till when remote will be active and hence will recognise the gesture.
    3. As soon as the setted time expires he has to again set the time.
    4. The max time, that can be setted is 10 mins. (Can be extened by changing the code accrodingly)
    5. After setting the timeout time now he can control the LED.

   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
             GESTURES USED
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    To make a gesture just move your wrist up,down,left,right.
      1. Setting the "Timeout Time".
            A. To decrease the time, MOVE UP.
            B. To increse the time, MOVE DOWN.
            C. To reset the time, MOVE LEFT.
            D. To fix that time, MOVE RIGHT.

      2. To control the LED.
            A. To decrease the brightness, MOVE UP.
            B. To inrease the brightness, MOVE DOWN.
            C. To turn LED OFF, MOVE LEFT.
            D. To turn LED ON, MOVE RIGHT.

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
#include<util/delay.h>
#include<util/twi.h>


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~
       GLOBAL VARIABLES
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
uint8_t MPU_addr = 0x68;                                     //MPU_6050 address

int X, Y;                                                   //Variables to store the accelerometer readings(in DEC form)

int16_t AcX, AcY;                                           //Variables to store the accelerometer readings(in BINARY form)

int Hist_Ax, Hist_Ay = 0;                                   //Variables to store the just previous value of the sensor readings(in DEC form).

int Cal_Ax, Cal_Ay = 0;                                     //Variables to store ethe initial readings of the sensor (Used as calibrated value). (in DEC form)

/*
   Various codes to trigger LED as shown
*/
uint32_t code_ON = 0b10110010010011011111001000001101;                  //LED On
uint32_t code_OFF = 0b10110010010011010110001010011101;                 //Led Off
uint32_t code_INC_BRIGHT = 0b10110010010011010101011010101001;          //Increase Brightness
uint32_t code_DEC_BRIGHT = 0b10110010010011010101001010101101;          //Decrease Brightness
uint32_t code;                                                          //Temp variable to store the code which will be transmitted by the IR Led.

volatile long int count_delay = 0;                                                 //Count variable to inc delay
volatile long int count_on_off = 0;                                                //Count variable to calculate time elasped since Timeout Time has been setted.

volatile int flag_delay;                                                           //To flag whether required delay is over or not.
volatile int change_time_ = 0;                                                     //To flag when Timeout Time has expired.

volatile int thresh_value_delay = 15;                                              //Threshold value for delay.(If "count_delay" met this value then 245ms delay has occured)
volatile long int thresh_value_on_off = 3780;                                      //Threshold value for Timeout Time. (If "count_on_off" met this value then time setted has elasped.)


int on_off_time_set = 0;                                                           //Variable to check whether Timeout Time has been setted or not.
int led_stat = 0;                                                                  //Variable to check the status of LED.

int time_ = 0;                                                                     //Variable to store the Timeout Time.


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~
          ISRs
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*
   This routine will toggle the IR LED pin ON/OFF
*/
ISR(TIMER0_COMPA_vect) {
  PORTD ^= (1 << 2);
}

/*
   This routuine will stop the current job being executed when transmitting the IR signal.
*/
ISR(TIMER1_COMPA_vect) {
  TCCR1B = 0x00;
  TCCR0B = 0x00;
  PORTD = 0x00;
}

/*
   This routine will cause delay/keep an eye on Time out time.
*/
ISR(TIMER2_OVF_vect) {
  ++count_delay;
  if (on_off_time_set == 1) {
    ++count_on_off;
  }
  if (count_delay >= thresh_value_delay) {
    count_delay = 0;
    flag_delay = 0;
  }
  if (count_on_off > thresh_value_on_off) {
    on_off_time_set = 0;
    count_on_off = 0;
    time_ = 0;
    change_time_ = 1;
  }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~
       I2C Functions
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*
   Function to initiallize I2C Communication.
*/
void i2c_init() {
  TWSR = 0;
  TWBR = 0x48;
  TWCR = (1 << TWEN) | (1 << TWEA) | (1 << TWINT);
}

/*
   Funtion to send START
*/
void i2c_start() {
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
  while ((TWCR & (1 << TWINT)) == 0);
}

/*
   Function to send STOP
*/
void i2c_stop() {
  TWCR = (1 << TWSTO) | (1 << TWEN) | (1 << TWINT);
}

/*
   Function to write data to SDA
*/
void i2c_write(unsigned char data) {
  TWDR = data;
  TWCR = (1 << TWINT) | (1 << TWEN);
  while ((TWCR & (1 << TWINT)) == 0);
}

/*
   Function to read data from SDA
*/
uint8_t i2c_read(unsigned char isLast) {
  if (isLast == 0) {                                              // If want to get more than one byte, send ACK and continue to recieve next data
    TWCR = (1 << TWEN) | (1 << TWEA) | (1 << TWINT);
  }
  else {
    TWCR = (1 << TWEN) | (1 << TWINT);                           // If want to get only one byte, send NACK and stop recieving the data
  }
  while ((TWCR & (1 << TWINT)) == 0);
  return TWDR;
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~
      TRANSMITTER FUNCTIONS
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*
  Function to send Start pulse.
  Pulse is 9ms long burst of 38kHz IR light.
*/
void start_pulse() {
  OCR1A = 18000;
  TCCR1B = (1 << CS11);
  TCCR0B = (1 << CS00);
  TCNT1 = 0;
  TCNT0 = 0;
  PORTD = 0b00000100;
  while (TCCR1B & (1 << CS11));
}

/*
   Function to cause delay.
   Pulse is retained low for 4.5ms.
*/
void start_delay() {
  PORTD = 0x00;
  OCR1A = 9000;
  TCCR1B = (1 << CS11);
  TCNT1 = 0;
  while (TCCR1B & (1 << CS11));
}

/*
   Function to send 562.5us burst of IR signal showing start of a bit.
*/
void code_pulse() {
  OCR1A = 9000;
  TCCR1B = (1 << CS10);
  TCCR0B = (1 << CS00);
  TCNT1 = 0;
  TCNT0 = 0;
  PORTD = 0b00000100;
  while (TCCR1B & (1 << CS10));
}

/*
   Function to retain signal line low as per the time passed in time_tic.
   If bit is 1, it is retained low for 1687.5us
   If bit is 0, it is retained low for 562.5us
*/
void delay_bit(int time_tic) {
  PORTD = 0x00;
  OCR1A = time_tic;
  TCCR1B = (1 << CS10);
  TCNT1 = 0;
  while (TCCR1B & (1 << CS10));
}

/*
  Funtion to cause a delay of 245ms in the program.
*/
void delay_time() {
  PORTD = 0x00;
  flag_delay = 1;
  while (flag_delay == 1);
}

/*
   Function to send code according to the value passed.
*/
void send_code(int code_value) {
  if (code_value == 1) {
    code = code_ON;
  }
  else if (code_value == 0) {
    code = code_OFF;
  }
  else if (code_value == 2) {
    code = code_INC_BRIGHT;
  }
  else if (code_value == 3) {
    code = code_DEC_BRIGHT;
  }
  start_pulse();
  start_delay();
  for (int i = 0; i < 32; ++i) {
    code_pulse();
    if (code & 0x80000000) {
      delay_bit(27000);
    }
    else {
      delay_bit(9000);
    }
    code <<= 1;
  }
  code_pulse();
  code = 0;
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~
      OTHER FUNCTIONS
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

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

/*
   Function to get Accelerometer values
*/
void get_acclerometer_values() {
  i2c_start();
  i2c_write(((MPU_addr << 1) | TW_WRITE));
  i2c_write(0x3B);                                          // starting with register 0x3B (ACCEL_XOUT_H)
  i2c_start();
  i2c_write(((MPU_addr << 1) | TW_READ));
  AcX = (i2c_read(0) << 8) | i2c_read(0);                   // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  AcY = (i2c_read(0) << 8) | i2c_read(1);                   // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  i2c_stop();
  /*
     The value returned lies between 0-65,536 (since 2^16=65536 hence the max value)
     Converting the value into 0-65 whole number by dividing it with 1000.
  */
  X = AcX / 1000;
  Y = AcY / 1000;
  /*
     The value now lies in 0-65.
     Converting it too scale -32 to +32.
  */
  if (X > 32) {
    X = X - 66;
  }
  if (Y > 32) {
    Y = Y - 66;
  }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~
      MAIN FUNCTION
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
int main() {
  TCCR0A = (1 << WGM01);                                     //Timer0 CTC mode
  TCCR1A = (1 << WGM12);                                     //Timer1 CTC mode

  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);          //Timer2 Normal mode, 1024 Prescaler
  TCNT2 = 0;
  //Enable interrupts on compare match vect A for Timer 1 and 0
  TIMSK0 = (1 << OCIE0A);
  TIMSK1 = (1 << OCIE1A);
  //Enable timer overflow interrupt for timer 2
  TIMSK2 = (1 << TOIE2);
  OCR0A = 210;
  sei();                                                    //Enable global interrupts.

  DDRD = 0b00000100;                                        //IR Led is connceted to PIN 2(PORT D).
  PORTD = 0x00;

  Serial.begin(9600);                                       //Begin USART communication.

  i2c_init();                                               //Begin I2C communication
  i2c_start();
  Serial.println("INITIALLISNG.....");
  i2c_write(((MPU_addr << 1) | TW_WRITE));                  //Send slave address.
  i2c_write(0x6B);                                          // PWR_MGMT_1 register
  i2c_write(0x00);                                          // set to zero (wakes up the MPU-6050)
  if (TW_STATUS == 0x28) {
    Serial.println("Sensor Ready.");
  }
  else {
    i2c_stop();
    Serial.println("ERROR: Failed to establish connection with the SENSOR.");
    Serial.println("Please re-run the program");
    return 1;
  }
  i2c_stop();

  Serial.println("");
  Serial.println("REMOTE INACTIVE");
  Serial.println(" ");
  ask_on_off_time();
  print_set_time();

  while (1) {
    if (change_time_ == 1) {                                //Whenever the Timeout Time will expire, the program will enter here
      change_time_ = 0;
      Serial.println("");
      Serial.println("REMOTE INACTIVE");
      Serial.println("");
      Serial.println("You time has expired..");
      ask_on_off_time();
      print_set_time();
    }

    get_acclerometer_values();                                //Get the sensor values

    if (Hist_Ax == 0 || Hist_Ay == 0) {       //If code is run for the first time/restarted, get calibrated value.
      Hist_Ax = X;
      Hist_Ay = Y;
      Cal_Ax = X;
      Cal_Ay = Y;
    }
    else {                                                                //Detect if gesture is changed and then execute the corresponding code.

      if (on_off_time_set == 1) {                                        //If Timeout Time has been setted then program will go here.

        if (led_stat == 1) {                                              //If LED is on the nbrightness will be increased/decreased.
          if (X > Cal_Ax) {
            if ((abs(X) - abs(Hist_Ax)) > 1) {
              Serial.println("Brightness Decreased");
              send_code(3);
            }
          }
          else if (X < Cal_Ax) {
            if ((abs(X) - abs(Hist_Ax)) > 1) {
              Serial.println("Brightness Increased");
              send_code(2);
            }
          }
        }

        if (Y > Cal_Ay) {                                                   //Detect change for LED
          if ((abs(Y) - abs(Hist_Ay)) > 2) {
            Serial.println("LED On");
            led_stat = 1;
            send_code(1);
          }
        }
        else if (Y < Cal_Ay) {
          if ((abs(Y) - abs(Hist_Ay)) > 2) {
            Serial.println("LED Off");
            led_stat = 2;
            send_code(0);
          }
        }
      }

      else {                                                                // Program will come here if Timeout has not been setted or it expires.
        if (X < Cal_Ax) {
          if ( (abs(X) - abs(Hist_Ax)) > 1) {
            ++time_;
            if (time_ > 10) {
              time_ = 10;
            }
            print_set_time();
          }
        }
        else if (X > Cal_Ax) {
          if ( (abs(X) - abs(Hist_Ax)) > 1) {
            --time_;
            if (time_ < 0) {
              time_ = 0;
            }
            print_set_time();

          }
        }

        if (Y > Cal_Ay) {
          if ((abs(Y) - abs(Hist_Ay)) > 2) {
            Serial.print("    Timer Setted at ");
            Serial.print(time_);
            Serial.println(" min(s)");
            Serial.print("You can now control the light for the next ");
            Serial.print(time_);
            Serial.println(" min(s)");
            Serial.println("");
            Serial.println("REMOTE ACTIVE");
            on_off_time_set = 1;
            thresh_value_on_off *= time_;                                                     //Update the threshold timer variable.
          }
        }
        else if (Y < Cal_Ay) {
          if ((abs(Y) - abs(Hist_Ay)) > 2) {
            Serial.println("   Timer Resetted");
            time_ = 0;
            print_set_time();
          }
        }

      }
      Hist_Ax = X;                                                  //Update the history variables.
      Hist_Ay = Y;
    }
    delay_time();
  }
  return 0;
}

