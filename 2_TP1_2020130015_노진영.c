//////////////////////////////////////////////////////////////////////
// TP1: Smart Watch
// 제출자: 2020130015 노진영
// 제출일: 2024.12.12
// 과제 개요
// 1. 3개의 기능(화면)을 갖는 smart watch를 제작
// 2. 각 기능(화면)은 스위치(sw7)를 입력하면 전환
// 3. 초기 화면은 'Alarm', 두번째 화면은 'Ball game, 세번째 화면은 'Thermostat'
// 4. 각 화면에는 동일한 시계(시간)가 오른쪽 상단에 항상 표시
// 5. 화면이 바뀌어도 설정된 알람 울림
//////////////////////////////////////////////////////////////////////

#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"
#include "ACC.h"
#define SW0_PUSH        0xFE00  //PH8
#define SW1_PUSH        0xFD00  //PH9
#define SW2_PUSH        0xFB00  //PH10
#define SW3_PUSH        0xF700  //PH11
#define SW4_PUSH        0xEF00  //PH12
#define SW5_PUSH        0xDF00  //PH13
#define SW6_PUSH        0xBF00  //PH14
#define SW7_PUSH        0x7F00  //PH15

void DisplayTitle(void);
void _GPIO_Init(void);
uint16_t KEY_Scan(void);
void TIMER6_Init(void);
void TIMER2_Init(void);
void TIMER3_PWM_Init(void);
void _ADC_Init(void);
void _EXTI_Init(void);
void USART1_Init(void);
void USART_BRR_Configuration(uint32_t USART_BaudRate);
void SerialSendChar(uint8_t c);
void SerialSendString(char* s);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void BEEP(void);
void BEEP_3(void);
void Display_Change(void);

void SPI1_Init(void);
void Ball_Process(int16* pBuf);
void Display_Process(int16* pBuf);
void SPI1_Process(int16* pBuf);  // ACC.c (ACC.h) 
void ACC_Init(void); // ACC.c (ACC.h)
void TIMER13_Init(void);

uint8_t Alarm_minute, Alarm_hour, hour_cnt, minute_cnt = 0;

uint8_t mode = 1; 
uint8_t data;
uint16_t ADC_Value, Voltage;
uint8_t Ball_X = 48; //초기 공위치
uint8_t Ball_Y = 62;
uint8_t Alarm_flag = 0; // 알람 준비
int Thermostat;

