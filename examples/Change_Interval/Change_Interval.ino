/****************************************************************************************************************************
  Change_Interval.ino
  For Portenta_H7 boards
  Written by Khoi Hoang

  Built by Khoi Hoang https://github.com/khoih-prog/Portenta_H7_TimerInterrupt
  Licensed under MIT license

  Now even you use all these new 16 ISR-based timers,with their maximum interval practically unlimited (limited only by
  unsigned long miliseconds), you just consume only one Portenta_H7 STM32 timer and avoid conflicting with other cores' tasks.
  The accuracy is nearly perfect compared to software timers. The most important feature is they're ISR-based timers
  Therefore, their executions are not blocked by bad-behaving functions / tasks.
  This important feature is absolutely necessary for mission-critical tasks.
*****************************************************************************************************************************/

/*
   Notes:
   Special design is necessary to share data between interrupt code and the rest of your program.
   Variables usually need to be "volatile" types. Volatile tells the compiler to avoid optimizations that assume
   variable can not spontaneously change. Because your function may change variables while your program is using them,
   the compiler needs this hint. But volatile alone is often not enough.
   When accessing shared variables, usually interrupts must be disabled. Even with volatile,
   if the interrupt changes a multi-byte variable between a sequence of instructions, it can be read incorrectly.
   If your data is multiple variables, such as an array and a count, usually interrupts need to be disabled
   or the entire sequence of your code which accesses the data.
*/

#if !( ( defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_PORTENTA_H7_M4) || defined(ARDUINO_GIGA) ) && defined(ARDUINO_ARCH_MBED) )
  #error This code is designed to run on Portenta_H7 platform! Please check your Tools->Board setting.
#endif

// These define's must be placed at the beginning before #include "Portenta_H7_TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define _TIMERINTERRUPT_LOGLEVEL_     4

// Can be included as many times as necessary, without `Multiple Definitions` Linker Error
#include "Portenta_H7_TimerInterrupt.h"

#define LED_OFF             HIGH
#define LED_ON              LOW

#ifndef LED_BUILTIN
  #define LED_BUILTIN       LEDG               // Pin 24 control on-board LED_GREEN on Portenta_H7
#endif

#ifndef LED_BLUE
  #define LED_BLUE          LEDB               // Pin 25 control on-board LED_BLUE on Portenta_H7
#endif

#ifndef LED_RED
  #define LED_RED           LEDR              // Pin 23 control on-board LED_RED on Portenta_H7
#endif

#define TIMER0_INTERVAL_MS        500
#define TIMER1_INTERVAL_MS        1000

volatile uint32_t Timer0Count = 0;
volatile uint32_t Timer1Count = 0;

// Depending on the board, you can select STM32H7 Hardware Timer from TIM1-TIM22
// If you select a Timer not correctly, you'll get a message from compiler
// 'TIMxx' was not declared in this scope; did you mean 'TIMyy'? 

// Portenta_H7 OK       : TIM1, TIM4, TIM7, TIM8, TIM12, TIM13, TIM14, TIM15, TIM16, TIM17
// Portenta_H7 Not OK   : TIM2, TIM3, TIM5, TIM6, TIM18, TIM19, TIM20, TIM21, TIM22
// Portenta_H7 No timer : TIM9, TIM10, TIM11. Only for STM32F2, STM32F4 and STM32L1 
// Portenta_H7 No timer : TIM18, TIM19, TIM20, TIM21, TIM22

// Init timer TIM15
Portenta_H7_Timer ITimer0(TIM15);
// Init  timer TIM16
Portenta_H7_Timer ITimer1(TIM16);

void printResult(uint32_t currTime)
{
  Serial.print(F("Time = ")); Serial.print(currTime); 
  Serial.print(F(", Timer0Count = ")); Serial.print(Timer0Count);
  Serial.print(F(", Timer1Count = ")); Serial.println(Timer1Count);
}

// In Portenta_H7, avoid doing something fancy in ISR, for example Serial.print
// Or you can get this run-time error / crash

void TimerHandler0()
{
  static bool toggle0 = false;

  // Flag for checking to be sure ISR is working as SErial.print is not OK here in ISR
  Timer0Count++;

  //timer interrupt toggles pin LED_BUILTIN
  digitalWrite(LED_BUILTIN, toggle0);
  toggle0 = !toggle0;
}

void TimerHandler1()
{
  static bool toggle1 = false;

  // Flag for checking to be sure ISR is working as Serial.print is not OK here in ISR
  Timer1Count++;
  
  //timer interrupt toggles outputPin
  digitalWrite(LED_BLUE, toggle1);
  toggle1 = !toggle1;
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BLUE,    OUTPUT);
  
  Serial.begin(115200);
  while (!Serial);

  delay(100);

  Serial.print(F("\nStarting Change_Interval on ")); Serial.println(BOARD_NAME);
  Serial.println(PORTENTA_H7_TIMER_INTERRUPT_VERSION);
 
  // Interval in microsecs
  if (ITimer0.attachInterruptInterval(TIMER0_INTERVAL_MS * 1000, TimerHandler0))
  {
    Serial.print(F("Starting  Timer0 OK, millis() = ")); Serial.println(millis());
  }
  else
    Serial.println(F("Can't set ITimer0. Select another freq. or timer"));

  // Interval in microsecs
  if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS * 1000, TimerHandler1))
  {
    Serial.print(F("Starting ITimer1 OK, millis() = ")); Serial.println(millis());
  }
  else
    Serial.println(F("Can't set ITimer1. Select another freq. or timer"));
}

