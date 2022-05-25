#include "stm32f4xx.h"
#include <stdio.h>

//***************  Signitures  ***************

void DelayMs(int n);
uint8_t RecieveByte(void);
void PortsInit(void);
void GLCD_init(void);
void GLCD_command(uint8_t CS, uint8_t command);
void GLCD_data(uint8_t CS, uint8_t data);
void GLCD_setCursor(uint8_t y, uint8_t line);
void GLCD_clearColumnInLine(uint8_t line, uint8_t y);
void GLCD_drawColumnInLine(uint8_t line, uint8_t y, uint8_t value);

//***************  Parameters  ***************

uint16_t glcdCs1 = 0x0004;       //GPIO PortB  Pin2
uint16_t glcdCs2 = 0x0008;       //GPIO PortB  Pin3
uint16_t glcdRs = 0x0010;        //GPIO PortB  Pin4
uint16_t glcdRw = 0x0020;        //GPIO PortB  Pin5
uint16_t glcdEn = 0x0040;        //GPIO PortB  Pin6
uint16_t glcdRes = 0x0080;       //GPIO PortB  Pin7
uint16_t glcd_dataPins[8];        //GPIO PortA  Pin0 to Pin7

uint8_t ch1[128], ch2[128];
double A = 1;
double B = 0;
int c = 0;

//***************  Main  ***************

int main(void) 
{
  PortsInit();
  GLCD_init();

  while (1) 
  {
    uint8_t temp = RecieveByte();

    if (temp == '1')
      A = A + 0.2;

    if (temp == '2')
      A = A - 0.2;

    if (temp == '3')
      B = B + 0.2;

    if (temp == '4')
      B = B - 0.2;

    if (temp == 1)
    {
      for (int i = 0; i < 128; i++) 
      {
          uint8_t ch2_line = ch2[i] / 8;
          uint8_t ch1_line = ch1[i] / 8;
          
          if (i != 64)
            GLCD_clearColumnInLine(ch1_line, i);

          if (ch2_line == ch1_line) 
          {
            if (ch2_line == 4)
              GLCD_drawColumnInLine(ch2_line, i, (1 << (ch2[i] % 8)) | 1);

            else
              GLCD_drawColumnInLine(ch2_line, i, (1 << (ch2[i] % 8)));
          }

          double voltage = (A * (((double) RecieveByte() / 255) * 3.3) + B) / 3.3;
          uint16_t value = voltage * 64;
          
          if (value < 64) 
          {
            ch1[i] = 63 - value;
            ch1_line = ch1[i] / 8;

            if (ch1_line != ch2_line) 
            {
              if (ch1_line == 4)
                GLCD_drawColumnInLine(ch1_line, i, (1 << (ch1[i] % 8)) | 1);

              else
                GLCD_drawColumnInLine(ch1_line, i, (1 << (ch1[i] % 8)));
            } 

            else 
            {
              if (ch2_line == 4)
                GLCD_drawColumnInLine(ch2_line, i, ((1 << (ch2[i] % 8)) | (1 << (ch1[i] % 8))) | 1);

              else
                GLCD_drawColumnInLine(ch2_line, i, ((1 << (ch2[i] % 8)) | (1 << (ch1[i] % 8))));
            }
          }
      }
    }

    if (temp == 2) 
    {
      for (int i = 0; i < 128; i++) 
      {
        uint8_t ch2_line = ch2[i] / 8;
        uint8_t ch1_line = ch1[i] / 8;
        
        if (i != 64)
          GLCD_clearColumnInLine(ch2_line, i);

        if (ch1_line == ch2_line) 
        {
          if (ch1_line == 4)
            GLCD_drawColumnInLine(ch1_line, i, (1 << (ch1[i] % 8)) | 1);
            
          else
            GLCD_drawColumnInLine(ch1_line, i, (1 << (ch1[i] % 8)));
        }

        double voltage = (A * (((double) RecieveByte() / 255) * 3.3) + B) / 3.3;
        uint16_t value = voltage * 64;

        if (value < 64) 
        {
          ch2[i] = 63 - value;
          ch2_line = ch2[i] / 8;

          if (ch2_line != ch1_line) 
          {
            if (ch2_line == 4)
              GLCD_drawColumnInLine(ch2_line, i, (1 << (ch2[i] % 8)) | 1);

            else
              GLCD_drawColumnInLine(ch2_line, i, (1 << (ch2[i] % 8)));
          } 
          else 
          {
            if (ch1_line == 4)
              GLCD_drawColumnInLine(ch1_line, i, ((1 << (ch1[i] % 8)) | (1 << (ch2[i] % 8))) | 1);
            
            else
              GLCD_drawColumnInLine(ch1_line, i, ((1 << (ch1[i] % 8)) | (1 << (ch2[i] % 8))));
          }
        }
      }
    }
  }
}

//***************  Methods  ***************