UINT8 bControl;
int main(void)
{
    int16 buffer[3];

    LCD_Init();	// LCD 구동 함수
    DelayMS(500);	// LCD구동 딜레이

    _GPIO_Init();
    USART1_Init();
    _ADC_Init();
    _EXTI_Init();

    SPI1_Init();        	// SPI1 초기화
    ACC_Init();		// 가속도센서 초기화


    GPIOG->ODR &= 0x00;	// LED0~7 Off 

    Fram_Init();            // FRAM 초기화 H/W 초기화
    Fram_Status_Config();   // FRAM 초기화 S/W 초기화

   
    DisplayTitle();	//LCD 초기화면구동 함수

    TIMER2_Init();
    TIMER6_Init();
    TIMER3_PWM_Init();
    TIMER13_Init();		// 가속도센서 스캔 주기 생성

    while (1)
    {

        
       
        if (mode == 2)
        {
            if (bControl)
            {
                bControl = FALSE;
                SPI1_Process(&buffer[0]);	// SPI통신을 이용하여 가속도센서 측정
                Display_Process(&buffer[0]);	// 측정값을 LCD에 표시
                Ball_Process(&buffer[0]); // 공 위치 업데이트
            }
        }
       

        if (Alarm_hour == hour_cnt && minute_cnt == 1 && Alarm_flag == 0)
        {
            BEEP();
            DelayMS(500);
            BEEP();
            DelayMS(500);
            BEEP();
            Alarm_flag = 1; //알람안울림
        }
    
       
    }
}
void SPI1_Init(void)
{
    /*!< Clock Enable  *********************************************************/
    RCC->APB2ENR |= (1 << 12);	// 0x1000, SPI1 Clock EN
    RCC->AHB1ENR |= (1 << 0);	// 0x0001, GPIOA Clock EN		

    /*!< SPI1 pins configuration ************************************************/

    /*!< SPI1 NSS pin(PA8) configuration : GPIO 핀  */
    GPIOA->MODER |= (1 << (2 * 8));	// 0x00010000, PA8 Output mode
    GPIOA->OTYPER &= ~(1 << 8); 	// 0x0100, push-pull(reset state)
    GPIOA->OSPEEDR |= (3 << (2 * 8));	// 0x00030000, PA8 Output speed (100MHZ) 
    GPIOA->PUPDR &= ~(3 << (2 * 8));	// 0x00030000, NO Pullup Pulldown(reset state)

    /*!< SPI1 SCK pin(PA5) configuration : SPI1_SCK */
    GPIOA->MODER |= (2 << (2 * 5)); 	// 0x00000800, PA5 Alternate function mode
    GPIOA->OTYPER &= ~(1 << 5); 	// 0020, PA5 Output type push-pull (reset state)
    GPIOA->OSPEEDR |= (3 << (2 * 5));	// 0x00000C00, PA5 Output speed (100MHz)
    GPIOA->PUPDR |= (2 << (2 * 5)); 	// 0x00000800, PA5 Pull-down
    GPIOA->AFR[0] |= (5 << (4 * 5));	// 0x00500000, Connect PA5 to AF5(SPI1)

    /*!< SPI1 MOSI pin(PA7) configuration : SPI1_MOSI */
    GPIOA->MODER |= (2 << (2 * 7));	// 0x00008000, PA7 Alternate function mode
    GPIOA->OTYPER &= ~(1 << 7);	// 0x0080, PA7 Output type push-pull (reset state)
    GPIOA->OSPEEDR |= (3 << (2 * 7));	// 0x0000C000, PA7 Output speed (100MHz)
    GPIOA->PUPDR |= (2 << (2 * 7)); 	// 0x00008000, PA7 Pull-down
    GPIOA->AFR[0] |= (5 << (4 * 7));	// 0x50000000, Connect PA7 to AF5(SPI1)

    /*!< SPI1 MISO pin(PA6) configuration : SPI1_MISO */
    GPIOA->MODER |= (2 << (2 * 6));	// 0x00002000, PA6 Alternate function mode
    GPIOA->OTYPER &= ~(1 << 6);	// 0x0040, PA6 Output type push-pull (reset state)
    GPIOA->OSPEEDR |= (3 << (2 * 6));	// 0x00003000, PA6 Output speed (100MHz)
    GPIOA->PUPDR |= (2 << (2 * 6));	// 0x00002000, PA6 Pull-down
    GPIOA->AFR[0] |= (5 << (4 * 6));	// 0x05000000, Connect PA6 to AF5(SPI1)

    // Init SPI1 Registers 
    SPI1->CR1 |= (1 << 2);	// MSTR(Master selection)=1, Master mode
    SPI1->CR1 &= ~(1 << 15);	// SPI_Direction_2 Lines_FullDuplex
    SPI1->CR1 &= ~(1 << 11);	// SPI_DataSize_8bit
    SPI1->CR1 |= (1 << 9);  	// SSM(Software slave management)=1, 
    // NSS 핀 상태가 코딩에 의해 결정
    SPI1->CR1 |= (1 << 8);	// SSI(Internal_slave_select)=1,
    // 현재 MCU가 Master이므로 NSS 상태는 'High' 
    SPI1->CR1 &= ~(1 << 7);	// LSBFirst=0, MSB transmitted first    
    SPI1->CR1 |= (4 << 3);	// BR(BaudRate)=0b100, fPCLK/32 (84MHz/32 = 2.625MHz)
    SPI1->CR1 |= (1 << 1);	// CPOL(Clock polarity)=1, CK is 'High' when idle
    SPI1->CR1 |= (1 << 0);	// CPHA(Clock phase)=1, 두 번째 edge 에서 데이터가 샘플링

    SPI1->CR1 |= (1 << 6);	// SPE=1, SPI1 Enable 
}
void TIMER13_Init(void)	// 가속도센서 측정 주기 생성: 150ms(PF8)
{
    RCC->AHB1ENR |= (1 << 5);	// GPIOF CLOCK Enable
    RCC->APB1ENR |= (1 << 7);	// TIMER13 Clock Enable

    GPIOF->MODER |= (2 << 16);	// 0x00020000 PB8 Output Alternate function mode					
    GPIOF->OSPEEDR |= (3 << 16);	// 0x00030000 PB8 Output speed (100MHz High speed)
    GPIOF->OTYPER &= ~(1 << 8);	// PB8 Output type push-pull (reset state)
    GPIOF->PUPDR |= (1 << 16);	// 0x00010000 PB8 Pull-up
    GPIOF->AFR[1] |= (9 << 0);	// 0x00000002 (AFR[1].(3~0)=0b0010): Connect TIM4 pins(PB8) to AF2(TIM3..5)

    TIM13->PSC = 8400 - 1;	// Prescaler 84MHz/8400 = 10KHz (0.1ms)  
    TIM13->ARR = 1500 - 1;	// Auto reload  0.1ms * 1500 = 150ms

    TIM13->CR1 &= ~(1 << 4);	// DIR=0(Up counter)(reset state)
    TIM13->CR1 &= ~(1 << 1);	// UDIS=0(Update event Enabled)
    TIM13->CR1 &= ~(1 << 2);	// URS=0(Update event source Selection)g events
    TIM13->CR1 &= ~(1 << 3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
    TIM13->CR1 |= (1 << 7);	// ARPE=1(ARR is buffered): ARR Preload Enable 
    TIM13->CR1 &= ~(3 << 8); 	// CKD(Clock division)=00(reset state)
    TIM13->CR1 &= ~(3 << 5); 	// CMS(Center-aligned mode Sel)=00 : Edge-aligned mode(reset state)

    TIM13->CCER |= (1 << 0);	// CC1E=1: OC1 Active(Capture/Compare 3 output enable)
    // 해당핀(167번)을 통해 신호출력
    TIM13->CCER &= ~(1 << 1);	// CC1P=0: CC1 Output Polarity (OCPolarity_High : OC1으로 반전없이 출력)

    TIM13->CCR1 = 1000;		// CCR1 value
    TIM13->CCMR1 &= ~(3 << 0); // CC3S(CC3 channel)= '0b00' : Output 
    TIM13->CCMR1 |= (1 << 3); 	// OC1PE=1: Output Compare 3 preload Enable
    TIM13->DIER |= (1 << 1);	// CC1IE: Enable the Tim13 CC1 interrupt
    TIM13->CCMR1 |= (7 << 4);   //duty 값 opposite 
    TIM13->CCMR1 |= (1 << 7);	// OC1CE=1: Output compare 3 Clear enable
    NVIC->ISER[1] |= (1 << 12);
    TIM13->CR1 |= (1 << 0);	// Enable Tim13 Counter
}
void TIM8_UP_TIM13_IRQHandler(void)	// 250ms int
{

    if (TIM13->SR & (1 << 1)) // CC1 인터럽트 플래그 확인
    {
        TIM13->SR &= ~(1 << 1); // CC1 인터럽트 플래그 클리어
        bControl = TRUE;        // 데이터 수신 및 처리 플래그
    }
    
}
void Display_Process(int16* pBuf)
{
    //char str[10];
    UINT16 G_VALUE;
    LCD_SetFont(&Gulim7);
    LCD_SetTextColor(RGB_BLACK);
    // X 축 가속도 표시		
    if (pBuf[0] < 0)  //음수
    {
        G_VALUE = abs(pBuf[0]);
        LCD_DisplayChar(2, 21, '-'); // g 부호 표시
    }
    else				// 양수
    {
        G_VALUE = pBuf[0];
        LCD_DisplayChar(2, 21, '+'); // g 부호 표시
    }
    
    G_VALUE = 100 * G_VALUE / 0x4009; // 가속도 --> g 변환
    LCD_DisplayChar(2, 22, G_VALUE / 100 + 0x30);
    LCD_DisplayChar(2, 23, '.');
    LCD_DisplayChar(2, 24, G_VALUE % 100 / 10 + 0x30);
    LCD_DisplayChar(2, 25, 'g');

    // Y 축 가속도 표시	
    if (pBuf[1] < 0)  //음수
    {
        G_VALUE = abs(pBuf[1]);
        LCD_DisplayChar(3, 21, '-'); // g 부호 표시
    }
    else				// 양수
    {
        G_VALUE = pBuf[1];
        LCD_DisplayChar(3, 21, '+'); // g 부호 표시
    }
    
    G_VALUE = 100 * G_VALUE / 0x4009;
    LCD_DisplayChar(3, 22, G_VALUE / 100 + 0x30);
    LCD_DisplayChar(3, 23, '.');
    LCD_DisplayChar(3, 24, G_VALUE % 100 / 10 + 0x30);
    LCD_DisplayChar(3, 25, 'g');

    
}

void Ball_Process(int16* pBuf)
{
    int16_t Ax = pBuf[0]; // X축 가속도 값
    int16_t Ay = pBuf[1]; // Y축 가속도 값

    // 가속도 값에 따라 이동 속도 설정
    uint8_t Speed_X = abs(Ax) / 1000; // X축 이동 속도
    uint8_t Speed_Y = abs(Ay) / 1000; // Y축 이동 속도

    // X축 이동 처리
    if (Ax > 0) // 오른쪽으로 이동
    {
        if (Ball_X + Speed_X + 7 <= 100) // 오른쪽 벽에 닿지 않도록 제한
            Ball_X += Speed_X;
        else
            Ball_X = 93; // 오른쪽 경계
    }
    else if (Ax < 0) // 왼쪽으로 이동
    {
        if (Ball_X - Speed_X >= 2) // 왼쪽 벽에 닿지 않도록 제한
            Ball_X -= Speed_X;
        else
            Ball_X = 3; // 왼쪽 경계
    }

    // Y축 이동 처리
    if (Ay > 0) // 아래로 이동
    {
        if (Ball_Y + Speed_Y + 7 <= 100) // 아래쪽 벽에 닿지 않도록 제한
            Ball_Y += Speed_Y;
        else
            Ball_Y = 107; // 아래쪽 경계
    }
    else if (Ay < 0) // 위로 이동
    {
        if (Ball_Y - Speed_Y >= 16) // 위쪽 벽에 닿지 않도록 제한
            Ball_Y -= Speed_Y;
        else
            Ball_Y = 17; // 위쪽 경계
    }
    
    // 공 그리기
    LCD_SetBrushColor(RGB_WHITE);
    LCD_DrawFillRect(3, 17, 99, 99); // 이전 공 위치 지우기

    LCD_SetPenColor(RGB_BLACK);
    LCD_DrawRectangle(2, 16, 100, 100); // 경기장 그리기

    LCD_SetPenColor(RGB_RED);
    LCD_DrawRectangle(Ball_X, Ball_Y, 8, 8); // 새로운 위치에 공 그리기
}



void _ADC_Init(void)
{   	// ADC2: PA1(pin 41)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;	// (1<<0) ENABLE GPIOA CLK (stm32f4xx.h 참조)
    GPIOA->MODER |= (3 << 2 * 1);		// CONFIG GPIOA PIN1(PA1) TO ANALOG IN MODE						
    RCC->APB2ENR |= (1 << 9);	// (1<<8) ENABLE ADC2 CLK (stm32f4xx.h 참조)
    ADC->CCR &= ~(0X1F << 0);		// MULTI[4:0]: ADC_Mode_Independent
    ADC->CCR |= (1 << 16); 		// 0x00010000 ADCPRE:ADC_Prescaler_Div4 (ADC MAX Clock 36MHz, 84Mhz(APB2)/4 = 21MHz)        
    ADC2->CR1 &= ~(3 << 24);		// RES[1:0]= 0x00 : 12bit Resolution
    ADC2->CR1 &= ~(1 << 8);		// SCAN=0 : ADC_ScanCovMode Disable
    ADC2->CR1 |= (1 << 5);		// EOCIE=1: Interrupt enable for EOC
    ADC2->CR2 &= ~(1 << 1);		// CONT=0: ADC_Continuous ConvMode Disable
    ADC2->CR2 |= (3 << 28);		// EXTEN[1:0]: ADC_ExternalTrigConvEdge_Enable(Falling Edge)
    ADC2->CR2 |= (5 << 24);	// Timer2 CC4 event
    ADC2->CR2 &= ~(1 << 11);		// ALIGN=0: ADC_DataAlign_Right
    ADC2->CR2 &= ~(1 << 10);		// EOCS=0: The EOC bit is set at the end of each sequence of regular conversions
    ADC2->SQR1 &= ~(0xF << 20);	// L[3:0]=0b0000: ADC Regular channel sequece length 
    // 0b0000:1 conversion)
    //Channel selection, The Conversion Sequence of PIN1(ADC1_CH1) is first, Config sequence Range is possible from 0 to 17
    ADC2->SQR3 |= (1 << 0);		// SQ1[4:0]=0b0001 : CH1
    ADC2->SMPR2 |= (0x7 << (3 * 1));	// ADC2_CH1 Sample TIme_480Cycles (3*Channel_1)
    //Channel selection, The Conversion Sequence of PIN1(ADC2_CH1) is first, Config sequence Range is possible from 0 to 17
    NVIC->ISER[0] |= (1 << 18);	// Enable ADC global Interrupt
    ADC2->CR2 |= (1 << 0);		// ADON: ADC ON
}

void TIMER3_PWM_Init(void)
{
 // TIM3 CH3 : PB0 
    // Clock Enable : GPIOB & TIMER3
    RCC->AHB1ENR |= (1 << 1);	// GPIOB CLOCK Enable
    RCC->APB1ENR |= (1 << 1);	// TIMER3 CLOCK Enable   
    // PB0을 출력설정하고 Alternate function(TIM3_CH3)으로 사용 선언 : PWM 출력
    GPIOB->MODER |= (2 << 0);	// PB3 Output Alternate function mode					
    GPIOB->OSPEEDR |= (3 << 0);	// PB3 Output speed (100MHz High speed)
    GPIOB->OTYPER &= ~(1 << 0);	// PB3 Output type push-pull (reset state)
    GPIOB->AFR[0] |= (2 << 0);	// (AFR[0].(3~0)=0b0010): Connect TIM3 pins(PB6) to AF2(TIM3..5) 
    // TIM3 Channel 3 : PWM 2 mode
    // Assign 'PWM Pulse Period'
    TIM3->PSC = 16800 - 1;  // Prescaler 설정: 84,000,000Hz / 16800 = 5,000 Hz (0.2ms) (범위: 1~65536)
    TIM3->ARR = 10000 - 1;  // Auto-reload 설정: 0.2ms * 10000 = 2초    
    // Setting CR1 : 0x0000 (Up counting)
    TIM3->CR1 &= ~(1 << 4);	// DIR=0(Up counter)(reset state)
    TIM3->CR1 &= ~(1 << 1);	// UDIS=0(Update event Enabled)
    TIM3->CR1 &= ~(1 << 2);	// URS=0(Update event source Selection)g events
    TIM3->CR1 &= ~(1 << 3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
    TIM3->CR1 |= (1 << 7);	// ARPE=1(ARR is buffered): ARR Preload Enable 
    TIM3->CR1 &= ~(3 << 8); 	// CKD(Clock division)=00(reset state)
    TIM3->CR1 &= ~(3 << 5); 	// CMS(Center-aligned mode Sel)=00 : Edge-aligned mode(reset state)  
    // Define the corresponding pin by 'Output'  
    // CCER(Capture/Compare Enable Register) : Enable "Channel 1" 
    
    TIM3->CCER &= ~(1 << 9);	// CC3P=0: CC3 Output Polarity (OCPolarity_High : OC3으로 반전없이 출력) 
    // Duty Ratio 
    TIM3->CCER |= (1 << 8);   // 채널 입출력 enable   
    TIM3->CCR3 = 500;		// CCR1 value  
    // 'Mode' Selection : Output mode, PWM 1
    TIM3->CCMR2 &= ~(3 << 0); // CC3S(CC3 channel)= '0b00' : Output 
    TIM3->CCMR2 |= (1 << 3); 	// OC3PE=1: Output Compare 1 preload Enable
    TIM3->CCMR2 |= (7 << 4);	// OC3M=0b110: Output compare 1 mode: PWM2 mode
    TIM3->CCMR2 |= (1 << 7);	// OC3CE=1: Output compare 1 Clear enable  
    TIM3->CR1 |= (1 << 0);	// CEN: Counter TIM3 Disable
}

void _EXTI_Init(void)
{
    RCC->AHB1ENR |= (1 << 7);	// RCC_AHB1ENR GPIOH Enable
    RCC->APB2ENR |= (1 << 14);	// Enable System Configuration Controller Clock				 
    SYSCFG->EXTICR[3] |= (7 << 12); 	// EXTI15에 대한 소스 입력은 GPIOH로 설정
    EXTI->FTSR |= (1 << 15);		// EXTI15: Falling Trigger Enable 
    NVIC->ISER[1] |= (1 << 8);	//EXTI15 Interrupt Enable
    EXTI->IMR |= (1 << 15);		//EXTI15 mask Enable
}

void USART1_Init(void)
{
    // USART1 : TX(PA9)
    RCC->AHB1ENR |= (1 << 0);	// RCC_AHB1ENR GPIOA Enable
    GPIOA->MODER |= (2 << 2 * 9);	// GPIOA PIN9 Output Alternate function mode					
    GPIOA->OSPEEDR |= (3 << 2 * 9);	// GPIOA PIN9 Output speed (100MHz Very High speed)
    GPIOA->AFR[1] |= (7 << 4);	// Connect GPIOA pin9 to AF7(USART1)

    // USART1 : RX(PA10)
    GPIOA->MODER |= (2 << 2 * 10);	// GPIOA PIN10 Output Alternate function mode
    GPIOA->OSPEEDR |= (3 << 2 * 10);	// GPIOA PIN10 Output speed (100MHz Very High speed
    GPIOA->AFR[1] |= (7 << 8);	// Connect GPIOA pin10 to AF7(USART1)

    RCC->APB2ENR |= (1 << 4);	// RCC_APB2ENR USART1 Enable

    USART_BRR_Configuration(9600); // USART Baud rate Configuration

    USART1->CR1 |= (1 << 12);	// USART_WordLength 8 Data bit
    USART1->CR1 |= (1 << 10);	// NO USART_Parity
    USART1->CR1 &= ~(1 << 9);	// NO USART_Parity

    USART1->CR1 |= (1 << 2);	// 0x0004, USART_Mode_RX Enable
    USART1->CR1 |= (1 << 3);	// 0x0008, USART_Mode_Tx Enable
    USART1->CR2 &= ~(3 << 12);	// 0b00, USART_StopBits_1
    USART1->CR3 = 0x0000;	// No HardwareFlowControl, No DMA

    USART1->CR1 |= (1 << 5);	// 0x0020, RXNE interrupt Enable
    NVIC->ISER[1] |= (1 << (37 - 32));// Enable Interrupt USART1 (NVIC 37번)
    USART1->CR1 |= (1 << 13);	//  0x2000, USART1 Enable
}

void _GPIO_Init(void)
{
    // LED (GPIO G) 설정
    RCC->AHB1ENR |= 0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
    GPIOG->MODER |= 0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
    GPIOG->OTYPER &= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
    GPIOG->OSPEEDR |= 0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 

    // SW (GPIO H) 설정 
    RCC->AHB1ENR |= 0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
    GPIOH->MODER &= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
    GPIOH->PUPDR &= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

    // Buzzer (GPIO F) 설정 
    RCC->AHB1ENR |= 0x00000020; // RCC_AHB1ENR : GPIOF(bit#5) Enable							
    GPIOF->MODER |= 0x00040000;	// GPIOF 9 : Output mode (0b01)						
    GPIOF->OTYPER &= ~0x0200;	// GPIOF 9 : Push-pull  	
    GPIOF->OSPEEDR |= 0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
}

void TIMER2_Init(void)
{
    // TIM2_CH4 (PA3) : 450ms 이벤트 발생
    // Clock Enable : GPIOA & TIMER3
    RCC->AHB1ENR |= (1 << 0);	// GPIOA Enable
    RCC->APB1ENR |= (1 << 0);	// TIMER2 Enable   
    // PA6을 출력설정하고 Alternate function(TIM2_CH4)으로 사용 선언 
    GPIOA->MODER |= (2 << 6);	// PA3 Output Alternate function mode					
    GPIOA->OSPEEDR |= (3 << 6);	// PA3 Output speed (100MHz High speed)
    GPIOA->OTYPER &= ~(1 << 3);	// PA6 Output type push-pull (reset state)
    GPIOA->AFR[0] |= (1 << 12); 	// Connect TIM2 pins(PA3) to AF1(TIM2)
    // Assign 'Interrupt Period' and 'Output Pulse Period'
    TIM2->PSC = 8400 - 1;	// Prescaler 84MHz/8400 = 10kHz
    TIM2->ARR = 4500 - 1;	// Auto reload  : 0.1ms * 4500 = 450ms(period) 
    // CR1 : Up counting
    TIM2->CR1 &= ~(1 << 4);	// DIR=0(Up counter)(reset state)
    TIM2->CR1 &= ~(1 << 1);	// UDIS=0(Update event Enabled): By one of following events
    //	- Counter Overflow/Underflow, 
    // 	- Setting the UG bit Set,
    //	- Update Generation through the slave mode controller 
    TIM2->CR1 &= ~(1 << 2);	// URS=0(Update event source Selection): one of following events
    //	- Counter Overflow/Underflow, 
    // 	- Setting the UG bit Set,
    //	- Update Generation through the slave mode controller 
    TIM2->CR1 &= ~(1 << 3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
    TIM2->CR1 &= ~(1 << 7);	// ARPE=0(ARR is NOT buffered) (reset state)
    TIM2->CR1 &= ~(3 << 8); 	// CKD(Clock division)=00(reset state)
    TIM2->CR1 &= ~(3 << 5); 	// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)
    // Center-aligned mode: The counter counts Up and DOWN alternatively  
    // Event & Interrup Enable : UI  
    TIM2->EGR |= (1 << 0);    // UG: Update generation      
    // Define the corresponding pin by 'Output'  
    TIM2->CCER |= (1 << 12);	// CC4E=1: CC1 channel Output Enable
    // OC1(TIM3_CH1) Active: 해당핀을 통해 신호출력
    TIM2->CCER &= ~(1 << 13);	// CC4P=0: CC4 channel Output Polarity (OCPolarity_High : OC4으로 반전없이 출력)    
    // 'Mode' Selection : Output mode, toggle  
    TIM2->CCMR2 &= ~(3 << 8); // CC4S(CC1 channel) = '0b00' : Output 
    TIM2->CCMR2 &= ~(1 << 11); // OC4P=0: Output Compare 1 preload disable
    TIM2->CCMR2 |= (3 << 12);	// OC4M: Output Compare 4 Mode : toggle
    TIM2->CCR4 = 3000;	// TIM2 CH4  
    ////////////////////////////////
    // Disable Tim2 CC4 interrupt
    TIM2->CR1 |= (1 << 0);	// CEN: Enable the Tim2 Counter 				
}


void TIMER6_Init(void)
{
    RCC->APB1ENR |= (1 << 4);	// RCC_APB1ENR TIMER6 Enable 
    // Setting CR1 : 0x0000 
    TIM6->CR1 &= ~(1 << 4);  // DIR=0(Up counter)(reset state)
    TIM6->CR1 &= ~(1 << 1);	// UDIS=0(Update event Enabled): By one of following events
    //  Counter Overflow/Underflow, 
    //  Setting the UG bit Set,
    //  Update Generation through the slave mode controller 
    // UDIS=1 : Only Update event Enabled by  Counter Overflow/Underflow,
    TIM6->CR1 &= ~(1 << 2);	// URS=0(Update Request Source  Selection):  By one of following events
    //	Counter Overflow/Underflow, 
    // Setting the UG bit Set,
    //	Update Generation through the slave mode controller 
    // URS=1 : Only Update Interrupt generated  By  Counter Overflow/Underflow,
    TIM6->CR1 &= ~(1 << 3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
    TIM6->CR1 &= ~(1 << 7);	// ARPE=0(ARR is NOT buffered) (reset state)
    TIM6->CR1 &= ~(3 << 8); 	// CKD(Clock division)=00(reset state)
    TIM6->CR1 &= ~(3 << 5); 	// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)
    // Center-aligned mode: The counter counts UP and DOWN alternatively  
    // Deciding the Period
    TIM6->PSC = 8400 - 1;	// Prescaler 84,000,000Hz/8400 = 10,000 Hz (0.1ms)  (1~65536)
    TIM6->ARR = 10000 - 1;	// Auto reload  0.1ms * 10000 = 1s  
    // Clear the Counter
    TIM6->EGR |= (1 << 0);	// UG(Update generation)=1 
    // Re-initialize the counter(CNT=0) & generates an update of registers     
    // Setting an UI(UEV) Interrupt 
    NVIC->ISER[1] |= (1 << 22); // Enable Timer6 global Interrupt
    TIM6->DIER |= (1 << 0);	// Enable the Tim6 Update interrupt 
    TIM6->CR1 |= (1 << 0);	// Enable the Tim6 Counter (clock enable)   
}

void ADC_IRQHandler(void)
{
    ADC2->SR &= ~(1 << 1);		// EOC flag clear

    ADC_Value = ADC2->DR;		// Reading ADC result
    Voltage = ADC_Value * (3.3 * 100) / 4095;   // 3.3V: 4095 =  Volatge : ADC_Value 

    Thermostat = 1 / 8.25 * Voltage - 10;	// -10도 ~ 30도 설정

    if (mode == 3) // sw7_flag가 3일때만 동작
    {
        LCD_SetBrushColor(RGB_WHITE); //그 전 값 지우기
        LCD_DrawFillRect(50, 15, 120, 10);

        LCD_SetTextColor(RGB_BLUE); // 글자 색 : BLUE            
        LCD_DisplayChar(2, 7, 0 + 0x30);  // C:0
        LCD_SetTextColor(RGB_RED); // 글자색 : RED
        LCD_DisplayChar(2, 3, 0 + 0x30); // H :0
        if (Thermostat <= 0) // 온도가 0보다 작을 때
        {
           
            if (Thermostat >= -10 && Thermostat < 0) // 온도 -10~0도
            {
                LCD_SetTextColor(RGB_GREEN);
                LCD_DisplayChar(1, 3, '-'); // '-'부호 표시
            }
            else if (Thermostat ==  0) // 온도 -10~0도
            {
                LCD_DisplayChar(1, 3, ' '); // '-'부호 제거
            }
            LCD_SetTextColor(RGB_GREEN); // 글자색 : GREEN
            LCD_DisplayChar(1, 4, abs(Thermostat) / 10 + 0x30); // 온도 절대값의 10의 자리 수 표시
            LCD_DisplayChar(1, 5, abs(Thermostat) % 10 + 0x30); // 온도 절대값의 1의 자리 수 표시        
            if (Thermostat >= -10 && Thermostat <= 0) // 온도 -10~0도
            {
                LCD_SetBrushColor(RGB_BLUE); // 채우기 색 : BLUE
                LCD_DrawFillRect(55, 16, (Thermostat * 2) + 30, 6);  // 막대 1단계

                LCD_SetTextColor(RGB_RED); // 글자색 : RED
                LCD_DisplayChar(2, 3, 2 + 0x30); // H : 2

      
                TIM3->CCR3 = 9000; // Duty Ratio 90%


                LCD_SetBrushColor(RGB_WHITE); //cooler 값 지우기
                LCD_DrawFillRect(55, 52, 10, 12);

                GPIOG->ODR |= 0x0020;  // LED5 ON
                GPIOG->ODR &= 0x0020;  // 나머지 LED OFF
            }
        }
        else // 온도가 0보다 클 때
        {
            LCD_DisplayChar(1, 3, ' '); // '-'부호 제거
            LCD_SetTextColor(RGB_GREEN); // 글자색 : GREEN
            LCD_DisplayChar(1, 4, Thermostat / 10 + 0x30); // 온도의 10의 자리 수 표시
            LCD_DisplayChar(1, 5, Thermostat % 10 + 0x30); // 온도의 1의 자리 수 표시  

            if (Thermostat >= 1 && Thermostat <= 10) // 온도 1~10도
            {
                LCD_SetBrushColor(GET_RGB(135,206,235)); // 채우기 색 : SKY BLUE
                LCD_DrawFillRect(55, 16, (Thermostat * 2) + 30, 6); // 막대 2단계

                LCD_SetTextColor(RGB_RED); // 글자색 : RED
                LCD_DisplayChar(2, 3, 1 + 0x30); // H:1

                TIM3->CCR3 = 1000; // Duty Ratio 10%

                LCD_SetBrushColor(RGB_WHITE); //cooler 값 지우기
                LCD_DrawFillRect(55, 52, 10, 12);

                GPIOG->ODR |= 0x0020;  // LED5 ON
                GPIOG->ODR &= 0x0020;  // 나머지 LED OFF
            }
            else if (Thermostat >= 11 && Thermostat <= 20) // 온도 11~20
            {
                LCD_SetBrushColor(RGB_GREEN); // 채우기 색 : GREEN
                LCD_DrawFillRect(55, 16, (Thermostat * 2) + 30, 6); // 막대 3단계

                LCD_SetTextColor(RGB_RED); // 글자 색 : RED
                LCD_DisplayChar(2, 3, 0 + 0x30);  // H:0   
                LCD_SetTextColor(RGB_BLUE); // 글자 색 : BLUE            
                LCD_DisplayChar(2, 7, 0 + 0x30);  // C:0

                TIM3->CCR3 = 1000; // Duty Ratio 10%
                GPIOG->ODR &= 0x0000;   // 모든 LED OFF                         

     
            }
            else if (Thermostat >= 21 && Thermostat <= 30) // 온도 21~30
            {
                LCD_SetBrushColor(RGB_RED); // 채우기 색 : RED
                LCD_DrawFillRect(55, 16, (Thermostat * 2) + 30, 6); // 막대 4단계

                LCD_SetTextColor(RGB_BLUE); // 글자 색 : BLUE         
                LCD_DisplayChar(2, 7, 1 + 0x30); // C:1  

                TIM3->CCR3 = 9000; // Duty Ratio 10%

                LCD_SetBrushColor(RGB_WHITE); //Heater 값 지우기
                LCD_DrawFillRect(25, 52, 10, 12);

                GPIOG->ODR |= 0x0040; // LED5 ON
                GPIOG->ODR &= 0x0040; // 나머지 LED OFF                          
            }
           
        }
    }
}

void USART1_IRQHandler(void)
{
    if (mode == 1) // mode 1일때만 작동
    {
    if ((USART1->SR & USART_SR_RXNE)) // USART_SR_RXNE= 1? RX Buffer Full?
        // #define  USART_SR_RXNE ((uint16_t)0x0020)    //  Read Data Register Not Empty     
    {
        
            data = (uint16_t)(USART1->DR & (uint16_t)0x01FF);
            if (data <= 0x0F) //받아오는 데이터범위
            {
                Fram_Write(1200, data);
                BEEP();
                Alarm_flag = 0; // 알람 준비
                LCD_SetTextColor(RGB_RED);
                if (data < 10) //10보다 작을시 LCD에 표시
                {
                    Alarm_hour = data;
                    LCD_DisplayChar(1, 7, data + 0x30); 
                }
                else            //10이상일시 알파벳표시
                {
                    Alarm_hour = data - 10 + 'A';
                    LCD_DisplayChar(1, 7, data - 10 + 'A');
                }

            }
            else //데이터밖 범위: 부저 2회
            {
                BEEP();
                DelayMS(500);
                BEEP();

            }
        }



    }

}

void TIM6_DAC_IRQHandler(void)  	// 1s Interrupt
{
    TIM6->SR &= ~(1 << 0);	// Interrupt flag Clear 
    LCD_SetFont(&Gulim8);
    LCD_SetTextColor(RGB_BLUE);
    LCD_DisplayChar(0, 16, ':');

    
    if (minute_cnt < 10) // 시계 분이 10보다 작을 때
    {
        LCD_DisplayChar(0, 17, minute_cnt + '0'); // LCD에 그대로 숫자 출력
    }
    else if (minute_cnt >= 10 && minute_cnt < 16) // 시계 분이 10 이상 16 미만일 때
    {
        LCD_DisplayChar(0, 17, (minute_cnt - 10) + 'A'); // A~F로 변환 후 LCD에 출력
    }
    else if (minute_cnt >= 16) // 시계 분이 16 이상일 때
    {
        minute_cnt = 0; // 분 초기화
        LCD_DisplayChar(0, 17, minute_cnt + '0');
        hour_cnt++; // 시 증가
    }
    
    minute_cnt++; // 시계 분 1s에 1씩 증가

    if (hour_cnt < 10) // 시계 시가 10보다 작을 때
    {
        LCD_DisplayChar(0, 15, hour_cnt + '0'); // LCD에 그대로 숫자 출력
    }
    else if (hour_cnt >= 10 && hour_cnt < 16) // 시계 시가 10 이상 16 미만일 때
    {
        LCD_DisplayChar(0, 15, (hour_cnt - 10) + 'A'); // A~F로 변환 후 LCD에 출력
    }
    else if (hour_cnt >= 16) // 시계 시가 16 이상일 때
    {
        hour_cnt = 0; // 시 초기화
        Alarm_flag = 0; //알람준비
        LCD_DisplayChar(0, 15, hour_cnt + '0');
    }

  

}

void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & 0x8000)                   // EXTI15 Interrupt Pending(발생) 여부?
    {
        EXTI->PR |= 0x8000; 		// Pending bit Clear
        BEEP(); // 부저 1회
        mode++; // mode 1씩 증가   
        if (mode > 3) // mode가 4가 되면
        {
            mode = 1; // mode 다시 1
        }
        Display_Change(); // Display 바뀌는 함수
        EXTI->PR &= ~0x8000;
    }
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

// Baud rate 설정
void USART_BRR_Configuration(uint32_t USART_BaudRate)
{
    uint32_t tmpreg = 0x00;
    uint32_t APB2clock = 84000000;	//PCLK2_Frequency
    uint32_t integerdivider = 0x00;
    uint32_t fractionaldivider = 0x00;
    // Find the integer part 
    if ((USART1->CR1 & USART_CR1_OVER8) != 0) // USART_CR1_OVER8=(1<<15)
        //  #define  USART_CR1_OVER8 ((uint16_t)0x8000) // USART Oversampling by 8 enable   
    {       // USART1->CR1.OVER8 = 1 (8 oversampling)
      // Computing 'Integer part' when the oversampling mode is 8 Samples 
        integerdivider = ((25 * APB2clock) / (2 * USART_BaudRate));  // 공식에 100을 곱한 곳임(소수점 두번째자리까지 유지하기 위함)  
    }
    else  // USART1->CR1.OVER8 = 0 (16 oversampling)
    {	// Computing 'Integer part' when the oversampling mode is 16 Samples 
        integerdivider = ((25 * APB2clock) / (4 * USART_BaudRate));  // 공식에 100을 곱한 곳임(소수점 두번째자리까지 유지하기 위함)    
    }								     // 100*(f_CK) / (8*2*Buadrate) = (25*f_CK)/(4*Buadrate)	
    tmpreg = (integerdivider / 100) << 4;

    // Find the fractional part 
    fractionaldivider = integerdivider - (100 * (tmpreg >> 4));

    // Implement the fractional part in the register 
    if ((USART1->CR1 & USART_CR1_OVER8) != 0)
    {	// 8 oversampling
        tmpreg |= (((fractionaldivider * 8) + 50) / 100) & (0x07);
    }
    else	// 16 oversampling
    {
        tmpreg |= (((fractionaldivider * 16) + 50) / 100) & (0x0F);
    }
    // Write to USART BRR register
    USART1->BRR = (uint16_t)tmpreg;
}

void DisplayTitle(void)
{
    LCD_Clear(RGB_WHITE); // LCE Clear : WHITE
    LCD_SetFont(&Gulim8); // 굴림 8

    LCD_SetBackColor(RGB_WHITE);	//배경색
    LCD_SetTextColor(RGB_BLUE);	//글자색

    minute_cnt = 10; // Reset 후 초기시각 F:A
    hour_cnt = 15;

    LCD_SetBackColor(RGB_WHITE);	// 글자 배경색 : WHITE
    LCD_SetTextColor(RGB_BLACK);	// 글자색 : BLACK
    LCD_DisplayText(0, 1, "1.ALARM(NJY)");
    LCD_DisplayText(1, 1, "Alarm");
    LCD_DisplayChar(1, 8, ':');
    LCD_DisplayChar(1, 9, '0');

    Alarm_hour = Fram_Read(1200); // FRAM에서 값 읽기

    if (Alarm_hour == 0xFF) // 유효하지 않은 값일 때
    {
        Alarm_hour = 0; // 기본값 설정
        Fram_Write(1200, 0); // FRAM에 초기값 저장
    }

    LCD_SetTextColor(RGB_RED);	// 글자색 : RED  
    if (Fram_Read(1200) < 10) // Fram 1200번지에 저장된 값이 10보다 작을 때
    {
        LCD_DisplayChar(1, 7, Alarm_hour + 0x30); // LCD에 그대로 표시
    }
    else // Fram 1200번지에 저장된 값이 10보다 크거나 같을 때
    {
        LCD_DisplayChar(1, 7, 'A' + (Alarm_hour - 10)); // LCD에 A~F 16진수로 표시
    }

}


void Display_Change(void) // Display 바꾸는 함수
{
    LCD_SetBrushColor(RGB_WHITE); // 채우기 색 : WHITE
    LCD_DrawFillRect(0, 12, 160, 120); // 그 전 화면 흰색으로 clear 시켜줌

    if (mode == 1) // mode = 1일 때
    {
        LCD_SetFont(&Gulim8);
        LCD_SetBackColor(RGB_WHITE);	//배경색 : WHITE
        LCD_SetTextColor(RGB_BLACK);	//글자색 : BLACK
        LCD_DisplayText(0, 1, "1.ALARM(NJY)");
        LCD_DisplayText(1, 1, "Alarm");
        LCD_DisplayChar(1, 8, ':');
        LCD_DisplayChar(1, 9, Alarm_minute + 0x30);

        Alarm_hour = Fram_Read(1200); // FRAM에서 값 읽기

        if (Alarm_hour == 0xFF) // 유효하지 않은 값일 때
        {
            Alarm_hour = 0; // 기본값 설정
            Fram_Write(1200, 0); // FRAM에 초기값 저장
        }

        LCD_SetTextColor(RGB_RED);	//글자색 : RED   
        if (Fram_Read(1200) < 10) // Fram 1200번지에 저장된 값이 10보다 작을 때 
        {
            LCD_DisplayChar(1, 7, Alarm_hour + 0x30); // LCD에 그대로 표시
        }
        else // Fram 1200번지에 저장된 값이 10보다 크거나 같을 때
        {
            LCD_DisplayChar(1, 7, 'A' + (Alarm_hour - 10)); // LCD에 A~F 16진수로 표시
        }
       
    }

    else if (mode == 2) // mode = 2일 때 
    {
        
        LCD_SetBackColor(RGB_WHITE);
        LCD_SetTextColor(RGB_BLACK);    //글자색
        LCD_DisplayText(0, 1, "2.Ball game  ");  // Title
        LCD_SetPenColor(RGB_BLACK);
        LCD_DrawRectangle(2, 16, 100, 100);
        LCD_SetPenColor(RGB_RED);
        LCD_DrawRectangle(Ball_X, Ball_Y, 8, 8); //공 중앙위치
        LCD_SetFont(&Gulim7); 
        LCD_DisplayText(2, 18, "Ax:");
        LCD_DisplayText(3, 18, "Ay:");
       
    }

    else if (mode == 3) // mode가 3일 때
    {
        LCD_SetFont(&Gulim8);
        LCD_SetBackColor(RGB_WHITE);	//배경색 : WHITE
        LCD_SetTextColor(RGB_BLACK);	//글자색 : BLACK
        LCD_DisplayText(0, 1, "3.Thermostat");
        LCD_DisplayText(1, 1, "T:");
        LCD_DisplayText(2, 1, "H:");
        LCD_DisplayText(2, 5, "C:");
    }
}

void DelayMS(unsigned short wMS)
{
    register unsigned short i;
    for (i = 0; i < wMS; i++)
        DelayUS(1000);  // 1000us => 1ms
}
void DelayUS(unsigned short wUS)
{
    volatile int Dly = (int)wUS * 17;
    for (; Dly; Dly--);
}
uint8_t key_flag = 0;
uint16_t KEY_Scan(void)	// input key SW0 - SW7 
{
    uint16_t key;
    key = GPIOH->IDR & 0xFF00;	// any key pressed ?
    if (key == 0xFF00)		// if no key, check key off
    {
        if (key_flag == 0)
            return key;
        else
        {
            DelayMS(10);
            key_flag = 0;
            return key;
        }
    }
    else				// if key input, check continuous key
    {
        if (key_flag != 0)	// if continuous key, treat as no key input
            return 0xFF00;
        else			// if new key,delay for debounce
        {
            key_flag = 1;
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



