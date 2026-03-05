//////////////////////////////////////////////////////////////////////
// TP2: Remote-controlled Robot
// 제출자: 2020130015 노진영
// 제출일: 2024.12.15
// 과제 개요
// 개의원격제어기에서USART통신으로제어하는이동로봇구현- 원격제어기1:PC(MFC프로그램)- 원격제어기2:Mobilephone(LightBlue 프로그램)- 이동로봇:F407 보드(user control program)
// 원격제어기의역할 : 이동로봇에이동 / 정지 / 회전명령을전송하고, 이동
// 로봇으로부터로봇상태(이동 / 정지 / 회전상태)를수신하여화면에표시함.
// 이동로봇의역할 : 원격제어기로부터이동 / 정지 / 회전명령을수신하고 실행(LCD, LED, Buzzer 작동)함.로봇상태(이동 / 정지 / 회전 상태)를 원격제어기에송신함.
//////////////////////////////////////////////////////////////////////


#include "stm32f4xx.h"
#include "GLCD.h"

#define MOVE 0x80
#define STOP 0x40
#define RIGHT 0x04
#define STRAIGHT 0x02
#define LEFT 0x01

void RunMenu(void);
void _GPIO_Init(void);
void USART1_Init(void);
void UART4_Init(void);
void USART1_BRR_Configuration(uint32_t USART_BaudRate);
void UART4_BRR_Configuration(uint32_t USART_BaudRate);
void DelayMS(unsigned short wMS);
void BEEP(void);
void _ADC_Init(void);
void SerialSendChar(uint8_t c);
void SerialSendString(char* s);
void SerialSendCharUART4(uint8_t c);
void SerialSendStringUART4(char* s);
void _EXTI_Init(void);
uint16_t KEY_Scan(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

void TIMER7_Init(void);
void DMAInit(void);
uint8_t current = 0x40;
uint8_t USART_value[1];
uint8_t Move_flag = 0;

unsigned short ADC_Value, Voltage;

int main(void)
{
	_GPIO_Init();
	USART1_Init();
	UART4_Init();
	_ADC_Init();
	_EXTI_Init();
	DMAInit();
	TIMER7_Init();
	LCD_Init();	// LCD 구동 함수
	DelayMS(100);	// LCD구동 딜레이
	GPIOG->ODR &= 0x00;// LED0~7 Off 
          
	//LCD 초기화면구동 함수
	RunMenu();
    
	while(1)
	{

			uint8_t ch;
			ch = USART_value[0]; // 수신된 문자 저장


			if (ch == MOVE) 		// 수신된 값이 0x80인지 확인
			{
				
				LCD_DisplayText(3, 7, "Move    "); // "move" 출력
				LCD_DisplayText(1, 7, "0x80");	// "0x80" 출력
				GPIOG->ODR |= 0x0080;
				BEEP();
				DelayMS(300);
				BEEP();
				Move_flag = 1;	// Move on 
				current = ch;	//pc 및 phone에 표시하기위해 데이터 저장
				USART_value[0] = 0;	// 수신된 명령 초기화
			}
			else if (ch == STOP)	// 수신된 값이 0x40인지 확인
			{
				LCD_DisplayText(3, 7, "Stop    "); // "Stop" 출력
				LCD_DisplayText(1, 7, "0x40");	// "0x40" 출력
				GPIOG->ODR &= 0x0000;
				BEEP();
				DelayMS(300);
				BEEP();
				DelayMS(300);
				BEEP();
				Move_flag = 0;	// Move off
				current = ch;	// pc 및 phone에 표시하기위해 데이터 저장
				USART_value[0] = 0;	// 수신된 명령 초기화


			}
			if (Move_flag == 1)		// Move on일 때
			{
				if (ch == RIGHT)	// 수신된 값이 0x04인지 확인
				{
					LCD_DisplayText(4, 7, "Right   "); // "Right" 출력
					LCD_DisplayText(1, 7, "0x04");	// "0x04" 출력
					GPIOG->ODR &= ~0x0007;
					GPIOG->ODR |= 0x0004;
					BEEP();
					current = ch;	// pc 및 phone에 표시하기위해 데이터 저장
					USART_value[0] = 0;	// 수신된 명령 초기화


				}
				else if (ch == STRAIGHT)	// 수신된 값이 0x02인지 확인
				{
					LCD_DisplayText(4, 7, "Straight"); // "Straight" 출력
					LCD_DisplayText(1, 7, "0x02");	// "0x02" 출력
					GPIOG->ODR &= ~0x0007;
					GPIOG->ODR |= 0x0002;
					BEEP();
					current = ch;	// pc 및 phone에 표시하기위해 데이터 저장
					USART_value[0] = 0;	// 수신된 명령 초기화
				}
				else if (ch == LEFT)	// 수신된 값이 0x01인지 확인
				{
					LCD_DisplayText(4, 7, "Left    "); // "Left" 출력
					LCD_DisplayText(1, 7, "0x01");	// "0x01" 출력
					GPIOG->ODR &= ~0x0007;
					GPIOG->ODR |= 0x0001;
					BEEP();
					current = ch;	// pc 및 phone에 표시하기위해 데이터 저장
					USART_value[0] = 0;	// 수신된 명령 초기화
				}
			}
			
		
             
	}
}


void _GPIO_Init(void)
{
	// LED (GPIO G) 설정
    RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
 	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
    
	// SW (GPIO H) 설정 
	RCC->AHB1ENR	|=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 
	RCC->AHB1ENR	|=  0x00000020;	// RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER 	|=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
}	

void _EXTI_Init(void)    //EXTI11(PH11,SW3)
{
	RCC->AHB1ENR |= (1 << 7); 	// 0x80, RCC_AHB1ENR GPIOH Enable
	RCC->APB2ENR |= (1 << 14);	// 0x4000, Enable System Configuration Controller Clock

	SYSCFG->EXTICR[2] |= (7 << 12);	// 0x7000, EXTI11에 대한 소스 입력은 GPIOH로 설정 (EXTICR3) (reset value: 0x0000)	

	EXTI->FTSR |= (1 << 11);		// 0x000800, Falling Trigger Enable  (EXTI11:PH11)
}

void _ADC_Init(void)
{     	
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;	// GPIOA 클럭 활성화
	GPIOA->MODER |= (3 << 2 * 1);		// PA1: 아날로그 입력 모드 설정
						
	RCC->APB2ENR |= RCC_APB2ENR_ADC3EN;	// ADC3 클럭 활성화

	ADC->CCR &= ~0X0000001F;	// 독립 모드 (MULTI[4:0])
	ADC->CCR |= 0x00010000;		// 프리스케일러: Div4
	ADC->CCR &= ~0x0000C000;	// DMA 비활성화
	ADC->CCR |= 0x00000F00;		// 샘플링 지연: 20 사이클
        
	ADC3->CR1 |=  (2 << 24);	// 해상도: 8비트
	ADC3->CR1 &= ~(1 << 8);		// 스캔 모드 비활성화
	ADC3->CR1 |=  (1 << 5);		// 변환 완료 인터럽트 활성화

	ADC3->CR2 &= ~(1 << 1);		// 연속 변환 모드 비활성화
	ADC3->CR2 |=  (2 << 28);	// 외부 트리거 Falling Edge
	ADC3->CR2 |= (0x0F << 24);	// 트리거 소스: EXTI11

	ADC3->CR2 &= ~(1 << 11);	// 데이터 정렬: Right-aligned
	ADC3->CR2 &= ~(1 << 10);	// EOC 비트: 시퀀스 완료 시 설정

	ADC3->SQR1 &= ~(0xF << 20);	// 변환 시퀀스 길이: 1
	ADC3->SQR3 |= (1 << 0);		// 채널 1 (PA1)

	ADC3->SMPR2 |= (0x7 << (3 * 1));	// 샘플 시간: 480 사이클

	NVIC->ISER[0] |= (1 << 18);	// ADC3 인터럽트 활성화

	ADC3->CR2 |= (1 << 0);		// ADC3 활성화
}


void TIMER7_Init(void)
{
	// Enable Timer CLK 
	RCC->APB1ENR |= (1<<5);	// RCC_APB1ENR TIMER7 Enable

	// Setting CR1 : 0x0000 
	TIM7->CR1 &= ~(1<<4);	// DIR=0(Up counter)(reset state)
	TIM7->CR1 &= ~(1 << 1);	// UDIS=0(Update event Enabled): By one of following events
	//   - Counter Overflow/Underflow, 
	//   - Setting the UG bit Set,
	//   - Update Generation through the slave mode controller 
	// UDIS=1 : Only Update event Enabled by Counter Overflow/Underflow,
	TIM7->CR1 &= ~(1 << 2);	// URS=0(Update Request Source  Selection): By one of following events
	//   - Counter Overflow/Underflow, 
	//   - Setting the UG bit Set,
	//   - Update Generation through the slave mode controller 
	// URS=1 : Only Update Interrupt generated By Counter Overflow/Underflow,
	TIM7->CR1 &= ~(1 << 3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM7->CR1 &= ~(1 << 7);	// ARPE=0(ARR is NOT buffered) (reset state)
	TIM7->CR1 &= ~(3 << 8); 	// CKD(Clock division)=00(reset state)
	TIM7->CR1 &= ~(3 << 5); 	// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)
	// Center-aligned mode: The counter counts UP and DOWN alternatively

// Deciding the Period
	TIM7->PSC = 8400 - 1;	// Prescaler 84,000,000Hz/8400 = 10,000 Hz (0.1ms)  (1~65536)
	TIM7->ARR = 20000 - 1;		// Auto reload  0.1ms * 20000= 2000ms  
// Clear the Counter
	TIM7->EGR |= (1 << 0);	// UG(Update generation)=1 
	// Re-initialize the counter(CNT=0) & generates an update of registers   

// Setting an UI(UEV) Interrupt 
	NVIC->ISER[1] |= (1 << 23); 	// Enable Timer7 global Interrupt
	TIM7->DIER |= (1 << 0);	// Enable the Tim7 Update interrupt

	TIM7->CR1 |= (1 << 0);	// Enable the Tim7 Counter (clock enable)   

}


void USART1_Init(void)
{
	RCC->APB2ENR |=	0x0010;	// RCC_APB2ENR USART1 Enable
  
	//TX(PA9)
	RCC->AHB1ENR |= 0x01;	// RCC_AHB1ENR GPIOA Enable
	GPIOA->MODER |= 0x00080000;// GPIOA PIN9 Output Alternate function mode					
	GPIOA->OSPEEDR |= 0x000C0000;// GPIOA PIN9 Output speed (100MHz Very High speed)
	GPIOA->OTYPER |= 0x00000000;// GPIOA PIN9 Output type push-pull (reset state)
	GPIOA->PUPDR |= 0x00040000;// GPIOA PIN9 Pull-up
	GPIOA->AFR[1] |= 0x70;	//Connect GPIOA pin9 to AF7(USART1)
    
	//RX(PA10)
	RCC->AHB1ENR |= 0x01;	// RCC_AHB1ENR GPIOA Enable    				   
	GPIOA->MODER |= 0x200000;// GPIOA PIN10 Output Alternate function mode
	GPIOA->OSPEEDR |= 0x00300000;// GPIOA PIN10 Output speed (100MHz Very High speed
	GPIOA->AFR[1]	|= 0x700;//Connect GPIOA pin10 to AF7(USART1)
    
	USART1_BRR_Configuration(9600);	// USART Baudrate Configuration
    
	USART1->CR1 |= 0x0000;	// USART_WordLength 8 Data bit
	USART1->CR1 |= 0x0000;	// USART_Parity_No
	USART1->CR1 |= 0x0004;	// USART_Mode_RX Enable
	USART1->CR1 |= 0x0008;	// USART_Mode_Tx Enable
	USART1->CR2 |= 0x0000;	// USART_StopBits_1
	USART1->CR3 |= 0x0000;	// USART_HardwareFlowControl_None;
	USART1->CR3 |= (1<<6);	// DMA enable receiver
    
	USART1->CR1 |=	0x2000;	// USART1 Enable   
}

void UART4_Init(void)
{
	// UART4 : TX(PC10)
	RCC->AHB1ENR |= (1 << 2);	// RCC_AHB1ENR GPIOC Enable
	GPIOC->MODER |= (2 << 2 * 10);	// GPIOC PIN10 Output Alternate function mode					
	GPIOC->OSPEEDR |= (3 << 2 * 10);	// GPIOC PIN10 Output speed (100MHz Very High speed)
	GPIOC->AFR[1] |= (8 << 4 * (10 - 8));// Connect GPIOC pin10 to AF8(USART1)

	// UART4 : RX(PC11)
	GPIOC->MODER |= (2 << 2 * 11);	// GPIOC PIN11 Output Alternate function mode
	GPIOC->OSPEEDR |= (3 << 2 * 11);	// GPIOC PIN11 Output speed (100MHz Very High speed
	GPIOC->AFR[1] |= (8 << 4 * (11 - 8));// Connect GPIOC pin11 to AF8(USART1)

	// BT RESET (PC13) : GPIO
	GPIOC->MODER |= (1 << 2 * 13);	// GPIOC PIN13 Output mode
	GPIOC->OSPEEDR |= (3 << 2 * 13);
	GPIOC->ODR |= (1 << 13);	// BT Reset

	RCC->APB1ENR |= (1 << 19);	// RCC_APB1ENR UART4 Enable

	UART4_BRR_Configuration(9600); // USART Baud rate Configuration

	UART4->CR1 &= ~(1 << 12);	// USART_WordLength 8 Data bit
	UART4->CR1 &= ~(1 << 10);	// NO USART_Parity

	UART4->CR1 |= (1 << 2);	// 0x0004, USART_Mode_RX Enable
	UART4->CR1 |= (1 << 3);	// 0x0008, USART_Mode_Tx Enable
	UART4->CR2 &= ~(3 << 12);	// 0b00, USART_StopBits_1
	UART4->CR3 = 0x0000;	// No HardwareFlowControl, No DMA

	UART4->CR1 |= (1 << 5);	// 0x0020, RXNE interrupt Enable
	NVIC->ISER[1] |= (1 << (52 - 32));// Enable Interrupt USART1 (NVIC 52번)
	UART4->CR1 |= (1 << 13);	//  0x2000, USART1 Enable
}

void DMAInit(void)
{
	// DMA2 Stream2 channe4 configuration *************************************
	RCC->AHB1ENR |= (1<<22);			//DMA2 clock enable
	DMA2_Stream2->CR   |= (4<<25);	//DMA2 Stream2 channel 4 selected  100 
	DMA2_Stream2->PAR |= (uint32_t)&USART1->DR;    //Peripheral add   ress - ADC1 Regular data Address
	DMA2_Stream2->M0AR |= (uint32_t)USART_value;  //Memory address - ADC1 Value
	DMA2_Stream2->NDTR = 1;		//DMA_BufferSize = 1
	  
	DMA2_Stream2->CR &= ~(3<<6);	//Data transfer direction : Peripheral-to-memory
	DMA2_Stream2->CR &= ~(1<<9);	//Peripheral increment mode - Peripheral address pointer is fixed
	DMA2_Stream2->CR |= (1<<10);	//Memory increment mode - Memory address pointer is incremented after each data transferd 
	  
	DMA2_Stream2->CR &= ~(3<<11);	//Peripheral data size -byte(8bit) 
	DMA2_Stream2->CR &= ~(3<<13);	//Memory data size - byte(8bit) peripheral data size와 동일
	DMA2_Stream2->CR |= (1<<8);	//Circular mode enabled   
	DMA2_Stream2->CR |= (2<<16);	//Priority level - High

	DMA2_Stream2->FCR &= ~(1<<2);	//DMA_FIFO_direct mode enabled

	DMA2_Stream2->CR &= ~(3<<23);	//Memory burst transfer configuration - single transfer
	DMA2_Stream2->CR &= ~(3<<21);	//Peripheral burst transfer configuration - single transfer  
	DMA2_Stream2->CR |= (1<<0);	//DMA2_Stream2 enabled

}

void ADC_IRQHandler(void)
{
	
		ADC3->SR &= ~(1 << 1);		// EOC flag clear

		ADC_Value = ADC3->DR;		// Reading ADC result

		Voltage = ADC_Value * (3.3 * 100) / 255;   // 3.3 : 255 =  Volatge : ADC_Value 
		// 100:  소수점아래 두자리까지 표시하기 위한 값  
		LCD_DisplayChar(5, 7, Voltage / 100 + 0x30);
		LCD_DisplayChar(5, 8, '.');
		LCD_DisplayChar(5, 9, Voltage % 100 / 10 + 0x30);

		char ch[5];
		sprintf(ch, "%d.%dV ", Voltage / 100, Voltage % 100 / 10);	//ADC전압값 저장
		SerialSendString(ch);	//ADC전압값 표시
	
	// NO SWSTART !!!
}
void UART4_IRQHandler(void)
{
	if ((UART4->SR & USART_SR_RXNE)) // USART_SR_RXNE= 1? RX Buffer Full?
		// #define  USART_SR_RXNE ((uint16_t)0x0020)    //  Read Data Register Not Empty(FULL)     
	{
		char ch;
		ch = UART4->DR;	// 수신된 문자 저장
		if (ch == MOVE)	// 수신된 값이 0x80인지 확인
		{
			LCD_DisplayText(3, 7, "Move    "); // "Move" 출력
			LCD_DisplayText(2, 7, "0x80"); // "0x80" 출력
			GPIOG->ODR |= 0x0080;
			BEEP();
			DelayMS(300);
			BEEP();
			Move_flag = 1;	//move on
			current = ch;	// pc 및 phone에 표시하기위해 데이터 저장
			UART4->DR = 0;	//수신된 명령 초기화
		}
		else if (ch == STOP)	// 수신된 값이 0x40인지 확인
		{
			LCD_DisplayText(3, 7, "Stop    "); // "Stop" 출력
			LCD_DisplayText(2, 7, "0x40");	// "0x40" 출력
			GPIOG->ODR &= 0x0000;
			BEEP();
			DelayMS(300);
			BEEP();
			DelayMS(300);
			BEEP();
			Move_flag = 0;	//move off
			current = ch;	//pc 및 phone에 표시하기위해 데이터 저장
			UART4->DR = 0;	//수신된 명령 초기화
		}
		if (Move_flag == 1)	//move on일때
		{
			if (ch == RIGHT)	// 수신된 값이 0x04인지 확인
			{
				LCD_DisplayText(4, 7, "Right   "); // "Right" 출력
				LCD_DisplayText(2, 7, "0x04");	// "0x04" 출력
				GPIOG->ODR &= ~0x0007;
				GPIOG->ODR |= 0x0004;
				BEEP();
				current = ch;	//pc 및 phone에 표시하기위해 데이터 저장
				UART4->DR = 0;	//수신된 명령 초기화


			}
			else if (ch == STRAIGHT)	// 수신된 값이 0x02인지 확인
			{
				LCD_DisplayText(4, 7, "Straight"); // "Straight" 출력
				LCD_DisplayText(2, 7, "0x02");	// "0x02" 출력
				GPIOG->ODR &= ~0x0007;
				GPIOG->ODR |= 0x0002;
				BEEP();
				current = ch;	// pc 및 phone에 표시하기위해 데이터 저장
				UART4->DR = 0;	//수신된 명령 초기화


			}
			else if (ch == LEFT)	// 수신된 값이 0x01인지 확인
			{
				LCD_DisplayText(4, 7, "Left    "); // "Left" 출력
				LCD_DisplayText(2, 7, "0x01");	// "0x01" 출력
				GPIOG->ODR &= ~0x0007;
				GPIOG->ODR |= 0x0001;
				BEEP();
				current = ch;	// pc 및 phone에 표시하기위해 데이터 저장
				UART4->DR = 0;	//수신된 명령 초기화


			}
		}
		


	}
	// DR 을 읽으면 SR.RXNE bit(flag bit)는 clear 된다. 즉 clear 할 필요없음 
}
void TIM7_IRQHandler(void)  	// 2000ms Interrupt
{

	if (TIM7->SR & (1 << 0))  // Check update interrupt flag
	{
		TIM7->SR &= ~(1 << 0);  // Clear interrupt flag
		
		switch (current)
		{
		case MOVE:
			SerialSendString("Move ");	//pc에 Move 출력
			SerialSendStringUART4("Move ");	//phone에 Move 출력
			break;

		case STOP:
			SerialSendString("Stop ");	//pc에 Stop 출력
			SerialSendStringUART4("Stop ");	//phone에 Stop 출력
			break;

		case RIGHT:
			SerialSendString("Right ");	//pc에 Right 출력
			SerialSendStringUART4("Right "); //phone에 Right 출력
			break;

		case STRAIGHT:
			SerialSendString("Straight ");	//pc에 Straight 출력
			SerialSendStringUART4("Straight ");	//phone에 Straight 출력
			break;

		case LEFT:
			SerialSendString("Left ");	//pc에 Left 출력
			SerialSendStringUART4("Left");	//phone에 Left 출력
			break;

		default:
			break;
		}
		

	}
}
void DelayMS(unsigned short wMS)
{
	register unsigned short i;

	for (i=0; i<wMS; i++)
		DelayUS(1000);	// 1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
	volatile int Dly = (int)wUS*17;
	for(; Dly; Dly--);
}



void RunMenu(void)
{
	LCD_Clear(GET_RGB(128,128,128));	//배경색(GRAY)
	LCD_SetFont(&Gulim8);		//폰트 
	LCD_SetBackColor(RGB_YELLOW);	//글자배경색
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayText(0, 0, "RC Robot(NJY)");
	LCD_SetBackColor(GET_RGB(128, 128, 128));	//글자배경색(GRAY)
	LCD_DisplayText(1, 0, "Rx(PC):");
	LCD_DisplayText(2, 0, "Rx(MP):");
	LCD_DisplayText(3, 0, "Motion:");
	LCD_DisplayText(4, 0, "Turn:");
	LCD_DisplayText(5, 0, "ADC(V):");

	LCD_SetTextColor(RGB_BLUE);	
	LCD_DisplayText(1, 7, "0x40");
	LCD_DisplayText(2, 7, "0x40");
	LCD_DisplayText(3, 7, "Stop");
	LCD_DisplayText(4, 7, "Straight");
	LCD_DisplayText(5, 7, "0.0");



}

void SerialSendChar(uint8_t Ch) // 1문자 보내기 함수
{
	// USART_SR_TXE(1<<7)=0?, TX Buffer NOT Empty? 
	// TX buffer Empty되지 않으면 계속 대기(송신 가능한 상태까지 대기)
	while ((USART1->SR & USART_SR_TXE) == RESET);
	USART1->DR = (Ch & 0x01FF);	// 전송 (최대 9bit 이므로 0x01FF과 masking)
}

void SerialSendString(char* str) // 여러문자 보내기 함수
{
	while (*str != '\0') // 종결문자가 나오기 전까지 구동, 종결문자가 나온후에도 구동시 메모리 오류 발생가능성 있음.
	{
		SerialSendChar(*str);	// 포인터가 가르키는 곳의 데이터를 송신
		str++; 			// 포인터 수치 증가
	}
}

void SerialSendCharUART4(uint8_t Ch) // UART4로 1문자 보내기 함수
{
	while ((UART4->SR & USART_SR_TXE) == RESET); // TXE 플래그가 설정될 때까지 대기
	UART4->DR = (Ch & 0x01FF); // 전송 (최대 9bit 이므로 0x01FF로 마스킹)
}

void SerialSendStringUART4(char* str) // UART4로 여러 문자 보내기 함수
{
	while (*str != '\0') // 문자열 끝(NULL 문자)까지 반복
	{
		SerialSendCharUART4(*str); // 현재 문자 전송
		str++; // 다음 문자로 이동
	}
}

void USART1_BRR_Configuration(uint32_t USART_BaudRate)
{ 
	uint32_t tmpreg = 0x00;
	uint32_t apbclock = 84000000;	//PCLK2_Frequency
	uint32_t integerdivider = 0x00;
	uint32_t fractionaldivider = 0x00;

	/* Determine the integer part */
	if ((USART1->CR1 & 0x8000) != 0)
	{
		/* Integer part computing in case Oversampling mode is 8 Samples */
		integerdivider = ((25 * apbclock) / (2 * (USART_BaudRate)));    
	}
	else /* if ((USARTx->CR1 & USART_CR1_OVER8) == 0) */
	{
		/* Integer part computing in case Oversampling mode is 16 Samples */
		integerdivider = ((25 * apbclock) / (4 * (USART_BaudRate)));    
	}
	tmpreg = (integerdivider / 100) << 4;
  
	/* Determine the fractional part */
	fractionaldivider = integerdivider - (100 * (tmpreg >> 4));

	/* Implement the fractional part in the register */
	if ((USART1->CR1 & USART_CR1_OVER8) != 0)
	{
		tmpreg |= ((((fractionaldivider * 8) + 50) / 100)) & ((uint8_t)0x07);
	}
	else /* if ((USARTx->CR1 & USART_CR1_OVER8) == 0) */
	{
		tmpreg |= ((((fractionaldivider * 16) + 50) / 100)) & ((uint8_t)0x0F);
	}
	/* Write to USART BRR register */
	USART1->BRR = (uint16_t)tmpreg;
    
}

void UART4_BRR_Configuration(uint32_t USART_BaudRate) 
{
	uint32_t tmpreg = 0x00;
	uint32_t APB1clock = 42000000;	//PCLK2_Frequency
	uint32_t integerdivider = 0x00;
	uint32_t fractionaldivider = 0x00;

	// Find the integer part 
	if ((UART4->CR1 & USART_CR1_OVER8) != 0) // USART_CR1_OVER8=(1<<15)
		//  #define  USART_CR1_OVER8 ((uint16_t)0x8000) // USART Oversampling by 8 enable   
	{       // UART4->CR1.OVER8 = 1 (8 oversampling)
		// Computing 'Integer part' when the oversampling mode is 8 Samples 
		integerdivider = ((25 * APB1clock) / (2 * USART_BaudRate));  // 공식에 100을 곱한 곳임(소수점 두번째자리까지 유지하기 위함)  
	}
	else  // USART1->CR1.OVER8 = 0 (16 oversampling)
	{	// Computing 'Integer part' when the oversampling mode is 16 Samples 
		integerdivider = ((25 * APB1clock) / (4 * USART_BaudRate));  // 공식에 100을 곱한 곳임(소수점 두번째자리까지 유지하기 위함)    
	}
	tmpreg = (integerdivider / 100) << 4;

	// Find the fractional part 
	fractionaldivider = integerdivider - (100 * (tmpreg >> 4));

	// Implement the fractional part in the register 
	if ((UART4->CR1 & USART_CR1_OVER8) != 0)
	{	// 8 oversampling
		tmpreg |= (((fractionaldivider * 8) + 50) / 100) & (0x07);
	}
	else	// 16 oversampling
	{
		tmpreg |= (((fractionaldivider * 16) + 50) / 100) & (0x0F);
	}

	// Write to USART BRR register
	UART4->BRR = (uint16_t)tmpreg;
}


uint8_t key_flag = 0;
uint16_t KEY_Scan(void)	// input key SW0 - SW7 
{ 
	uint16_t key;
	key = GPIOH->IDR & 0xFF00;	// any key pressed ?
	if(key == 0xFF00)		// if no key, check key off
	{  	if(key_flag == 0)
        			return key;
		else
		{	DelayMS(10);
			key_flag = 0;
			return key;
		}
	}
  	else				// if key input, check continuous key
	{	if(key_flag != 0)	// if continuous key, treat as no key input
            			return 0xFF00;
        		else			// if new key,delay for debounce
        		{	key_flag = 1;
			DelayMS(10);
			return key;
		}
	}
}


void BEEP(void)			// Beep for 20 ms 
{
	GPIOF->ODR |= (1 << 9);	// PF9 'H' Buzzer on
	DelayMS(20);		// Delay 20 ms
	GPIOF->ODR &= ~(1 << 9);	// PF9 'L' Buzzer off
}