void PortsInit(void) 
{
  RCC -> AHB1ENR |= 0x03;             //GPIOA and GPIOB clock enabled
  RCC -> APB2ENR |= 0x00000010;       //USART1 clock enabled

  GPIOA -> AFR[1] = 0x00000770;       //ALT7 for USART1
  GPIOA -> MODER = 0x40285555;        //Alternate function for PA9 enabled
  GPIOB -> MODER = 0x00005555;

  USART1 -> BRR = 0x0683;             //9600 baud rate
  USART1 -> CR1 = 0x000C;             //Enable Tx - 8-bit data
  USART1 -> CR2 = 0x0000;             //one stop bit
  USART1 -> CR3 = 0x0000;             //No flow control
  USART1 -> CR1 |= 0x2000;            //USART2 enabled
}

uint8_t RecieveByte(void) 
{
  while (!(USART1 -> SR & 0x0020)) 
    c++;

  return USART1 -> DR;
}

//Delay n milliseconds (16 MHz CPU clock)
void DelayMs(int n) 
{
  int i;

  for (; n > 0; n--)
    for (i = 0; i < 3195; i++) 
      __NOP();
}

void GLCD_init(void) 
{
  for (int i = 0; i < 8; i++) 
  {
    glcd_dataPins[i] = (1 << i);
    GPIOA -> BSRR = (glcd_dataPins[i] << 16);
  }

  GPIOB -> BSRR = (glcdRs << 16);
  GPIOB -> BSRR = (glcdRw << 16);
  GPIOB -> BSRR = (glcdRes << 16);
  GPIOB -> BSRR = (glcdEn << 16);
  GPIOB -> BSRR = (glcdCs1 << 16);
  GPIOB -> BSRR = (glcdCs2 << 16);
  DelayMs(5);
  GPIOB -> BSRR = (glcdRes);
  DelayMs(50);

  GLCD_command(0, 0x003F);
  GLCD_command(1, 0x003F);
  GLCD_command(0, 0x00C0);
  GLCD_command(1, 0x00C0);

  uint8_t line, y;
  for (line = 0; line < 8; ++line) 
  {
    for (y = 0; y < 128; ++y) 
    {
      if (y != 64)
        GLCD_clearColumnInLine(line, y);
    }
  }
}

void GLCD_command(uint8_t CS, uint8_t command) 
{
  GPIOB -> BSRR = (glcdRs << 16);
  GPIOB -> BSRR = (glcdRw << 16);

  if (CS == 1) 
  {
    GPIOB -> BSRR = (glcdCs1 << 16);
    GPIOB -> BSRR = (glcdCs2);
  } 

  else 
  {
    GPIOB -> BSRR = (glcdCs1);
    GPIOB -> BSRR = (glcdCs2 << 16);
  }

  for (int i = 0; i < 8; ++i) 
  {
    if ((command >> i) & 1)
      GPIOA -> BSRR = glcd_dataPins[i];

    else
      GPIOA -> BSRR = (glcd_dataPins[i] << 16);
  }

  GPIOB -> BSRR = glcdEn;
  DelayMs(0);
  GPIOB -> BSRR = (glcdEn << 16);
}

void GLCD_data(uint8_t CS, uint8_t data) {
  GPIOB -> BSRR = (glcdRs);
  GPIOB -> BSRR = (glcdRw << 16);
  
  if (CS == 1) 
  {
    GPIOB -> BSRR = (glcdCs1 << 16);
    GPIOB -> BSRR = (glcdCs2);
  }

  else 
  {
    GPIOB -> BSRR = (glcdCs1);
    GPIOB -> BSRR = (glcdCs2 << 16);
  }
  
  for (int i = 0; i < 8; ++i) 
  {
    if ((data >> i) & 1)
      GPIOA -> BSRR = glcd_dataPins[i];

    else
      GPIOA -> BSRR = (glcd_dataPins[i] << 16);
  }
  
  GPIOB -> BSRR = glcdEn;
  DelayMs(0);
  GPIOB -> BSRR = (glcdEn << 16);
}

void GLCD_setCursor(uint8_t line, uint8_t y) 
{
  if (y > 63) 
  {
    GLCD_command(0, 0x0040 + y - 64);
    GLCD_command(0, 0x00B8 + line);
  } 

  else 
  {
    GLCD_command(1, 0x0040 + y);
    GLCD_command(1, 0x00B8 + line);
  }
}

void GLCD_clearColumnInLine(uint8_t line, uint8_t y) 
{
  if (line == 4) 
  {
    GLCD_drawColumnInLine(4, y, 1);
    return;
  }

  GLCD_setCursor(line, y);
  
  if (y > 63) 
    GLCD_data(0, 0);

  else 
    GLCD_data(1, 0);
}

void GLCD_drawColumnInLine(uint8_t line, uint8_t y, uint8_t value) 
{
  if (y == 64)
    return;

  GLCD_setCursor(line, y);

  if (y > 63) 
    GLCD_data(0, value);

  else 
    GLCD_data(1, value);
}