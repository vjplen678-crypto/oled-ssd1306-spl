#include "stm32f10x.h"
#include "ssd1306.h"
#include "fonts.h"
#include <stdio.h>

volatile uint32_t gas_alert_count = 0;
volatile uint8_t prev_state = 0;         
volatile uint32_t debounce_counter = 0;  

void GPIO_Config(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2ENR_IOPBEN, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}


void TIM2_Config(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1ENR_TIM2EN, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = 10000 - 1;    
    TIM_TimeBaseStructure.TIM_Prescaler = 7200 - 1;  
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
}


void TIM2_IRQHandler(void) {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        uint8_t curr_state = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0);

        
        if (debounce_counter > 0) {
            debounce_counter--;
        }

       
        if (curr_state && !prev_state && debounce_counter == 0) {
            gas_alert_count++;

            char buf[20];
            sprintf(buf, "Count: %lu", gas_alert_count);

            ssd1306_Fill(Black);
            ssd1306_SetCursor(0, 0);
            ssd1306_WriteString("Gas Alert", Font_7x18, White);
            ssd1306_SetCursor(0, 20);
            ssd1306_WriteString(buf, Font_7x18, White);
            ssd1306_UpdateScreen();
        }

        prev_state = curr_state; 

        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}

int main(void) {
    SystemInit();
    ssd1306_Init();
    GPIO_Config();

 
    prev_state = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0);

    TIM2_Config();

    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("chua co Gas", Font_7x18, White);
    ssd1306_UpdateScreen();

    while (1) {
      
    }
}
