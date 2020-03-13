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
#include<stdio.h>
#include "uart0.h"
#include <stdbool.h>
#include <string.h>
#include<ctype.h>
#include <math.h>
#include "tm4c123gh6pm.h"
#include<stdbool.h>
#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define GREEN_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))
#define BLUE_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))
#define LDAC_NOT        (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 6*4)))
#define CS_NOT       (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 5*4)))
#define AIN1_MASK 8
#define AIN0_MASK 4
#define MAX_CHAR 80
#define MAX_FIELDS 10
#define RED_LED_MASK 2
#define GREEN_LED_MASK 8
#define BLUE_LED_MASK 4
#define LDAC_MASK 64
#define CS_NOT_MASK 32
#define TIMERON TIMER1_CTL_R |= TIMER_CTL_TAEN;
#define delay4Cycles() __asm(" NOP\n NOP\n NOP\n NOP")
#define DAC_Aint 12288
#define DAC_Bint 45056
   uint8_t argCount =0;
     char notation;
     uint8_t e,f,q;
     char trial[100];
char str[MAX_CHAR+1];
uint8_t x =0;
uint8_t y =0;
uint8_t pos[MAX_FIELDS];
uint8_t argCount;
uint8_t count =0;
uint8_t *v;
float *j;

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

    // Enable GPIO port F D and A peripherals
    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOF | SYSCTL_RCGC2_GPIOD |  SYSCTL_RCGC2_GPIOA | SYSCTL_RCGC2_GPIOB |SYSCTL_RCGC2_GPIOE ;

    // Configure LED pin
//


    GPIO_PORTF_DIR_R = RED_LED_MASK | GREEN_LED_MASK |BLUE_LED_MASK;  // make bit an output
    GPIO_PORTF_DR2R_R = RED_LED_MASK | GREEN_LED_MASK |BLUE_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R = RED_LED_MASK | GREEN_LED_MASK |BLUE_LED_MASK;  // enable LED

    GPIO_PORTA_DIR_R = LDAC_MASK;  // make bit an output
       GPIO_PORTA_DR2R_R = LDAC_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
       GPIO_PORTA_DEN_R = LDAC_MASK;  // enable for digital op.
}
void inttimer1A()
{
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;
    TIMER1_TAILR_R = 400;
    TIMER1_IMR_R = TIMER_IMR_TATOIM;
    NVIC_EN0_R |= 1<< (INT_TIMER1A-16);
   // TIMER1_CTL_R |= TIMER_CTL_TAEN;
   // TIMERON;
}

void inithwadc()
{
 //   SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);
    SYSCTL_GPIOHBCTL_R = 0;
    SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0;   //ADC0 ACTIVATED
   // SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOE;   //PORTE ANALOG PINS

    //Configure ain1,0 analog input
    GPIO_PORTE_AFSEL_R |= AIN0_MASK | AIN1_MASK;    //PE2(AIN1),PE3(AIN0)
    GPIO_PORTE_DEN_R &= ~(AIN0_MASK | AIN1_MASK);
    GPIO_PORTE_AMSEL_R |= AIN0_MASK | AIN1_MASK;

    //Configure ADC
        ADC0_CC_R = ADC_CC_CS_SYSPLL;
        ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN3;
        ADC0_EMUX_R = ADC_EMUX_EM3_PROCESSOR;

        ADC0_SSMUX3_R = 1;
        ADC0_SSCTL3_R = ADC_SSCTL3_END0;
        ADC0_ACTSS_R |= ADC_ACTSS_ASEN3;

    }
