/*
   Program to play simple TIC-TAC-TOE game using 9 LEDs and 3 push buttons.
   
   ``````````````````````````
          INSTRUCTIONS
   `````````````````````````` 
   There are three push buttons.
   Extreme left and centre buttons are used to traverse through the matrix left and right 
   respectively. Extreme right is used to first enable the selection and then fixing the LEd.
   By default game starts with X's turn followed by O's then again X's and so on. LED will be 
   turned on continously for X, blink with 24 milli sec delay where as the cursor LED blinks
   with 1600 milli sec.

   If 'X' wins X will be formed using LEds, if 'O' wins O will be formed using LED.



   ``````````````````````````
          LED MATRIX
   ``````````````````````````

       o       o       o                                                         o      o      o                                            
     [0,0]   [0,1]   [0,2]                                                     [C5]   [D7]   [D6]

       o       o       o                                                         o      o      o                                             
     [1,0]   [1,1]   [1,2]                     Connected to =>                 [C4]   [C3]   [D5]    

       o       o       o                                                         o      o     o                                             
     [2,0]   [2,1]   [2,2]                                                     [C2]   [C1]   [C0]
      
   
  */
 
#include<avr/io.h>
#include<avr/interrupt.h>

char LED_port[3][3]={ 'C','D','D',                    
                      'C','C','D',
                      'C','C','C'};                    //Variable storing which LED is connected to which PORT
                      
int LED_port_pin[3][3]={ 5,7,6,                      
                         4,3,5,
                         2,1,0 };                      //Variable storing which LED is connected to which PIN number.
                       
char LED_owner[3][3]={ 'z','z','z',
                       'z','z','z',
                       'z','z','z'};                   //Variable storing which LED is occupied by either X or O.
                                                       // If value is 'z' then not occupied, if 'X' then occupied by X
                                                       // if 'O' then occupied by O.
                                                       
int i=-1;                                              //Variable(s) to traverse through the matrix
int j=-1;                                              // Intiallised with -1 to point to null.

int lock=0;                                            //Variable to enable lock. If value is 0, center and extreme left button wont work.
                                                       // If value is 1 then those two buttons will work.
                                                       
int turn=1;                                            //Variable to monitor the turn for X and O.
                                                       //If value stored is odd, then X's turn, if even the O's turn.
                                                       
int count_0;                                           // Variable to use timmer0 t cause 1600 milli sec delay.
                                                       //Since with 1024 prescaler, max delay is 16 milli sec, so if this is incremented whenever timer overflows
                                                       //and this counts upto 100 then total delay will be 16*100=1600 milli secs.
                                                       
//Function to initiallise Timer0 and Timer1 with global interrupts.
//Timer 1 is to delay for 0 LED
//Timer 0 is to delay for cursor LED.
 void timer()
  {
     TCCR0B|=(CS02<<1)|(CS00<<1);   //1024 Prescaling, timer0
     TCCR1B|=(CS12<<1);             //256 Prescaling, timer1
     TCNT0=6;                       //Setting timer counter 0
     TCNT1=64036;                   //Setting timer counter 1
     TIMSK1 = (1 << TOIE1);         //Enabling timer interrupts 1
     TIMSK0 = (1 << TOIE0);         //Enabling timer interrupts 1
     sei();                         //Global interrupts 
  }

//This interrupts is called when Timer 1 overflows.
 ISR (TIMER1_OVF_vect) 
  {
      int m,n;
      for(m=0;m<3;m++)                                                            //Traversing through the LED_owner matrix.
               {                                                                   
                  for(n=0;n<3;n++)
                   {
                    if(LED_owner[m][n]=='O')                                      //For elements havin O as value,
                       {
                        char p=LED_port[m][n];                                    //Its PORT is extracted,
                        int p_pin=LED_port_pin[m][n];                             //Its PIN_Number is extracted.

                        //Corresponding pin is toggoled.
                        if(p=='C')
                          {
                            PORTC^=(1<<p_pin);
                          }
                        else if(p=='D')
                          {
                            PORTD^=(1<<p_pin);
                          }
                       }
                   }
               }
     TCNT1=64036;                                                              //Counter is resetted.
  }

