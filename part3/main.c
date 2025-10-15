#include <stdint.h>


uint32_t syscfg_base = 0x40010000;
uint32_t* RCC_AHB2ENR = (uint32_t*) (0x4C + 0x40021000);
uint32_t GPIOC_BASE = 0x48000800;
uint32_t GPIOB_BASE = 0x48000400; // PB7 & PB14

#define EXTI_BASE 0x40010400
#define NVIC_ISER 0xE000E100
#define TIM4_BASE 0x40000800

// B1 is connected to PC13
// Blue LED is connected to PB7
void set_up(void){
	// Turn on Clocks for GPIOs Port B and C, and SYSCFGEN
	uint32_t* RCC_AHB2ENR = (uint32_t*) (0x4C + 0x40021000);
	uint32_t* RCC_ABP2ENR = (uint32_t*) (0x60 + 0x40021000);

	*RCC_AHB2ENR |= (0x3 << 1);
	*RCC_ABP2ENR |= 0x1;
	// Set GPIO PC13 as Input
	uint32_t* GPIOC_MODE = (uint32_t*) GPIOC_BASE;
	uint32_t input_mask = ~((0x3 << 26));
	*GPIOC_MODE = input_mask;

	// Set GPIO PB7 as output
	uint32_t* GPIOB_MODE = (uint32_t*) GPIOB_BASE;
	uint32_t reset_value = *GPIOB_MODE;
	reset_value &= ~(0x3 << 14);
	*GPIOB_MODE = reset_value | (0x1 << 14);


	// Set EXTI13 interrupt source to GPIO Bank C
	// EXTI4 C
	uint32_t* syscfg_exticr4 = (uint32_t*)(syscfg_base + 0x14);
	*syscfg_exticr4 |= (0x2 << 4);


	// CONFIGURE EXTI Settings
	uint32_t* EXTI_INT = (uint32_t*)(EXTI_BASE);
	*EXTI_INT |= (0x1 << 13);

	uint32_t* EXTI_RISING_SEL = (uint32_t*) (EXTI_BASE + 0x08);
	*EXTI_RISING_SEL = (0x1 << 13);
	uint32_t* EXTI_FALLING_SEL = (uint32_t*) (EXTI_BASE + 0x0C);
	*EXTI_FALLING_SEL |= (0x1 << 13);
	// Configure NVIC Registers
	// EXTI(15-10) take position 40 in the NVIC Table
	uint32_t* NVIC_ISER1 = (uint32_t*)(NVIC_ISER + 0x04);
	// Set bit 8 on ISER1 (Interrupt 40) to 1
	*NVIC_ISER1 |= (0x1 << 8);
}

void setup_timer(void){
	// Set PB14 (Red LED) and PC7 (Green LED) as output
	uint32_t* GPIOB_MODE = (uint32_t*) GPIOB_BASE;
	uint32_t reset_value = *GPIOB_MODE;
	reset_value &= (~(0x3 << 28));
	*GPIOB_MODE = (reset_value);
	*GPIOB_MODE	|= (0x1 << 28);

	uint32_t* GPIOC_MODE = (uint32_t*) GPIOC_BASE;
	reset_value = *GPIOC_MODE;
	reset_value &= ~(0x3 << 14);
	*GPIOC_MODE = reset_value ;
	*GPIOC_MODE |= (0x1 << 14);

	// TUrn on HSI16
	uint32_t* RCC_CR = (uint32_t*) (0x40021000);
	*RCC_CR |= (0x1 << 8);
	uint8_t HSI_RDY_FLAG = 0;
	while (HSI_RDY_FLAG == 0){
		HSI_RDY_FLAG = ( (*RCC_CR >> 10) & 0x1);
	}

	uint32_t* RCC_CFGR = (uint32_t*) (0x40021000 + 0x8);
	*RCC_CFGR |= 0x01;
	uint8_t SYS_CLK_RDY = 0;
	while (SYS_CLK_RDY == 0){
		SYS_CLK_RDY = ((*RCC_CFGR >> 2) & 0x3);
	}
	// Enable TIM4
	uint32_t* RCC_APB1ENR = (uint32_t*) (0x58 + 0x40021000);
	*RCC_APB1ENR |= (0x1 << 2);
	// TIM4 is 16 Bits
	// 500kHz -> 1us
	uint32_t* TIM4_PSC = (uint32_t*) (0x28 + TIM4_BASE);
	*TIM4_PSC = 0x13F; // prescaler = (319 + 1) -> 16MHz/320 = 50kHz

	// Set AR Value
	uint32_t* TIM4_ARR = (uint32_t*) (0x2C + TIM4_BASE);
	*TIM4_ARR = 0xC350;

	// Enable Interrupt
	uint32_t* TIM4_DIER = (uint32_t*) (0x0C + TIM4_BASE);
	*TIM4_DIER |= (0x1 << 6);

	// Enable Event
	*TIM4_DIER |= (0x1);

	// Enable Clock
	uint32_t* TIM4_CR1 = (uint32_t*) (TIM4_BASE);
	*TIM4_CR1 |= 0x1;

	//TIM4 Interrupt Position = 30
	uint32_t* NVIC_ISER0 = (uint32_t*)(NVIC_ISER);
	*NVIC_ISER0 |= (0x1 << 30);
	return;
}