void inithwssi()
{
    //SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);
    SYSCTL_GPIOHBCTL_R = 0;
    SYSCTL_RCGCSSI_R = SYSCTL_RCGCSSI_R2;            // turn-on SSI2 clocking

                                                      // turn on GPIO ports B
    GPIO_PORTB_DIR_R |= CS_NOT_MASK;       // make bits 5 outputs
    GPIO_PORTB_DR2R_R |= CS_NOT_MASK;      // set drive strength to 2mA
    GPIO_PORTB_DEN_R |= CS_NOT_MASK;       // enable bits 5 for digital

    GPIO_PORTB_DIR_R |= 0x90;                        // make bits 4 and 7 outputs
       GPIO_PORTB_DR2R_R |= 0x90;                       // set drive strength to 2mA
       GPIO_PORTB_AFSEL_R |= 0x90;                      // select alternative functions for MOSI, SCLK pins
       GPIO_PORTB_PCTL_R = GPIO_PCTL_PB7_SSI2TX | GPIO_PCTL_PB4_SSI2CLK; // map alt fns to SSI2
       GPIO_PORTB_DEN_R |= 0x90;                        // enable digital operation on TX, CLK pins
       GPIO_PORTB_PUR_R |= 0x10;                        // must be enabled when SPO=1

       // Configure the SSI2 as a SPI master, mode 3, 8bit operation, 1 MHz bit rate
       SSI2_CR1_R &= ~SSI_CR1_SSE;                      // turn off SSI2 to allow re-configuration
       SSI2_CR1_R = 0;                                  // select master mode
       SSI2_CC_R = 0;                                   // select system clock as the clock source
       SSI2_CPSR_R = 10;                                // set bit rate to 4 MHz (if SR=0 in CR0)
       SSI2_CR0_R = (0x00000000) | (0x00000000) | SSI_CR0_FRF_MOTO | SSI_CR0_DSS_16; // set SR=0, mode 3 (SPH=1, SPO=1), 16-bit
       SSI2_CR1_R |= SSI_CR1_SSE;                       // turn on SSI2
    }

