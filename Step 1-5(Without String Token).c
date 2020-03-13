// Timing C/ASM Mix Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red LED:
//   PF1 drives an NPN transistor that powers the red LED

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include<stdlib.h>
#include <stdbool.h>
//#include <string.h>
#include "tm4c123gh6pm.h"
#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define GREEN_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))
#define BLUE_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))

#define MAX_CHAR 80
#define MAX_FIELDS 10
#define RED_LED_MASK 2
#define GREEN_LED_MASK 8
#define BLUE_LED_MASK 4
#define delay4Cycles() __asm(" NOP\n NOP\n NOP\n NOP")
#define ALPHANUM 'a'
#define DELIMETER 'd'
   uint8_t argCount =0;
     char notation;
     uint8_t e,f,q;
char str[MAX_CHAR+1];
uint8_t x =0;
uint8_t y =0;
uint8_t pos[MAX_FIELDS];
uint8_t argCount;
uint8_t count =0;
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHwLED()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, sysdivider of 5, creating system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable GPIO port F peripherals
    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOF | SYSCTL_RCGC2_GPIOD;
 //   SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOD;
    // Configure LED pin
//


    GPIO_PORTF_DIR_R = RED_LED_MASK | GREEN_LED_MASK |BLUE_LED_MASK;  // make bit an output
    GPIO_PORTF_DR2R_R = RED_LED_MASK | GREEN_LED_MASK |BLUE_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R = RED_LED_MASK | GREEN_LED_MASK |BLUE_LED_MASK;  // enable LED

}
void initHwUART()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    // Note UART on port A must use APB
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable GPIO port A and F peripherals
    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOA | SYSCTL_RCGC2_GPIOF;

    // Configure LED pins
    GPIO_PORTF_DIR_R = GREEN_LED_MASK | RED_LED_MASK;  // bits 1 and 3 are outputs
    GPIO_PORTF_DR2R_R = GREEN_LED_MASK | RED_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R = GREEN_LED_MASK | RED_LED_MASK;  // enable LEDs

    // Configure UART0 pins
    GPIO_PORTA_DIR_R |= 2;                           // enable output on UART0 TX pin: default, added for clarity
    GPIO_PORTA_DEN_R |= 3;                           // enable digital on UART0 pins: default, added for clarity
    GPIO_PORTA_AFSEL_R |= 3;                         // use peripheral to drive PA0, PA1: default, added for clarity
    GPIO_PORTA_PCTL_R &= 0xFFFFFF00;                 // set fields for PA0 and PA1 to zero
    GPIO_PORTA_PCTL_R |= GPIO_PCTL_PA1_U0TX | GPIO_PCTL_PA0_U0RX;
                                                     // select UART0 to drive pins PA0 and PA1: default, added for clarity

    // Configure UART0 to 115200 baud, 8N1 format
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0;         // turn-on UART0, leave other UARTs in same status
    delay4Cycles();                                  // wait 4 clock cycles
    UART0_CTL_R = 0;                                 // turn-off UART0 to allow safe programming
    UART0_CC_R = UART_CC_CS_SYSCLK;                  // use system clock (40 MHz)
    UART0_IBRD_R = 21;                               // r = 40 MHz / (Nx115.2kHz), set floor(r)=21, where N=16
    UART0_FBRD_R = 45;                               // round(fract(r)*64)=45
    UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN; // configure for 8N1 w/ 16-level FIFO
    UART0_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
                                                     // enable TX, RX, and module
}

// Approximate busy waiting (in units of microseconds), given a 40 MHz system clock
void waitMicrosecond(uint32_t us)
{
    __asm("WMS_LOOP0:   MOV  R1, #6");          // 1
    __asm("WMS_LOOP1:   SUB  R1, #1");          // 6
    __asm("             CBZ  R1, WMS_DONE1");   // 5+1*3
    __asm("             NOP");                  // 5
    __asm("             NOP");                  // 5
    __asm("             B    WMS_LOOP1");       // 5*2 (speculative, so P=1)
    __asm("WMS_DONE1:   SUB  R0, #1");          // 1
    __asm("             CBZ  R0, WMS_DONE0");   // 1
    __asm("             NOP");                  // 1
    __asm("             B    WMS_LOOP0");       // 1*2 (speculative, so P=1)
    __asm("WMS_DONE0:");                        // ---
                                                // 40 clocks/us + error
}
void putcUart0(char c)
{
    while (UART0_FR_R & UART_FR_TXFF);               // wait if uart0 tx fifo full
    UART0_DR_R = c;                                  // write character to fifo
}

// Blocking function that writes a string when the UART buffer is not full
void putsUart0(char* str)
{
    uint8_t i;
    for (i = 0; i < strlen(str); i++)
      putcUart0(str[i]);
}

// Blocking function that returns with serial data once the buffer is not empty
char getcUart0()
{
    while (UART0_FR_R & UART_FR_RXFE);               // wait if uart0 rx fifo empty
    return UART0_DR_R & 0xFF;                        // get character from fifo
}

