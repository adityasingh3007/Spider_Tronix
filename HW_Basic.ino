/*
 Program to Blink an LED using TIMER. Blinking rate is controlled using a button. 2 buttons are used, where 1 button can be used 
 for increasing the blinking rate and the other button can be used to decrease the blinking rate. And the blinking rate goes in 
 cycle i.e after reaching its max it will again go down to its min and will incease and vice versa.

 ```````````````````
     CONNECTION
 ```````````````````
 LED's +ve terminal is connected to PIN0 of PORTC.
 The two push button's terminals are connected to PIN0 and PIN1 of PORTB.
 
 */
 
#include<avr/io.h>
#include<util/delay.h>

//Function to initiallize TIMER1 of microcontroller.
void timer()
 {
   TCCR1B|=1<<CS12;         //Prescaling to 256
   TCNT1=0;                 //Initiallising the TimerCouNTer.
 }
 int main()
  {
     DDRD=0xFF;            //Setting PORTD as Output.
     DDRB=0x00;            //Setting PORTB as input.
     DDRC=0xFF;            //Setting PORTD as Output.
     PORTD=0x00;           //Output gate closed on PORTD.
     PORTB=0xFF;           //Pull-ups enabled on PORTB
     PORTC=0xFF;           //Output gate opened on PORTC.
     timer();              //Initiallising timer
     int x=0;              //Variable to store the blinking delay rate.
     while(1)
      {
         if((bit_is_set(PINB,0))==0)        //If increasing rate button is pressed
           {
            if(x>=65536)                   //If max rate is reached, set to min rate.
              x=0;
            while((bit_is_set(PINB,0))==0);
           }
        else 
          if((bit_is_set(PINB,1))==0)      //If decreasing rate button is pressed
           {
            x-=1000;
            if(x<=0)                      //If min rate is reached, set to max rate.
              x=65500;
            while((bit_is_set(PINB,1))==0);
           }
         if(TCNT1>=x)
          {
            PORTC^=_BV(PC0);             //Toogle the PIN0 of PORTC, where LED is connected.
            TCNT1=0; 
          }
      }
    return 0;    
  }