int16_t readAdc0Ss3()
{
    ADC0_PSSI_R |= ADC_PSSI_SS3;
    while(ADC0_ACTSS_R & ADC_ACTSS_BUSY);
    return ADC0_SSFIFO3_R;

}
uint16_t setdc()
{
    uint16_t out;
     float volt ;
     out = *v;
     volt = *j;
    float Vout =0;
    uint16_t D =0;    //volt = volt + 8.1936 ;
    uint16_t DAC_valueA,DAC_valueB;//,DAC_value1 =0 ;
    if(out==1)
           {
              // putsUart0("OUTA");
               if(volt > 0.5 && volt <1.0)
               {

                Vout = (((volt*2.048)/-10.0) + 1.024) ;
                D = (( Vout * 4096 ) / 2.048)  ;
               D = D-32;
               DAC_valueA =  DAC_Aint  + D;
       /*        CS_NOT = 0;                        // assert chip select
                          __asm (" NOP");                    // allow line to settle
                          __asm (" NOP");
                          __asm (" NOP");
                      //    __asm (" NOP");
                       SSI2_DR_R = DAC_valueA ;
                       while (SSI2_SR_R & SSI_SR_BSY);
                     CS_NOT = 1;*/
               }
               else if(volt > 0.0 && volt <= 0.5)
               {

                Vout = (((volt*2.048)/-10.0) + 1.024) ;
                D = (( Vout * 4096 ) / 2.048)  ;
               D = D-43;
               DAC_valueA =  DAC_Aint  + D;

               }
               else if(volt >= 1.0 && volt < 2.0)
               {

                Vout = (((volt*2.048)/-10.0) + 1.024) ;
                D = (( Vout * 4096 ) / 2.048)  ;
               D = D-18;
               DAC_valueA =  DAC_Aint  + D;
               //p = &DAC_valueA;
               }
               else if(volt == 2.0)
                           {

                            Vout = (((volt*2.048)/-10.0) + 1.024) ;
                            D = (( Vout * 4096 ) / 2.048)  ;
                           D = D+1;
                           DAC_valueA =  DAC_Aint + D;
                           }
               else if(volt >2.0 && volt < 3.0)
                   {

                    Vout = (((volt*2.048)/-10.0) + 1.024) ;
                    D = (( Vout * 4096 ) / 2.048)  ;
                   D = D+5;
                   DAC_valueA =  DAC_Aint+ D;
                   }
               else if(volt >= 3.0)
               {
                Vout = (((volt *2.048)/-10.00) + 1.024) ;
                D = (( Vout * 4096 ) / 2.048)  ;
               D = D+20;
               DAC_valueA =  DAC_Aint + D;
               }
               else if(volt < 0.0 && volt >= -0.5)
               {
               Vout = (((volt *2.048)/-10.00) + 1.024) ;
               D = (( Vout * 4096 ) / 2.048)  ;
              D = D-53;
              DAC_valueA =  DAC_Aint+ D;
               }
               else if(volt < -0.5 && volt > -1.0)
                      {
                      Vout = (((volt *2.048)/-10.00) + 1.024) ;
                      D = (( Vout * 4096 ) / 2.048)  ;
                     D = D-64;
                     DAC_valueA =  DAC_Aint+ D;
                      }
               else if(volt < -1.0 && volt > -2.0)
                      {
                      Vout = (((volt *2.048)/-10.00) + 1.024) ;
                      D = (( Vout * 4096 ) / 2.048)  ;
                     D = D-72;
                     DAC_valueA =  DAC_Aint+ D;
                      }
               else if(volt == -1.0)
                      {
                      Vout = (((volt *2.048)/-10.00) + 1.024) ;
                      D = (( Vout * 4096 ) / 2.048)  ;
                     D = D-68;
                     DAC_valueA =  DAC_Aint+ D;
                      }
               else if(volt == -2.0)
                      {
                      Vout = (((volt *2.048)/-10.00) + 1.024) ;
                      D = (( Vout * 4096 ) / 2.048)  ;
                     D = D-94;
                     DAC_valueA =  DAC_Aint+ D;
                      }
               else if(volt <-2.0 && volt > -3.0)
                 {
                 Vout = (((volt *2.048)/-10.00) + 1.024) ;
                 D = (( Vout * 4096 ) / 2.048)  ;
                D = D-108;
                DAC_valueA =  DAC_Aint+ D;
                 }
               else if( volt <= -3.0)
               {
                Vout = (((volt *2.048)/-10.00) + 1.024) ;
                D = (( Vout * 4096 ) / 2.048)  ;
               D = D-118;
               DAC_valueA =  DAC_Aint+ D;
               }
               else if(volt == 0.00)
               {
               Vout = (((volt *2.048)/-10.00) + 1.024) ;
               D = (( Vout * 4096 ) / 2.048)  ;
               D = D-55;
              DAC_valueA =  DAC_Aint+ D;
               }


                   LDAC_NOT =0;
            __asm (" NOP");                    // allow line to settle
             __asm (" NOP");
             __asm (" NOP");
             //__asm (" NOP");
             LDAC_NOT =1;

           }
       //********************** DACOUTB ****************************************************************************
           if(out==2)
           {
             //  putsUart0("OUTB");
               if(volt > 0.5 && volt <1.0)
               {

                Vout = (((volt*2.048)/-10.0) + 1.024) ;
                D = (( Vout * 4096 ) / 2.048)  ;
               D = D-32;
               DAC_valueB =  DAC_Bint  + D;
               }
               else if(volt > 0.0 && volt <= 0.5)
               {

                Vout = (((volt*2.048)/-10.0) + 1.024) ;
                D = (( Vout * 4096 ) / 2.048)  ;
               D = D-43;
               DAC_valueB =  DAC_Bint  + D;
               }
               else if(volt >= 1.0 && volt < 2.0)
               {

                Vout = (((volt*2.048)/-10.0) + 1.024) ;
                D = (( Vout * 4096 ) / 2.048)  ;
               D = D-18;
               DAC_valueB =  DAC_Bint  + D;
               }
               else if(volt == 2.0)
                           {

                            Vout = (((volt*2.048)/-10.0) + 1.024) ;
                            D = (( Vout * 4096 ) / 2.048)  ;
                           D = D+1;
                           DAC_valueB =  DAC_Bint + D;
                           }
               else if(volt >2.0 && volt < 3.0)
                   {

                    Vout = (((volt*2.048)/-10.0) + 1.024) ;
                    D = (( Vout * 4096 ) / 2.048)  ;
                   D = D+5;
                   DAC_valueB =  DAC_Bint+ D;
                   }
               else if(volt >= 3.0)
               {
                Vout = (((volt *2.048)/-10.00) + 1.024) ;
                D = (( Vout * 4096 ) / 2.048)  ;
               D = D+20;
               DAC_valueB =  DAC_Bint+ D;
               }
               else if(volt < 0.0 && volt >= -0.5)
               {
               Vout = (((volt *2.048)/-10.00) + 1.024) ;
               D = (( Vout * 4096 ) / 2.048)  ;
              D = D-53;
              DAC_valueB =  DAC_Bint+ D;
               }
               else if(volt < -0.5 && volt > -1.0)
                      {
                      Vout = (((volt *2.048)/-10.00) + 1.024) ;
                      D = (( Vout * 4096 ) / 2.048)  ;
                     D = D-64;
                     DAC_valueB =  DAC_Bint+ D;
                      }
               else if(volt < -1.0 && volt > -2.0)
                      {
                      Vout = (((volt *2.048)/-10.00) + 1.024) ;
                      D = (( Vout * 4096 ) / 2.048)  ;
                     D = D-72;
                     DAC_valueB =  DAC_Bint+ D;
                      }
               else if(volt == -1.0)
                      {
                      Vout = (((volt *2.048)/-10.00) + 1.024) ;
                      D = (( Vout * 4096 ) / 2.048)  ;
                     D = D-68;
                     DAC_valueB =  DAC_Bint+ D;
                      }
               else if(volt == -2.0)
                      {
                      Vout = (((volt *2.048)/-10.00) + 1.024) ;
                      D = (( Vout * 4096 ) / 2.048)  ;
                     D = D-94;
                     DAC_valueB =  DAC_Bint+ D;
                      }
               else if(volt <-2.0 && volt > -3.0)
                 {
                 Vout = (((volt *2.048)/-10.00) + 1.024) ;
                 D = (( Vout * 4096 ) / 2.048)  ;
                D = D-108;
                DAC_valueB =  DAC_Bint+ D;
                 }
               else if( volt <= -3.0)
               {
                Vout = (((volt *2.048)/-10.00) + 1.024) ;
                D = (( Vout * 4096 ) / 2.048)  ;
               D = D-118;
               DAC_valueB =  DAC_Bint+ D;
               }
               else if(volt == 0.00)
               {
               Vout = (((volt *2.048)/-10.00) + 1.024) ;
               D = (( Vout * 4096 ) / 2.048)  ;
               D = D-55;
              DAC_valueB =  DAC_Bint+ D;


               }
            //   return &DAC_valueB;
       /*             LDAC_NOT =0;
            __asm (" NOP");                    // allow line to settle
             __asm (" NOP");
             __asm (" NOP");
             //__asm (" NOP");
             LDAC_NOT =1;*/

           }
           CS_NOT =0;
            SSI2_DR_R = DAC_valueB;
             while (SSI2_SR_R & SSI_SR_BSY);
             CS_NOT = 1;
             LDAC_NOT =0;
         __asm (" NOP");                    // allow line to settle
         __asm (" NOP");
         __asm (" NOP");
         __asm (" NOP");
         LDAC_NOT =1;
    LDAC_NOT = 1;

//************************************OUTA************************************************************************


  //  TIMERON;
return 0;
    }