//This interrupts is called when Timer 0 overflows.
  ISR (TIMER0_OVF_vect)    
  { 
            ++count_0;                                                   //Count_0 is incremented with each overflow of timer flag.
              if(count_0>=100)                                           //IF count_0==100, i.e delay occuder is 1600ms.
                {
                  count_0=0;
                  char p=LED_port[i][j];                               //Extract the PORT of current cursor LED.
                  int p_pin=LED_port_pin[i][j];                        //Extract its PIN_Number.

                  //Toggle it.
                  if(p=='C')
                    {
                      PORTC^=(1<<p_pin);
                    }
                 else if(p=='D')
                    {
                       PORTD^=(1<<p_pin);
                    }
                }
     TCNT0=6;                                                         //Counter is resetted.
  }

//Function to check if someone wins or not.
void check_winner()
 {
   int win=0;                                                    // Variable to flag the winner. If equals 1 then X wins, if 2 then O wins.
   char check_char='X';                                          //Variable to store the character which is to be compared.
   int row=0,col=0;
   int flag=0;
   int pass_1=1;                                                // Flag variable to switch check_char to 'O'.
   z:

   //Checking in rows.
   for(row=0;row<3;row++)
    {
      col=0;
      flag=0;
      while(col<3)
       {
          if(LED_owner[row][col]==check_char)
             ++flag;
         col++;
       }
      if(flag==3)
       {
         if(check_char=='X')
            win=1;
         else
            win=2;
         break;
       }
    }

   //Checking in columns.
   flag=0;
   for(col=0;col<3;col++)
    {
      row=0;
      flag=0;
      while(row<3)
       {
          if(LED_owner[row][col]==check_char)
             ++flag;
         row++;
       }
      if(flag==3)
       {
         if(check_char=='X')
            win=1;
         else
            win=2;
         break;
       }
    }

    //Checking in principal diagonal.
    flag=0;
    for(row=0;row<3;row++)
       {
         for(col=0;col<3;col++)
           {
            if(row==col)
              {
                if(LED_owner[row][col]==check_char)
                 {
                  ++flag;
                 }
              }
           }
       }
    if(flag==3)
       {
         if(check_char=='X')
            win=1;
         else
            win=2;
       }

    // Checking in secondary diagonal.
    flag=0;
    for(row=0;row<3;row++)
       {
         for(col=0;col<3;col++)
           {
            if(row+col==2)
              {
                if(LED_owner[row][col]==check_char)
                 {
                  ++flag;
                 }
              }
           }
       }
    if(flag==3)
       {
         if(check_char=='X')
            win=1;
         else
            win=2;
       }
   flag=0;
    if(pass_1==1 && win==0)                      //If win is 0 and pass_1 is 1 then check for 'O'.
      {
        check_char='O';
        pass_1=0;
        goto z;
      }

   //If X wins.
   if(win==1)
    {                                                        //Glow LED in X manner.
      
      for(row=0;row<3;row++)                                //            o     .     o               
       {
         for(col=0;col<3;col++)                             //            .     o     .                   o => On , . => Off
           {
            LED_owner[row][col]='z';                        //            o     .     o
           }
        }
      PORTC=0b00101101;
      PORTD=0b01000000;
    }

    //If 0 wins
    if(win==2)                                            //Glow LED in O manner.
    {
      for(row=0;row<3;row++)                              //            o     o     o
        {
         for(col=0;col<3;col++)                           //            o     .     o                   o => On , . => Off
           {
            LED_owner[row][col]='z';                      //            o     o     o
           }
        }
      PORTC=0b00110111;
      PORTD=0b11100000;
    }
 }


 
 int main()
  {
     DDRD=0xFF;                    //PORTD for output.
     DDRC=0xFF;                    //PORTC for output
     DDRB=0b11111000;              //PORTB for partial output and partial input(push buttons).
     PORTB=0b11110111;             //Enabling pull-ups.
     PORTD=0x00;                   //Output 0 to PORTD => LED turnoff.
     PORTC=0x00;                   //Output 0 to PORTC => LED turnoff.
     int m,n;
     timer();                      // Initiallize timer(s).
     while(1)
        {
          
          if((bit_is_set(PINB,0))==0)             // Extreme right button is pressed(used to start selection/fix selected LED)
            {
              if(lock==0)                         //If lock is 0 then selection should start, fixing should not be done.
                 {
                  i=0;j=0;                        // Select the first LED at [0,0]
                  lock=1;                         // Change the mode to fixing mode.
                  while(LED_owner[i][j]!='z')     //Loop to check whether selected LED is already fixed or not
                    {
                      ++j;                        //if fixed i.e not equal to 'z' then increment the index value.
                      if(j>2)
                        {
                          j=0;
                          ++i;
                        }
                      if(i>2)
                        {
                          i=0;
                          j=0;
                        }
                    }
                 }
              else                                          // If selection is laready done, fix the LED.
                 {
                   if(turn%2==0)
                      LED_owner[i][j]='O';
                   else
                      LED_owner[i][j]='X';
                   i=-1;                                    // Set back the garbage address i.e -1.
                   j=-1;
                   turn++;                                  //Change the turn.
                   lock=0;                                  // Change back mode to selection.
                 }
              while((bit_is_set(PINB,0))==0);
            }
            
          if((bit_is_set(PINB,1))==0)                      // If middle button is pressed(used to move cursor Right).
            {
             if(lock==1)                                  // IF slection mode is on
                {
                  char p=LED_port[i][j];                  //First take the current cursor position.
                  int p_pin=LED_port_pin[i][j];
                  if(p=='C')                              //And turn it off.
                    {
                      PORTC&=~(1<<p_pin);
                    }
                 else if(p=='D')
                    {
                       PORTD&=~(1<<p_pin);
                    }
                  l:
                  ++j;                                    // And then increment the index to next value
                  if(j>2)
                     {
                      j=0;
                      ++i;
                     }
                  if(i>2)
                    {
                      i=0;
                      j=0;
                    }
                  if(LED_owner[i][j]!='z')               // To check if the cursor position is already fixed or not
                    {
                      goto l;
                    }
                }
             while((bit_is_set(PINB,1))==0);
            }
            
          if((bit_is_set(PINB,2))==0)                  // If extreme left button is pressed (used to move cursor left)
            {
              if(lock==1)                                 // If selection mode is on
                {
                  char p=LED_port[i][j];
                  int p_pin=LED_port_pin[i][j];                // Turn the current LED off where cursor is now pointing to.
                  if(p=='C')
                    {
                      PORTC&=~(1<<p_pin);
                    }
                 else if(p=='D')
                    {
                       PORTD&=~(1<<p_pin);
                    }
                  k:
                  --j;                                         // Decrement the index value and hence cursor is moved to previous non-occupied poisition.
                  if(j<0)
                     {
                      j=2;
                      --i;
                     }
                  if(i<0)
                    {
                      i=2;
                      j=2;
                    }
                  if(LED_owner[i][j]!='z')                     // To check whether current cursor position is occupied or not.
                    {
                      goto k;
                    }
                }
              while((bit_is_set(PINB,2))==0);
            }
         
         for(m=0;m<3;m++)                                                 // Loop to turn on LED who is selected by player 'X'.
             {
              for(n=0;n<3;n++)
                   {
                    if(LED_owner[m][n]=='X')
                       {
                        char p=LED_port[m][n];
                        int p_pin=LED_port_pin[m][n];
                        if(p=='C')
                          {
                            PORTC|=(1<<p_pin);
                          }
                        else if(p=='D')
                          {
                            PORTD|=(1<<p_pin);
                          }
                       }
                   }
               }

         check_winner();                                                // Function to check if someone has won the game or not.
        }
    return 0;
  }