/****************************************************************************************************************
***********************************STEP:2 (Support Backspace Ctl + H and Ctl + ? CR LF )***********************************************************
***************************************************************************************************************/
unsigned char getcUart0String(char str[], uint8_t x)
{

//    count ++;
    while(1)
    {
   char c = getcUart0();

   if ((c == 8) || (c == 127))
   {
       if(count >0)
       {
           count--;
           str[count]=c;
       }
       else
       {
           c =0;
           c = getcUart0();
       }
   }

   if ((c == 13) || (c == 10))
            {
               if(count !=0 )
                   {
                       str[count]=0;
                       putsUart0("\r\n");
                       putsUart0("UART0 String:");
                       putsUart0(str);
                       putsUart0("\r\n");

                       count =0;
                       c =0;
                       break;
                   }
            }
     if(c >= 32)
       {

               str[count++] = c;
       }
       else if(count ==MAX_CHAR)
                 {
                     str[count] =0;
                     break ;
                 }

    }

   //putsUart0(str);


   //putsUart0("\r\n");
return 0;
}
//***************************************************************************************************************
//***************************************************************************************************************

/****************************************************************************************************************
***********************************STEP:3 (ParseString)***********************************************************
***************************************************************************************************************/
void parseString()
{

    for(e=0,f=0;str[e]!='\0';e++)
    {
        //Checking if the String is A to Z or a to z
        //Checking if string is 0 to 9 or " . " or " , "
        y=1;
        x=1;
        if(((0x41 <= (str[e]))&& ((str[e])<=0x5A)) || ((0x61 <= (str[e])) && ((str[e]) <= 0x7A))||
                ((0x30 <= (str[e] )) && ((str[e])  <= 0x39))|| (0x2E== (str[e])) || (0x2C == (str[e])))
        {
             x =0;

            //notation = ALPHANUM;
        }

        else
        {

            y =0;
            //notation = DELIMETER;
        }
        //if( e != 0  && notation == ALPHANUM )
        if( e != 0  && x ==0 )
        {
            e--;
            if(((0x41 <= (str[e]))&& ((str[e])<=0x5A)) || ((0x61 <= (str[e])) && ((str[e]) <= 0x7A))||
                    ((0x30 <= (str[e] )) && ((str[e])  <= 0x39))|| (0x2E== (str[e])) || (0x2C == (str[e])))
            {
                e++;
            }
            else
            {
                e++;
                pos[f] = e;
                f++;
                argCount ++;
            }
        }
       // else if(e ==0 && notation == ALPHANUM  )
        else if(e ==0 && x == 0  )
            {
            pos[f] = e;
            f++;
            argCount++;
            }
       // else if(notation == DELIMETER)
        else if(y ==0)
        {
            str[e] = '\0';

        }
        }





}
/****************************************************************************************************************
***********************************STEP:4 (getArgString and Outputs Specific POSITION )***********************************************************
***************************************************************************************************************/
char *getArgString(uint8_t argNo)
{
    if(argNo <= argCount)
    {
        putsUart0(&str[pos[argNo]]);

        return &str[pos[argNo]];
    }
    else
    {
        putsUart0("TRASH FOUND");
    }
 /*   uint32_t getargInt(uint8_t argNo);

    float getargFloat(uint8_t argNo);*/

return &str[pos[argNo]] ;
}
uint32_t getArgInt(char* argString)
{
    uint32_t value;
    value = atoi(argString);
    return value;
}
char* getString(argNo)
{
    return(&str[pos[argNo+1]]);
}
float getArgFloat(char* argString)
{
    float value1;
    value1 = atof(argString);
    return value1;
}
bool is_command(char* strcmd, uint8_t minArg)

{

    q= strcmp((const char*)(&str[pos[0]]) ,strcmd);
           // (const char *( &str[pos[0]]) , strcmd[] );
            if(argCount > minArg  && q==0 )
            {
                return true;
            }
            else
            {
                return false;
            }
    }
//***************************************************************************************************************************

void GREEN_OFF()
{
   GREEN_LED  =0;
}
void GREEN_ON()
{
   GREEN_LED  =1;
}



//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
 {
    // Initialize hardware
    initHwLED();
    initHwUART();
        while(1)
        {

 /****************************************************************************************************************
 ***********************************STEP 1 (500ms Delay)***********************************************************
 ***************************************************************************************************************/
          //  GREEN_ON();
           // waitMicrosecond(500000);
           // GREEN_OFF();
           // waitMicrosecond(500000);
// ****************************************************************************************************************
            getcUart0String(str,MAX_CHAR);
           parseString();
           int i;
           bool valid;
           for(i = 0 ; i <argCount ; i++)
         {
           getArgString(i);
         putsUart0("\r\n");
          }
           if(is_command("set",3))
           {

               uint32_t   p = getArgInt(getString(0));
              uint8_t o = getArgInt(getString(1));
               uint8_t i = getArgInt(getString(2));
               putsUart0("Set function is active...... :)");
                bool valid = true;
           }
           if(is_command("unlock",0))
           {
               GREEN_ON();
           }
           if(is_command("lock",0))
           {
               GREEN_OFF();
           }
// ***********************************STEP 5 (IS COMMAND RESET)***********************************************************
           if(is_command("reset",0))
           {
               putsUart0("RESET in Progress...........");
               NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
               bool valid = true;
           }
           argCount =0;
          /* if(!valid )
           {}*/
           /*if(is_command("input",1))
           {
               putsUart0("Input in Progress...........");
           }*/
        }

 }