#define GREEN 0x01
#define BLUE 0x02
#define RED 0x03
uint8_t color = GREEN;
uint8_t btn_state = 0;
uint8_t change_color =0;

void TIM4_IRQHandler(void){
	uint32_t* TIM4_PENDING = (uint32_t*)(TIM4_BASE + 0x10);
	*TIM4_PENDING &= ~(0x1);

	// Toggle GPIO
	change_color = 1;
	return;
}




void EXTI15_10_IRQHandler(void){
	uint32_t* EXTI_PENDING = (uint32_t*)(EXTI_BASE + 0x14);
	*EXTI_PENDING = (0x1 << 13);

	btn_state = ~btn_state;

	return;
}



#define PRESSED 0x1
#define RELEASED 0x0
int main(void){
	set_up();
	setup_timer();
	// your code goes here
	while(1){
		if(btn_state){
			while(btn_state){}
			uint8_t delay = 100;
			while(delay > 0){
				delay -= 1;
			}
		}
		if ((change_color > 0)){
			if (color == GREEN){
					uint32_t* GPIOB_OUT = (uint32_t*)( GPIOB_BASE + 0x14);
					*GPIOB_OUT = 0;
					uint32_t* GPIOC_OUT = (uint32_t*)(GPIOC_BASE + 0x14);
					// Extract bit for GPIO 7
					uint32_t last_data = ((*GPIOC_OUT >> 7) & 1);
					// Flip GPIO 7 bit
					uint32_t new_data = ((~last_data) & 1) << 7;
					// Set GPIO B Port Data to reflect flipped bit
					*GPIOC_OUT = (new_data);
					color += 1;
				}

				else if (color == BLUE){
					uint32_t* GPIOC_OUT = (uint32_t*)(GPIOC_BASE + 0x14);
					*GPIOC_OUT = 0;
					uint32_t* GPIOB_OUT = (uint32_t*)( GPIOB_BASE + 0x14);
					// Extract bit for GPIO 7
					uint32_t last_data = ((*GPIOB_OUT >> 7) & 1);
					// Flip GPIO 7 bit
					uint32_t new_data = ((~last_data) & 1) << 7;
					// Set GPIO B Port Data to reflect flipped bit
					*GPIOB_OUT = (new_data);
					color += 1;
				}


				else if (color == RED){
					uint32_t* GPIOB_OUT = (uint32_t*)( GPIOB_BASE + 0x14);
					*GPIOB_OUT = 0;
					// Extract bit for GPIO 7
					uint32_t last_data = ((*GPIOB_OUT >> 14) & 1);
					// Flip GPIO 7 bit
					uint32_t new_data = ((~last_data) & 1) << 14;
					// Set GPIO B Port Data to reflect flipped bit
					*GPIOB_OUT = (new_data);
					color = GREEN;
				}
		change_color = 0;
		}
	}
}
