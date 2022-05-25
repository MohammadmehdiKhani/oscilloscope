#include "stm32f4xx.h"
#include <stdio.h>

//***************  Defines	 ***************

#define RS 0x20                         //Pin5 PortB for RS
#define RW 0x40                         //Pin6 PortB for RW
#define EN 0x80                         //Pin7 PortB for E

//***************  Signitures  ***************

void DelayMs(int n);
void LCD_command(unsigned char command);
void LCD_data(char data);
void LCD_init(void);
void LCD_showParameters(void);
void PortsInit(void);
void SendByte(uint8_t data);
uint8_t KeypadColumn(uint16_t odr, uint8_t col);
uint8_t Keypad(void);

//***************  Parameters  ***************

double a = 1;
double b = 0;
uint8_t ch1[128];
uint8_t ch2[128];
uint8_t index = 0;

//***************  Main  ***************

int main(void)
{
  PortsInit();
  LCD_init();
  LCD_showParameters();

  while (1) 
  {
    uint8_t temp = Keypad();

    if (temp == '1')
      a += 0.2;

    if (temp == '2')
      a -= 0.2;

    if (temp == '3')
      b += 0.2;

    if (temp == '4')
      b -= 0.2;
    
    if(temp == '1' || temp == '2'|| temp == '3'|| temp == '4')
      SendByte(temp);

    if ((temp == '5') && (TIM2 -> ARR < 1000))
        TIM2 -> ARR += 100;

    if ((temp == '6') && (TIM2 -> ARR > 400))
        TIM2 -> ARR -= 100;
    
      LCD_command(1);
      LCD_showParameters();
  }
}

//***************  Methods  ***************

//Initialize port pins then initialize LCD controller
void LCD_init(void) 
{
  DelayMs(30);
  LCD_command(0x30);
  DelayMs(10);
  LCD_command(0x30);
  DelayMs(1);
  LCD_command(0x30);

  LCD_command(0x38);                    //Set 8-bit data, 2-line, 5x7 font
  LCD_command(0x06);                    //Move cursor right after each char
  LCD_command(0x01);                    //Clear screen - move cursor to home
  LCD_command(0x0F);                    //Turn on display - cursor blinking
}

void PortsInit(void) 
{
  RCC -> AHB1ENR |= 0x07;               //Enable GPIOA and GPIOB and GPIOC clock
  RCC -> APB2ENR |= 0x00000010;         //Enable USART1 clock

  GPIOA -> AFR[1] = 0x00000770;         //ALT7 for USART1
  GPIOA -> MODER = 0x00280F3C;          //Enable ALT function for Pin9 PortA

  RCC -> APB2ENR |= RCC_APB2ENR_ADC1EN; //ADC1 clock enabled
  ADC1 -> CR2 = 0;                      //Will be triggered from our program (Soft Waire trigger)
  ADC1 -> SQR3 = 0x1;                   //Conversion sequence starts at ch1
  ADC1 -> SQR1 |= 0;                    //Conversion sequence length 1
  ADC1 -> CR2 |= 1;                     //ADC1 enabled

  USART1 -> BRR = 0x0683;               //9600 baud rate
  USART1 -> CR1 = 0x000C;               //Enable Tx with 1 byte of data
  USART1 -> CR2 = 0x0000;               //Having one stop bit
  USART1 -> CR3 = 0x0000;               //Using no flow control
  USART1 -> CR1 |= 0x2000;              //USART1 enabled

  GPIOB -> MODER &= ~0x0000FC00;        //Clear pin mode
  GPIOB -> MODER = 0x00005400;          //Set pin output mode (LCD)
  GPIOB -> MODER |= 0x00150000;         //Set pin output mode (keypad)
  GPIOB -> PUPDR &= ~0x00000100;        //Clear pull-up pull-down
  GPIOB -> PUPDR = 0xAA000200;          //Set pull-up pull-down
  GPIOB -> BSRR = 0x00C00000;           //Turn off E and RW

  GPIOC -> MODER &= ~0x0000FFFF;        //Clear pin mode
  GPIOC -> MODER |= 0x00005555;         //Set pin output mode for LCD

  RCC -> APB1ENR |= RCC_APB1ENR_TIM5EN; //TIM5 clock enabled
  TIM5 -> PSC = 16 - 1;                 //Prescaled and divided by 16
  TIM5 -> ARR = 20 - 1;                 //Count to 20 using auto reload register
  TIM5 -> DIER |= TIM_DIER_UIE;         //Enable universal INT (When counter reached at expected value throw a INT)
  TIM5 -> CR1 = TIM_CR1_CEN;            //Counter enabled
  NVIC_EnableIRQ(TIM5_IRQn);            //Interrupt in NVIC enabled

  RCC -> APB1ENR |= RCC_APB1ENR_TIM2EN;
  TIM2 -> PSC = 16000 - 1;
  TIM2 -> ARR = 400 - 1;
  TIM2 -> DIER |= TIM_DIER_UIE;
  TIM2 -> CR1 |= TIM_CR1_CEN;
  NVIC_EnableIRQ(TIM2_IRQn);            //Interrupt in NVIC enabled

  __enable_irq();
}