void Timer1AISR()
{

    setdc();

    TIMER1_ICR_R = TIMER_ICR_TATOCINT;
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
           str[count]=tolower(c);
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

               str[count++] = tolower(c);

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
                ((0x30 <= (str[e] )) && ((str[e])  <= 0x39))|| (0x2E== (str[e])) || (0x2C == (str[e])) || (0x2D== (str[e])))
        {
             x =0;

        }

        else
        {

            y =0;

        }

        if( e != 0  && x ==0 )
        {
            e--;
            if(((0x41 <= (str[e]))&& ((str[e])<=0x5A)) || ((0x61 <= (str[e])) && ((str[e]) <= 0x7A))||
                    ((0x30 <= (str[e] )) && ((str[e])  <= 0x39))|| (0x2E== (str[e])) || (0x2C == (str[e])) || (0x2D== (str[e])))
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
        else if(e ==0 && x == 0  )
            {
            pos[f] = e;
            f++;
            argCount++;
            }
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
    inithwadc();
    initUart0();
    inithwssi();
    inttimer1A();

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
                 valid = true;
           }
           if(is_command("unlock",0))
           {
               GREEN_ON();
              // LDAC_NOT =1;
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
                valid = true;
           }
 // ***********************************STEP 6 (VOLTAGE)***********************************************************

           if(is_command("voltage",1))
           {
               if(strcmp(getString(0),"0")==0)
               {
                   putsUart0("Voltage Channel = 0");
               }
               else if((strcmp(getString(0),"1")==0))
               {
                   uint16_t raw=0 ;
                   float Voltage1 =0;

                    raw = readAdc0Ss3();


                    Voltage1 = ((raw + 0.5 )/4096) * 3.3;
                 //
                    putsUart0("RAW VALUE");

                    sprintf(trial,"%4u",raw);
                    putsUart0("RAW VALUE");
                    putsUart0(trial);
                    sprintf(trial,"Voltage 1:    %f \r\n",Voltage1);
                    putsUart0(trial);

               }

           }
// ***********************************STEP 7 (DC COMMAND )***********************************************************
      if(is_command("dc",2))
      {
      //    uint8_t *p;
       //   float *q;
          uint8_t   out = getArgInt(getString(0));
          float volt = getArgFloat(getString(1));
          v = &out;
          j = &volt;

          putsUart0("DC COMMAND");
         // setdc(out,volt);
          TIMERON;
  /*        LDAC_NOT =0;
          waitMicrosecond(5000);
           LDAC_NOT =1;
*/
      }
     // SSI2_DR_R = 0 ;
           argCount =0;
          /* if(!valid )
           {}*/
           /*if(is_command("input",1))
           {
               putsUart0("Input in Progress...........");
           }*/
        }

 }