#define CHECK_INTERVAL_MS     10000L
#define CHANGE_INTERVAL_MS    20000L

void loop()
{
  static uint32_t lastTime = 0;
  static uint32_t lastChangeTime = 0;
  static uint32_t currTime;
  static uint32_t multFactor = 0;

  currTime = millis();

  if (currTime - lastTime > CHECK_INTERVAL_MS)
  {
    printResult(currTime);
    lastTime = currTime;

    if (currTime - lastChangeTime > CHANGE_INTERVAL_MS)
    {
      //setInterval(unsigned long interval, timerCallback callback)
      multFactor = (multFactor + 1) % 2;
      
      ITimer0.setInterval(TIMER0_INTERVAL_MS * 1000 * (multFactor + 1), TimerHandler0);
      ITimer1.setInterval(TIMER1_INTERVAL_MS * 1000 * (multFactor + 1), TimerHandler1);

      Serial.print(F("Changing Interval, Timer0 = ")); Serial.print(TIMER0_INTERVAL_MS * (multFactor + 1));
      Serial.print(F(",  Timer1 = ")); Serial.println(TIMER1_INTERVAL_MS * (multFactor + 1)); 
    
      lastChangeTime = currTime;
    }
  }
  if (Serial.available()) {
    while (Serial.read() != -1) {}
    print_timer_regs(15, TIM15);
    print_timer_regs(16, TIM16);

  }
}

void print_timer_regs(uint8_t timer_num, TIM_TypeDef *ptimer) {
    Serial.print("Timer("); Serial.print(timer_num);
    Serial.print("): "); Serial.println((uint32_t)ptimer, HEX);
    Serial.print("\tCR1: "); Serial.print(ptimer->CR1, HEX); Serial.println("\tcontrol register 1");
    Serial.print("\tCR2: "); Serial.print(ptimer->CR2, HEX); Serial.println("\tcontrol register 2");
    Serial.print("\tSMCR: "); Serial.print(ptimer->SMCR, HEX); Serial.println("\tslave mode control register");
    Serial.print("\tDIER: "); Serial.print(ptimer->DIER, HEX); Serial.println("\tDMA/interrupt enable register");
    Serial.print("\tSR: "); Serial.print(ptimer->SR, HEX); Serial.println("\tstatus register");
    Serial.print("\tEGR: "); Serial.print(ptimer->EGR, HEX); Serial.println("\tevent generation register");
    Serial.print("\tCCMR1: "); Serial.print(ptimer->CCMR1, HEX); Serial.println("\tcapture/compare mode register 1");
    Serial.print("\tCCMR2: "); Serial.print(ptimer->CCMR2, HEX); Serial.println("\tcapture/compare mode register 2");
    Serial.print("\tCCER: "); Serial.print(ptimer->CCER, HEX); Serial.println("\tcapture/compare enable register");
    Serial.print("\tCNT: "); Serial.print(ptimer->CNT, HEX); Serial.println("\tcounter register");
    Serial.print("\tPSC: "); Serial.print(ptimer->PSC, HEX); Serial.println("\tprescaler");
    Serial.print("\tARR: "); Serial.print(ptimer->ARR, HEX); Serial.println("\tauto-reload register");
    Serial.print("\tRCR: "); Serial.print(ptimer->RCR, HEX); Serial.println("\trepetition counter register");
    Serial.print("\tCCR1: "); Serial.print(ptimer->CCR1, HEX); Serial.println("\tcapture/compare register 1");
    Serial.print("\tCCR2: "); Serial.print(ptimer->CCR2, HEX); Serial.println("\tcapture/compare register 2");
    Serial.print("\tCCR3: "); Serial.print(ptimer->CCR3, HEX); Serial.println("\tcapture/compare register 3");
    Serial.print("\tCCR4: "); Serial.print(ptimer->CCR4, HEX); Serial.println("\tcapture/compare register 4");
    Serial.print("\tBDTR: "); Serial.print(ptimer->BDTR, HEX); Serial.println("\tbreak and dead-time register");
    Serial.print("\tDCR: "); Serial.print(ptimer->DCR, HEX); Serial.println("\tDMA control register");
    Serial.print("\tDMAR: "); Serial.print(ptimer->DMAR, HEX); Serial.println("\tDMA address for full transfer");
    Serial.print("\tCCMR3: "); Serial.print(ptimer->CCMR3, HEX); Serial.println("\tcapture/compare mode register 3");
    Serial.print("\tCCR5: "); Serial.print(ptimer->CCR5, HEX); Serial.println("\tcapture/compare register5");
    Serial.print("\tCCR6: "); Serial.print(ptimer->CCR6, HEX); Serial.println("\tcapture/compare register6");
    Serial.print("\tAF1: "); Serial.print(ptimer->AF1, HEX); Serial.println("\talternate function option register 1");
    Serial.print("\tAF2: "); Serial.print(ptimer->AF2, HEX); Serial.println("\talternate function option register 2");
    Serial.print("\tTISEL: "); Serial.print(ptimer->TISEL, HEX); Serial.println("\tInput Selection register");

}