void LCD_command(unsigned char command) 
{
  GPIOB -> BSRR = (RS | RW) << 16;      //RS = 0 - RW = 0
  GPIOC -> ODR = command;               //Put command to ODR
  GPIOB -> BSRR = EN;                   //Raise E
  DelayMs(0);
  GPIOB -> BSRR = EN << 16;             //Fall E

  if (command < 4)
    DelayMs(5);                         //Command 1 and 2 needs up to 1.64ms
  else
    DelayMs(2);                         //All others 40 us
}

void LCD_data(char data) 
{
  GPIOB -> BSRR = RS;                   //RS = 1
  GPIOB -> BSRR = RW << 16;             //RW = 0
  GPIOC -> ODR = data;                  //Put data to ODR
  GPIOB -> BSRR = EN;                   //Raise E
  DelayMs(1);
  GPIOB -> BSRR = EN << 16;             //Fall E
  DelayMs(2);
}

void LCD_showParameters(void) 
{
  LCD_data('b');
  LCD_data('=');
  char charsOfNumber[20];
  sprintf(charsOfNumber, "%.1f", b);
  uint8_t index = 0;

  do 
  {
    LCD_data(charsOfNumber[index]);
    index++;
  }
  while (charsOfNumber[index] != '\0' && index < 20);

  LCD_data(' ');
  LCD_data('V');
  LCD_data('U');
  LCD_data('=');
  sprintf(charsOfNumber, "%.1f", a);
  index = 0;

  do 
  {
    LCD_data(charsOfNumber[index]);
    index++;
  }
  while (charsOfNumber[index] != '\0' && index < 20);

  LCD_command(0xC0);
  LCD_data('T');
  LCD_data('U');
  LCD_data(' ');
  LCD_data('=');
  LCD_data(' ');
  sprintf(charsOfNumber, "%d", TIM2 -> ARR + 1);
  index = 0;

  do 
  {
    LCD_data(charsOfNumber[index]);
    index++;
  }
  while (charsOfNumber[index] != '\0' && index < 20);

  LCD_data('m');
  LCD_data('s');
}

//Delay n milliseconds (16 MHz CPU clock)
void DelayMs(int n) 
{
  int i;
  for (; n > 0; n--)
    for (i = 0; i < 3195; i++)
      __NOP();
}

uint8_t KeypadColumn(uint16_t odr, uint8_t col) 
{
  GPIOB -> ODR = odr;
  uint16_t input = GPIOB -> IDR & 0xF000;

  if (input != 0x0000) 
  {
    while ((GPIOB -> IDR & 0xF000) != 0) 
    {/*Busy waiting*/}
    
    switch (input) {

      case (0x1000):
        return col + '0';

      case (0x2000):
        return col + 3 + '0';
      
      case (0x4000):
        return col + 6 + '0';
      
      case (0x8000):
        return col;
    }
  } else return 0;
}

uint8_t Keypad(void) 
{
  for (;;) 
  {
    uint8_t tmp = KeypadColumn(0x0100, 1);
    
    if (tmp != 0) 
    {
      if (tmp == 1) 
        return '*';

      else
        return tmp;
    }

    tmp = KeypadColumn(0x0200, 2);

    if (tmp != 0) 
    {
      if (tmp == 2) 
        return '0';

      else
        return tmp;
    }

    tmp = KeypadColumn(0x0400, 3);

    if (tmp != 0) 
    {
      if (tmp == 3) 
        return '#';

      else
        return tmp;
    }
  }
}

void SendByte(uint8_t data) 
{
  while (!(USART1 -> SR & 0x0080))
  {/*Busy waiting*/}

  USART1 -> DR = (data & (uint8_t) 0xFF);
}

//Fill buffer
void TIM5_IRQHandler(void) 
{
  ADC1 -> SQR3 = 1;                         //Select channel 1 (PA1)
  ADC1 -> CR2 |= ADC_CR2_SWSTART;           //SW trigger

  while (!(ADC1 -> SR & 2))
  {/*Busy waiting*/}

  ch1[index] = (ADC1 -> DR >> 4);           //Devided by 16
  
  ADC1 -> SQR3 = 2;                         //Select channel 2 (PA2)
  ADC1 -> CR2 |= ADC_CR2_SWSTART;           //SW trigger

  while (!(ADC1 -> SR & 2)) 
  {/*Busy waiting*/}

  ch2[index] = (ADC1 -> DR >> 4);           //Devided by 16
  index = index == 127 ? 0 : index + 1;
}

//Send buffer contents to U2 (STM32-2)
void TIM2_IRQHandler(void) 
{
  SendByte(0x01);

  for (int i = index; i < 128; i++)
    SendByte(ch1[i]);

  for (int i = 0; i < index; i++)
    SendByte(ch1[i]);

  SendByte(0x02);
  for (int i = index; i < 128; i++)
    SendByte(ch2[i]);

  for (int i = 0; i < index; i++)
    SendByte(ch2[i]);

  index = 0;
}