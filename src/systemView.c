#include "Arduino.h"
#include "SEGGER_SYSVIEW.h"

uint32_t sampleRate = 1000; //sample rate in milliseconds, determines how often TC5_Handler is called

#define LED_PIN LED_BUILTIN //just for an example
bool state = 0; //just for an example

volatile uint16_t usCnt = 0;

//this function gets called by the interrupt at <sampleRate>Hertz
void TC5_Handler (void) {
  //YOUR CODE HERE 
  usCnt++;
//   digitalWrite(A0, LOW);
//   digitalWrite(A0, HIGH);
  // END OF YOUR CODE
  TC5->COUNT16.INTFLAG.bit.MC0 = 1; //Writing a 1 to INTFLAG.bit.MC0 clears the interrupt so that it will run again
}

bool tcIsSyncing()
{
    return TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY;
}

//This function enables TC5 and waits for it to be ready
void tcStartCounter()
{
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE; //set the CTRLA register
    while (tcIsSyncing()); //wait until snyc'd
}

//Reset TC5 
void tcReset()
{
    TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (tcIsSyncing());
    while (TC5->COUNT16.CTRLA.bit.SWRST);
}

//disable TC5
void tcDisable()
{
    TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
    while (tcIsSyncing());
}

void sysViewInit(void) {

    pinMode(A0 ,OUTPUT); //this configures the LED pin, you can remove this it's just example code
    
    // select the generic clock generator used as source to the generic clock multiplexer
    GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
    while (GCLK->STATUS.bit.SYNCBUSY);

    tcReset(); //reset TC5

    // Set Timer counter 5 Mode to 16 bits, it will become a 16bit counter ('mode1' in the datasheet)
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
    // Set TC5 waveform generation mode to 'match frequency'
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
    //set prescaler
    //the clock normally counts at the GCLK_TC frequency, but we can set it to divide that frequency to slow it down
    //you can use different prescaler divisons here like TC_CTRLA_PRESCALER_DIV1 to get a different range
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_ENABLE; //it will divide GCLK_TC frequency by 1024
    
    //set the compare-capture register. 
    //The counter will count up to this value (it's a 16bit counter so we use uint16_t)
    //this is how we fine-tune the frequency, make it count to a lower or higher value
    //system clock should be 1MHz (8MHz/8) at Reset by default
    TC5->COUNT16.CC[0].reg = (uint16_t) (F_CPU / 100000);
    while (tcIsSyncing());

    // Configure interrupt request
    NVIC_DisableIRQ(TC5_IRQn);
    NVIC_ClearPendingIRQ(TC5_IRQn);
    NVIC_SetPriority(TC5_IRQn, 0);
    NVIC_EnableIRQ(TC5_IRQn);

    // Enable the TC5 interrupt request
    TC5->COUNT16.INTENSET.bit.MC0 = 1;
    while (tcIsSyncing()); //wait until TC5 is done syncing 

    tcStartCounter(); //starts the timer

    SEGGER_SYSVIEW_Conf(); 


}