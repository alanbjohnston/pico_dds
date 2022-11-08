#include "hardware/pwm.h"  // pwm 
#include "pico_dds.h"
#include "RPi_Pico_TimerInterrupt.h"

#define DDS_PWM_PIN 26

//#define DDS_ALT

bool debug_pwm = false;
volatile int dds_duration_us = 500;
bool dds_timer_started = false;
long time_stamp = 0;
int dds_count = 0;
volatile bool dds_enable = false;
volatile long dds_counter = 0;
int dds_pin_slice;
pwm_config dds_pwm_config;
byte sin_table[201];

int clock = 50E3;
float multiplier;
int wrap = 9;
int isr_period;

RPI_PICO_Timer dds_ITimer2(2);

bool dds_TimerHandler0(struct repeating_timer *t) {  // DDS timer for waveform
  if (dds_enable) {
     int index = ((int)(((float)(time_us_32() - time_stamp) / (float) dds_duration_us) * 200.0 )) % 200;

//      Serial.print(index);
//      Serial.print(" + ");

     uint16_t  i = sin_table[index];

      //      Serial.print(i);
//      Serial.print(" ");
    
      pwm_set_gpio_level(DDS_PWM_PIN, i);
  }
  return(true);
}


void dds_begin() {
  if (!dds_timer_started) { 
   
    multiplier = 133E6 / (clock * wrap);
    isr_period = (int) ( 1E6 / clock + 0.5);
    
    Serial.printf("DDS begin\nClock: %d Wrap: %d Multiplier: %4.1f Period in us: %d\n", clock, wrap, multiplier, isr_period);
    
#ifdef DDS_ALT    
    if (dds_ITimer2.attachInterruptInterval(isr_period, dds_TimerHandler0))	{   // was 10
      Serial.print(F("Starting dds_ITimer2 OK, micros() = ")); Serial.println(micros());
      dds_timer_started = true;
    }
    else
      Serial.println(F("Can't set dds_ITimer2. Select another Timer, freq. or timer"));
#endif    
    
  dds_counter = 0;  
    Serial.println("Starting pwm f= MHz!");
  
    Serial.println(" ");
  
    gpio_set_function(DDS_PWM_PIN, GPIO_FUNC_PWM);
    dds_pin_slice = pwm_gpio_to_slice_num(DDS_PWM_PIN);
      // Setup PWM interrupt to fire when PWM cycle is complete
#ifndef DDS_ALT    
    pwm_clear_irq(dds_pin_slice);
    pwm_set_irq_enabled(dds_pin_slice, true);
    // set the handle function above
    irq_set_exclusive_handler(PWM_IRQ_WRAP, dds_pwm_interrupt_handler); 
    irq_set_enabled(PWM_IRQ_WRAP, true);	  
#endif  
  
    dds_pwm_config = pwm_get_default_config();
    pwm_config_set_clkdiv(&dds_pwm_config, multiplier); // was 100.0 50 75 25.0); // 33.333);  // 1.0f
    pwm_config_set_wrap(&dds_pwm_config, wrap); // 3 
    pwm_init(dds_pin_slice, &dds_pwm_config, true);
    pwm_set_gpio_level(DDS_PWM_PIN, (dds_pwm_config.top + 1) * 0.5);
  
    Serial.printf("PWM config.top: %d\n", dds_pwm_config.top);
    
//  if (debug_pwm) 
  {	
    Serial.print(pwm_gpio_to_slice_num(DDS_PWM_PIN));
    Serial.print(" ");	
    Serial.print(pwm_gpio_to_channel(DDS_PWM_PIN));
    Serial.print(" ");		
    Serial.print(pwm_gpio_to_slice_num(DDS_PWM_PIN));
    Serial.print(" ");	
    Serial.print(pwm_gpio_to_channel(DDS_PWM_PIN));
    Serial.println(" ");	
  } 
  dds_timer_started = true;
}   
//  } 
  dds_enable = true;
 time_stamp = time_us_32();

    for (int i = 0; i < 200; i++)  {
      sin_table[i] = 0.5 * (wrap + 2) * sin((2 * 3.14 * i)/200.0) + 0.5 * (wrap + 1); //  + 0.5; 
      Serial.print(sin_table[i]);
      Serial.print(" ");
//      pwm_set_gpio_level(DDS_PWM_PIN, i);
//      delay(100);
    }
/*  
   Serial.println("10x");
    for (int j = 0; j < 10; j++) {
    for (int i = 0; i < 200; i++)  {
      pwm_set_gpio_level(DDS_PWM_PIN, sin_table[i]);
      delay(10);
    }
    }
 */   
  Serial.println("Sweep");
  for (int k = 100; k < 1500; k+=100) {
    dds_setfreq(k);
    delay(3000);
    Serial.println(k);
  }
  Serial.println("End");

}

void dds_pwm_interrupt_handler() {
//  pwm_clear_irq(pwm_gpio_to_slice_num(DDS_PWM_PIN)); 

  if (dds_enable) {
    if (dds_counter++ > 9) {  
      dds_counter = 0;
//    Serial.print(time_us_32() - time_stamp);
//    Serial.print("  > ");
//    time_stamp = time_us_32();
//    uint16_t  i = 0.5 * (dds_pwm_config.top) * sin((3.14 * time_us_32())/dds_duration_us) + 0.5 * (dds_pwm_config.top + 1);  // was 2 *

      int index = ((int)(((float)(time_us_32() - time_stamp) / (float) dds_duration_us) * 200.0 )) % 200;

//      Serial.print(index);
//      Serial.print(" + ");

     uint16_t  i = sin_table[index];

      //      Serial.print(i);
//      Serial.print(" ");
      pwm_set_gpio_level(DDS_PWM_PIN, i);
//    Serial.print(time_us_32());
//    Serial.print(" ");
//    time_stamp = time_us_32();
  }
    
  } else
     pwm_set_gpio_level(DDS_PWM_PIN,0);

    pwm_clear_irq(pwm_gpio_to_slice_num(DDS_PWM_PIN)); 
}



void dds_down() {
  dds_enable = false;
//  Serial.println("Stopping DDS");
}

void dds_setfreq(int freq) {

  dds_duration_us = 1E6 / (float)freq; // - 10;  // subtract 3 us of processing delay
//    Serial.print("Period: ");
//    Serial.println(dds_duration_us);

//  if (dds_duration_us != dds_duration_previous_us) {   // only change if frequency is different
//    dds_duration_previous_us = dds_duration_us;
    
//    time_stamp = time_us_32();
//  }   
}
