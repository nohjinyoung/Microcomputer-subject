////////////////////////////////////////////////////////////
// 과제명:커피자판기
// 과제개요:커피를 자동으로 제조 및 판매하는 커피자판기 제어 프로그램을 작성함
// 사용한하드웨어(기능):GPIO,Joy-stick, EXTI, GLCD ...
// 제출일:2024. 6. 15
// 제출자클래스: 화요일반
//			학번:2020130015
//			이름 :노진영
///////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"

void _GPIO_Init(void);
void _EXTI_Init(void);
void DisplayInitScreen(void);
void BEEP(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void DisplayTOTInit(void);
uint16_t KEY_Scan(void);
uint16_t JOY_Scan(void);

uint8_t IN, NOC;
uint16_t TOT ;			// TOT 값을 알아보기위한 변수(16bit는 표현이 안되어 분할함)
uint8_t TOT10 , TOT100;	// TOT 10의자리 , TOT 100자리 
uint8_t cf_choice = 0;	 // 커피 선택 여부
uint8_t cp_flag = 9;	//컵 재고 수량
uint8_t sg_flag = 5;	//슈가 재고 수량
uint8_t mk_flag = 5;	//밀크 재고 수량
uint8_t cf_flag = 9;	//커피 재고 수량
uint8_t cf = 0;		// 1:black 2:sugar 3:mix



int main(void)
{
	
	_GPIO_Init(); 	// GPIO (LED,SW,Buzzer,Joy stick) 초기화
	LCD_Init();	// LCD 모듈 초기화
	DelayMS(10);
	_EXTI_Init();	// EXTI 초기화

	Fram_Init();            // FRAM 초기화 H/W 초기화
	Fram_Status_Config();   // FRAM 초기화 S/W 초기화

	
	IN = Fram_Read(50);	        //IN 50번지 설정
	TOT10 = Fram_Read(61);	        //TOT10 61번지 설정
	TOT100 = Fram_Read(62);	//TOT100 62번지 설정
        TOT = (TOT100 * 100) + (TOT10 * 10);    //TOT 정의
	NOC = Fram_Read(70);	//NOC 70번지 설정

	DisplayInitScreen();    // LCD 초기화면


	

	GPIOG->ODR &= ~0x00FF;	// LED 초기값: LED0~7 Off
    
 
	while(1) 
        {
		if (cp_flag * sg_flag * mk_flag * cf_flag == 0) //한가지라도 재고가 0일시
		{
			LCD_SetBrushColor(RGB_RED);
			LCD_DrawFillRect(120, 78, 16, 16);  //RF 박스 빨간색으로 채우기
			DelayMS(50);
		}
		switch (KEY_Scan())
		{
		case 0xFB00: 	//SW2
			if (cf_choice == 1)// 커피 선택했을때
			{
				EXTI->IMR &= ~0x3B00;// 커피 제조중일 떄 모든 동작 X					
				if (cp_flag * sg_flag  * mk_flag * cf_flag != 0) //한가지라도 재고가 남아있을때
				{

					if ((IN >= 10) && (cf == 1)) // BLACK커피 선택했을때 10보다 같거나 클경우
					{
						BEEP();
						GPIOG->ODR |= 0x00FF;		// led0~7 점등
						LCD_SetTextColor(RGB_WHITE); 
						LCD_SetBackColor(RGB_RED);
						LCD_DisplayText(4, 4, "0");
						DelayMS(1000);
						LCD_DisplayText(4, 4, "1");
						DelayMS(1000);
						LCD_DisplayText(4, 4, "2");
						DelayMS(1000);
						LCD_DisplayText(4, 4, "W");
						GPIOG->ODR &= ~0x00FF;		// led0~7 소등
						BEEP();
						DelayMS(500);
						BEEP();
						DelayMS(500);
						BEEP();
						IN -= 10; // IN 저장된 데이터에서 10 빼기
						Fram_Write(50, IN);				// 50번지에 IN 저장
						LCD_SetBackColor(RGB_BLACK);    // 글자 배경색: BLACK
						LCD_SetTextColor(RGB_YELLOW);	// 글자색 :YELLOW
						LCD_DisplayChar(1, 15, IN / 100 + 0x30); // 100의 자리
						LCD_DisplayChar(1, 16, (IN / 10) % 10 + 0x30); // 10의 자리
						LCD_DisplayChar(1, 17, 0 + 0x30); // 1의 자리
						TOT += 10;                     //TOT에 10추가  
						if (TOT > 990)
						{
							TOT = 990;          //TOT 가 990이 넘어가면 990고정
						}
						TOT100 = TOT / 100;				//TOT 100의자리 정의			
						TOT10 = (TOT % 100) / 10;		//TOT 10의 자리 정의			
						LCD_SetBackColor(RGB_BLACK);    // 글자 배경색: BLACK
						LCD_SetTextColor(RGB_YELLOW);	// 글자색 : YELLOW
						Fram_Write(61, TOT10);		//61번지에 TOT10 저장
						Fram_Write(62, TOT100);		//62번지에 TOT100 저장
						LCD_DisplayChar(4, 15, TOT100 + 0x30); // 100의 자리
						LCD_DisplayChar(4, 16, TOT10 + 0x30); // 10의 자리
						LCD_DisplayChar(4, 17, 0 + 0x30); // 1의 자리
						
						NOC++;
						if (NOC <= 50)				
						{
							Fram_Write(70, NOC);	//70번지에 NOC저장
						}
						else 
						{
							NOC = 50;			// 50넘을시 50으로 고정
							Fram_Write(70, NOC);		//70번지에 NOC저장
						}
						LCD_SetBackColor(RGB_YELLOW);    // 글자 배경색: YELLOW
						LCD_SetTextColor(RGB_BLACK);	// 글자색 : BLACK
						LCD_DisplayChar(9, 13, (NOC / 10) % 10 + 0x30); // NOC 백의 자리
						LCD_DisplayChar(9, 14, NOC % 10 + 0x30); // NOC 십의 자리

						LCD_SetBackColor(RGB_BLUE);    // 글자 배경색: BLUE
						LCD_SetTextColor(RGB_WHITE);	// 글자색 : WHITE
						LCD_DisplayText(3, 2, "B");
						cp_flag--;						//컵 1 소모
						cf_flag--;						//커피  1 소모
						LCD_SetBackColor(RGB_WHITE);    // 글자 배경색: WHITE
						LCD_SetTextColor(RGB_BLACK);	// 글자색 : BLACK
						LCD_DisplayChar(6, 1, cp_flag + 0x30);	//컵 재고 수량 표시
						LCD_DisplayChar(6, 7, cf_flag + 0x30); //커피 재고 수량 표시
						cf = 1;							//BLACK커피를 선택							
						cf_choice = 0;					//선택끝					

					}
					else if ((IN >= 20) && (cf == 2))	//SUGAR커피 선택했을때 10보다 같거나 클경우
					{
						
						BEEP();
						GPIOG->ODR |= 0x00FF;			// led0~7 점등
						LCD_SetTextColor(RGB_WHITE);;	
						LCD_SetBackColor(RGB_RED);
						LCD_DisplayText(4, 4, "0");
						DelayMS(1000);
						LCD_DisplayText(4, 4, "1");
						DelayMS(1000);
						LCD_DisplayText(4, 4, "2");
						DelayMS(1000);
						LCD_DisplayText(4, 4, "W");
						GPIOG->ODR &= ~0x00FF;			// led0~7 소등
						BEEP();
						DelayMS(500);
						BEEP();
						DelayMS(500);
						BEEP();
						IN -= 20;	// IN 저장된 데이터에서 20 빼기
						Fram_Write(50, IN);				// 50번지에 IN 저장
						LCD_SetBackColor(RGB_BLACK);     // 글자 배경색: BLACK
						LCD_SetTextColor(RGB_YELLOW);	// 글자색 :YELLOW
						LCD_DisplayChar(1, 15, IN / 100 + 0x30);  // 100의 자리
						LCD_DisplayChar(1, 16, (IN / 10) % 10 + 0x30); //10의 자리
						LCD_DisplayChar(1, 17, 0 + 0x30); //1의 자리
						TOT += 20;                       //TOT에 20추가  
						if (TOT > 990)
						{
                                                  TOT = 990;          //TOT 가 990이 넘어가면 990고정
						}
						TOT100 = TOT / 100;				//TOT 백의 자리 정의
						TOT10 = (TOT % 100) / 10;		//TOT 십의 자리 정의
						LCD_SetBackColor(RGB_BLACK);    // 글자 배경색: BLACK
						LCD_SetTextColor(RGB_YELLOW);	// 글자색 : YELLOW
						Fram_Write(61, TOT10);		//61번지에 TOT10 저장
						Fram_Write(62, TOT100);		//62번지에 TOT100 저장
						LCD_DisplayChar(4, 15, TOT100 + 0x30); // 100의 자리
						LCD_DisplayChar(4, 16, TOT10 + 0x30); // 10의 자리
						LCD_DisplayChar(4, 17, 0 + 0x30); // 1의 자리
						NOC++;
						if (NOC <= 50)
						{
							Fram_Write(70, NOC);	//70번지에 NOC저장
						}
						else
						{
							NOC = 50;				//50이 넘을시 50 고정
							Fram_Write(70, NOC);		//70번지에 NOC저장
						}
						LCD_SetBackColor(RGB_YELLOW);    // 글자 배경색: YELLO
						LCD_SetTextColor(RGB_BLACK);	// 글자색 : BLACK
						LCD_DisplayChar(9, 13, (NOC / 10) % 10 + 0x30); // NOC 백의 자리
						LCD_DisplayChar(9, 14, NOC % 10 + 0x30);  // NOC 십의 자리
						LCD_SetBackColor(RGB_BLUE);   // 글자 배경색: BLUE
						LCD_SetTextColor(RGB_WHITE);	// 글자색 : WHITE
						LCD_DisplayText(2, 4, "S");
						cp_flag--;						//컵 1 소모
						cf_flag--;						//커피 1 소모
						sg_flag--;						//슈가 1 소모
						LCD_SetBackColor(RGB_WHITE);    // 글자 배경색: WHITE
						LCD_SetTextColor(RGB_BLACK);	// 글자색 : BLACK
						LCD_DisplayChar(6, 1, cp_flag + 0x30);	//컵 재고 수량 표시
						LCD_DisplayChar(6, 3, sg_flag + 0x30);	//슈가 재고 수량 표시
						LCD_DisplayChar(6, 7, cf_flag + 0x30);	//커피 재고 수량 표시
						cf = 2;							//SUGAR커피를 선택
						cf_choice = 0;					//선택 끝
					
					}
					else if ((IN >= 30) && (cf == 3))	// MIX커피 선택했을때 10보다 같거나 클경우
					{
						
						BEEP();							// led0~7 점등
						GPIOG->ODR |= 0x00FF;
						LCD_SetTextColor(RGB_WHITE);;	
						LCD_SetBackColor(RGB_RED);
						LCD_DisplayText(4, 4, "0");
						DelayMS(1000);
						LCD_DisplayText(4, 4, "1");
						DelayMS(1000);
						LCD_DisplayText(4, 4, "2");
						DelayMS(1000);
						LCD_DisplayText(4, 4, "W");
						GPIOG->ODR &= ~0x00FF;			// led0~7 소등
						BEEP();
						DelayMS(500);
						BEEP();
						DelayMS(500);
						BEEP();
						IN -= 30;	// IN 저장된 데이터에서 30 빼기
						Fram_Write(50, IN);				// 50번지에 IN 저장
						LCD_SetBackColor(RGB_BLACK);    // 글자 배경색: BLACK
						LCD_SetTextColor(RGB_YELLOW);	// 글자색 : YELLOW
						LCD_DisplayChar(1, 15, IN / 100 + 0x30); // 100의 자리
						LCD_DisplayChar(1, 16, (IN / 10) % 10 + 0x30); // 10의 자리
						LCD_DisplayChar(1, 17, 0 + 0x30); // 1의 자리
						TOT += 30;                       //TOT에 30추가  
						if (TOT > 990)
						{
							TOT = 990;          //TOT 가 990이 넘어가면 990고정
						}
						TOT100 = TOT / 100;				//TOT 100의 자리 정의
						TOT10 = (TOT % 100) / 10;		//TOT 10의 자리 정의
						LCD_SetBackColor(RGB_BLACK);    // 글자 배경색: BLACK
						LCD_SetTextColor(RGB_YELLOW);	// 글자색 : YELLOW
						Fram_Write(61, TOT10);		//61번지에 TOT10 저장
						Fram_Write(62, TOT100);		//62번지에 TOT100 저장
						LCD_DisplayChar(4, 15, TOT100 + 0x30); // 100의 자리
						LCD_DisplayChar(4, 16, TOT10 + 0x30); // 10의 자리
						LCD_DisplayChar(4, 17, 0 + 0x30); // 1의 자리
						NOC++;
						if (NOC <= 50)
						{
							Fram_Write(70, NOC);		//70번지에 NOC저장
						}
						else
						{
							NOC = 50;					//NOC가 50을 넘을 시에 50 고정
							Fram_Write(70, NOC);		//70번지에 NOC저장
						}
						LCD_SetBackColor(RGB_YELLOW);    // 글자 배경색: YELLOW
						LCD_SetTextColor(RGB_BLACK);	// 글자색 : BLACK
						LCD_DisplayChar(9, 13, (NOC / 10) % 10 + 0x30); // NOC 백의 자리
						LCD_DisplayChar(9, 14, NOC % 10 + 0x30); // NOC 십의 자리

						LCD_SetBackColor(RGB_BLUE);    // 글자 배경색:  BLUE
						LCD_SetTextColor(RGB_WHITE);	// 글자색 : WHITE
						LCD_DisplayText(3, 6 , "M");
						cp_flag--;		// 컵 1 소요
						cf_flag--;		// 커피 1 소요
						mk_flag--;		// 밀크 1 소요
						sg_flag--;		// 슈가 1 소요
						LCD_SetBackColor(RGB_WHITE);    // 글자 배경색: WHITE
						LCD_SetTextColor(RGB_BLACK);	// 글자색 : BLACK
						LCD_DisplayChar(6, 1, cp_flag + 0x30);	//컵 재고 수량 표시
						LCD_DisplayChar(6, 3, sg_flag + 0x30);	//슈가 재고 수량 표시
						LCD_DisplayChar(6, 5, mk_flag + 0x30);	//밀크 재고 수량 표시
						LCD_DisplayChar(6, 7, cf_flag + 0x30);	//커피 재고 수량 표시
						cf = 3;		//MIX커피를 선택
						cf_choice = 0;					//선택 끝
					}

				}
				
				EXTI->IMR |= 0x3B00; // 커피 제조중 끝났으니 다시 동작을 Enable
			}
			break;
		}

		switch (JOY_Scan())
		{
			while (cf_choice == 0)	//동작스위치 누르기전에 커피 선택은 계속 가능
			{
			case 0x01E0:     //NAVI_LEFT(BLACK)
			BEEP();
			LCD_SetBackColor(RGB_BLUE);    
			LCD_SetTextColor(RGB_RED);	
			LCD_DisplayText(3, 2, "B");	// B 빨간색 글자로 바꾸기
			LCD_SetBackColor(RGB_BLUE);    
			LCD_SetTextColor(RGB_WHITE);	
			LCD_DisplayText(2, 4, "S");
			LCD_DisplayText(3, 6, "M");
			cf = 1;			//BLACK커피 선택
			cf_choice = 1;	//커피 선택완료
			
			break;

		case 0x03A0:	   // NAVI_RIGHT(SUGAR)
			BEEP();
			LCD_SetBackColor(RGB_BLUE);    
			LCD_SetTextColor(RGB_RED);	
			LCD_DisplayText(2, 4, "S");	// S 빨간색 글자로 바꾸기
			LCD_SetBackColor(RGB_BLUE);    
			LCD_SetTextColor(RGB_WHITE);	
			LCD_DisplayText(3, 2, "B");
			LCD_DisplayText(3, 6, "M");
			cf = 2;			//MILK커피 선택
			cf_choice = 1;	//커피 선택완료
			

			break;

		case 0x02E0: //NAVI_UP(MIX)
			BEEP();
			LCD_SetBackColor(RGB_BLUE);    
			LCD_SetTextColor(RGB_RED);	
			LCD_DisplayText(3, 6, "M");	// M 빨간색 글자로 바꾸기
			LCD_SetBackColor(RGB_BLUE);   
			LCD_SetTextColor(RGB_WHITE);	
			LCD_DisplayText(2, 4, "S");
			LCD_DisplayText(3, 2, "B");
			cf = 3;			//MIX커피 선택
			cf_choice = 1;	//커피 선택완료
			

			break;
			}
		}  // switch(JOY_Scan())
         
        }
}


/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
	LCD_Clear(RGB_WHITE);		// 화면 클리어
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
	LCD_SetBackColor(RGB_WHITE);    // 글자 배경색: WHITE
	LCD_SetTextColor(RGB_BLACK);	// 글자색 : BLACK
	LCD_DisplayText(6, 1, "9");
	LCD_DisplayText(6, 3, "5");
	LCD_DisplayText(6, 5, "5");
	LCD_DisplayText(6, 7, "9");
	LCD_DisplayText(1, 11, "\\10");
	LCD_DisplayText(3, 11, "\\50");
	LCD_DisplayText(0, 15, "IN");
	LCD_DisplayText(3, 15, "TOT");
	LCD_DisplayText(6, 13, "RF");
	LCD_DisplayText(8, 13, "NoC");
	LCD_DisplayText(7, 1, "cpsgmkcf");
	for (int i = 0; i < 4; i++)  // 재고 수량 표시 박스 4개 만들기 
	{
		LCD_SetPenColor(RGB_GREEN);
		LCD_DrawRectangle(5 + 16 * i, 78, 12, 12);

	}


	LCD_DrawRectangle(11, 36, 16, 16); //  B박스
	LCD_DrawRectangle(27, 24, 16, 16); // S박스
	LCD_DrawRectangle(27, 50, 16, 16); //  W박스
	LCD_DrawRectangle(43, 36, 16, 16); //   M박스

	LCD_SetBrushColor(RGB_GREEN);
	LCD_DrawFillRect(120, 78, 16, 16);  //RF 상자 (초록색)


	LCD_DrawRectangle(99, 26, 10, 10); //10 박스
	LCD_DrawRectangle(99, 52, 10, 10); //50 박스
	LCD_SetBrushColor(RGB_GRAY); 
	LCD_DrawFillRect(100, 27, 9, 9);	//10 박스 색 채우기(회색)
	LCD_DrawFillRect(100, 53, 9, 9);	//50 박스 색 채우기(회색)

	LCD_DrawRectangle(118, 12, 26, 14); // IN 박스
	LCD_DrawRectangle(118, 50, 26, 14); // OUT 박스
	LCD_SetBrushColor(RGB_BLACK);
	LCD_DrawFillRect(119, 13, 25, 13);	// IN 박스 색채우기(검정)
	LCD_DrawFillRect(119, 51, 25, 13);	// OUT 박스 색채우기(검정)

	LCD_SetBackColor(RGB_BLACK);		//글자 배경색:BLACK
	LCD_SetTextColor(RGB_YELLOW);		//글자색:YELLOW
	LCD_DisplayChar(1, 15,	IN / 100 + 0x30); // IN 100의 자리
	LCD_DisplayChar(1, 16, (IN / 10) % 10 + 0x30); // IN 10의 자리
	LCD_DisplayChar(1, 17, 0 + 0x30); // IN 1의 자리
	LCD_DisplayChar(4, 15, TOT100 + 0x30); // TOT 100의 자리
	LCD_DisplayChar(4, 16, TOT10 + 0x30); // TOT 10의 자리
	LCD_DisplayChar(4, 17, 0 + 0x30); // TOT 1의 자리


	LCD_DrawRectangle(105, 116, 20, 20);  //NOC 상자
	LCD_SetBrushColor(RGB_YELLOW);			
	LCD_DrawFillRect(106, 117, 19, 19);		//NOC 상자 색채우기(노랑색)

	LCD_SetBrushColor(RGB_BLUE);
	LCD_DrawFillRect(12, 37, 15, 15);  //B 박스
	LCD_DrawFillRect(28, 25, 15, 15);  //S 박스
	LCD_DrawFillRect(44, 37, 15, 15);  //M 박스
	LCD_SetBrushColor(RGB_RED);
	LCD_DrawFillRect(28, 51, 15, 15);  //W 박스

	LCD_SetTextColor(RGB_WHITE);
	LCD_SetBackColor(RGB_BLUE);			
	LCD_DisplayText(3, 2, "B");			//B 박스안에 B 쓰기
	LCD_DisplayText(2, 4, "S");			//S 박스안에 S 쓰기
	LCD_DisplayText(3, 6, "M");			//M 박스안에 M 쓰기
	LCD_SetBackColor(RGB_RED);
	LCD_DisplayText(4, 4, "W");			//W 박스안에 W 쓰기



	LCD_SetPenColor(RGB_BLACK);
	LCD_DrawRectangle(1, 0, 80, 13);   // NJY coffee 박스
	LCD_SetBrushColor(RGB_YELLOW); 
	LCD_DrawFillRect(2, 1, 79, 12);		// NJY coffee 박스 색채우기(노랑색)
	LCD_SetTextColor(RGB_BLACK);
	LCD_SetBackColor(RGB_YELLOW);    
	LCD_DisplayText(0, 0, "NJY coffee");
	LCD_DisplayChar(9, 13, (NOC / 10) % 10 + 0x30);	//NOC 10의 자리
	LCD_DisplayChar(9, 14, NOC % 10 + 0x30);		//NOC 1의 자리


}
/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer)) 초기 설정	*/
void _GPIO_Init(void)
{
	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	// LED (GPIO G) 설정 : Output mode
	GPIOG->MODER 	&= ~0x0000FFFF;	// GPIOG 0~7 : Clear(0b00)						
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						

	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	

	GPIOG->OSPEEDR 	&= ~0x0000FFFF;	// GPIOG 0~7 : Clear(0b00)
	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
	// PUPDR : Default (floating) 
   
	// SW (GPIO H) 설정 : Input mode 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 : Output mode
	RCC->AHB1ENR	|=  0x00000020;	// RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER 	|=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 

	//Joy Stick SW(PORT I) 설정
	RCC->AHB1ENR |= 0x00000100;	// RCC_AHB1ENR GPIOI Enable
	GPIOI->MODER &= ~0x000FFC00;	// GPIOI 5~9 : Input mode (reset state)
	GPIOI->PUPDR  &= ~0x000FFC00;	// GPIOI 5~9 : Floating input (No Pull-up, pull-down) (reset state)       
}	

void _EXTI_Init(void)
{
	RCC->AHB1ENR 	|= 0x0180;	// RCC_AHB1ENR GPIOH,GPIOI Enable
	RCC->APB2ENR 	|= 0x4000;	// Enable System Configuration Controller Clock
	
	SYSCFG->EXTICR[2] |= 0x7077;	// EXTI8,9,11에 대한 소스 입력은 GPIOH로 설정
					// EXTI8 <- PH8, EXTI9 <- PH9 
					// EXTICR3(EXTICR[2])를 이용 
					// reset value: 0x0000	
	SYSCFG->EXTICR[3] |= 0x0077;	// EXTI12,13에 대한 소스 입력은 GPIOH로 설정
	
	EXTI->FTSR |= 0x100;		// EXTI8: Falling Trigger Enable 
	EXTI->FTSR |= 0x200;		// EXTI9: Falling Trigger Enable 
	EXTI->FTSR |= 0x800;		// EXTI11: Falling Trigger Enable 
	EXTI->FTSR |= 0x1000;		// EXTI12: Falling Trigger Enable 
	EXTI->FTSR |= 0x2000;		// EXTI13: Falling Trigger Enable  

	
	EXTI->IMR  |= 0x3B00;		// EXTI8,9,11,12,13 인터럽트 mask (Interrupt Enable) 설정
	

	NVIC->ISER[0] |= ( 1<<23  );	// Enable 'Global Interrupt EXTI7,8,9'
					// Vector table Position 참조
    NVIC->ISER[1] |= ( 1<<8 );      //Enable 'Global Interrupt EXTI11,12,13'
}

/* EXTI5~9 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)
{
	if (EXTI->PR & 0x0100) // EXTI8 Interrupt Pending 여부
	{
		if (cp_flag * sg_flag * mk_flag * cf_flag != 0) //재료중 한가지라도 재고가 0이 아닐떄 
		{
			EXTI->PR |= 0x0100; // Pending bit Clear
			LCD_SetTextColor(RGB_YELLOW); // 글자 색: YELLOW
			LCD_SetBackColor(RGB_BLACK); // 글자 배경색: BLACK
			IN += 10;		// IN에 10 추가
			if (IN <= 200)
			{
				Fram_Write(50, IN); // FRAM 50번지에 IN 값 저장
			}
			else
			{
				IN = 200;	//IN이 200 넘어갈시 200으로 고정
				Fram_Write(50, IN); // FRAM 50번지에 IN 값 저장
			}
			LCD_DisplayChar(1, 15, IN / 100 + 0x30); // IN 100의 자리
			LCD_DisplayChar(1, 16, (IN / 10) % 10 + 0x30); // IN 10의 자리
			LCD_DisplayChar(1, 17, 0 + 0x30); // IN 1의 자리
			BEEP();
			LCD_SetBrushColor(RGB_YELLOW);
			LCD_DrawFillRect(100, 27, 9, 9);	// 10 박스 노란색으로 채워지기
			DelayMS(1000);
			LCD_SetBrushColor(RGB_GRAY);
			LCD_DrawFillRect(100, 27, 9, 9);	// 10 박스 1초후 회색으로 채워지기
		}
	}
	else if (EXTI->PR & 0x0200) // EXTI9 Interrupt Pending 여부
	{
		if (cp_flag * sg_flag * mk_flag * cf_flag != 0)	 //재료중 한가지라도 재고가 0이 아닐떄 
		{
			EXTI->PR |= 0x0200; // Pending bit Clear
			LCD_SetTextColor(RGB_YELLOW); // 글자 색: YELLOW
			LCD_SetBackColor(RGB_BLACK); // 글자 배경색: BLACK
			IN += 50;	// IN에 50 추가
			if (IN <= 200)
			{
				Fram_Write(50, IN); // FRAM 50번지에 IN 값 저장
			}
			else
			{
				IN = 200;	//IN이 200 넘어갈시 200으로 고정
				Fram_Write(50, IN); // FRAM 50번지에 IN 값 저장
			}
			LCD_DisplayChar(1, 15, IN / 100 + 0x30); // 100의 자리
			LCD_DisplayChar(1, 16, (IN / 10) % 10 + 0x30); // 10의 자리
			LCD_DisplayChar(1, 17, 0 + 0x30); // 1의 자리
			BEEP();
			LCD_SetBrushColor(RGB_YELLOW);
			LCD_DrawFillRect(100, 53, 9, 9);	// 50 박스 노란색으로 채워지기
			DelayMS(1000);
			LCD_SetBrushColor(RGB_GRAY);
			LCD_DrawFillRect(100, 53, 9, 9);	// 50 박스 1초후 회색으로 채워지기
		}
		
	}
	

}
void EXTI15_10_IRQHandler(void){
 
	if (EXTI->PR & 0x0800)		// EXTI11 Interrupt Pending 여부? (잔돈 반환)
	{

		EXTI->PR |= 0x0800;		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생) (IN 리셋)
		IN = 0;							// IN에 0 저장
		Fram_Write(50, IN);				// 50번지에 IN 저장
		LCD_SetBackColor(RGB_BLACK);    // 글자 배경색: WHITE
		LCD_SetTextColor(RGB_YELLOW);	// 글자색 : BLACK
		LCD_DisplayChar(1, 15, IN / 100 + 0x30); // IN 100의 자리
		LCD_DisplayChar(1, 16, (IN / 10) % 10 + 0x30); // IN 10의 자리
		LCD_DisplayChar(1, 17, 0 + 0x30); // IN 1의 자리

	}

	
	if(EXTI->PR & 0x1000)		// EXTI12 Interrupt Pending(발생) 여부? (RF)
	{ 
		EXTI->PR |= 0x0100;		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
		BEEP();                         // 부저 1회
		DelayMS(500);
		BEEP();							// 부저 1회
		if (cp_flag == 0)				
		{
			cp_flag = 9;				// 컵 재고가 0일때 9로 채우기
		}
		if (sg_flag == 0)
		{
			sg_flag = 5;				// 슈가 재고가 0일때 5로 채우기
		}
		if (mk_flag == 0)
		{
			mk_flag = 5;				// 밀크 재고가 0일때 5로 채우기
		}
		if (cf_flag == 0)
		{
			cf_flag = 9;				// 커피 재고가 0일때 9로 채우기

		}
		LCD_SetBrushColor(RGB_GREEN);	
		LCD_DrawFillRect(120, 78, 16, 16);	//RF 박스 색 채우기 (초록색)
		LCD_SetBackColor(RGB_WHITE);    // 글자 배경색: WHITE
		LCD_SetTextColor(RGB_BLACK);	// 글자색 : BLACK
		LCD_DisplayChar(6, 1, cp_flag + 0x30);	//컵 재고 수량 표시
		LCD_DisplayChar(6, 3, sg_flag + 0x30);	//슈가 재고 수량 표시
		LCD_DisplayChar(6, 5, mk_flag + 0x30);	//밀크 재고 수량 표시
		LCD_DisplayChar(6, 7, cf_flag + 0x30);	//커피 재고 수량 표시
		
	}
	if (EXTI->PR & 0x2000)		// EXTI13 Interrupt Pending(발생) 여부? (CLEAR)
	{
	
		EXTI->PR |= 0x2000;		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
		NOC = 0;	//NOC에 0저장
		TOT = 0;	//TOT에 0저장
		TOT100 = 0;	//TOT백의 자리 
		TOT10 = 0;	//TOT십의 자리
		Fram_Write(61, TOT10); //61번지에 TOT10저장
		Fram_Write(62, TOT100); //62번지에 TOT100저장
		Fram_Write(70, NOC); //70번지에 NOC저장
		LCD_SetBackColor(RGB_YELLOW);    // 글자 배경색: YELLOW
		LCD_SetTextColor(RGB_BLACK);	// 글자색 : BLACK
		LCD_DisplayChar(9, 13, (NOC / 10) % 10 + 0x30); // NOC 10의 자리                   
		LCD_DisplayChar(9, 14, NOC % 10 + 0x30); // NOC 1의 자리
		LCD_SetBackColor(RGB_BLACK);    // 글자 배경색: BLACK
		LCD_SetTextColor(RGB_YELLOW);	// 글자색 : YELLOW
		LCD_DisplayChar(4, 15, TOT100 + 0x30); // TOT 100의 자리
		LCD_DisplayChar(4, 16, TOT10 + 0x30); // TOT 10의 자리
		LCD_DisplayChar(4, 17, 0 + 0x30); // TOT 1의 자리

	}
	
	
}

/* Switch가 입력되었는지를 여부와 어떤 switch가 입력되었는지의 정보를 return하는 함수  */
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

uint8_t joy_flag = 0;
uint16_t JOY_Scan(void)	// input joy stick NAVI_* 
{
	uint16_t key;
	key = GPIOI->IDR & 0x03E0;	// any key pressed ?
	if (key == 0x03E0)		// if no key, check key off
	{
		if (joy_flag == 0)
			return key;
		else
		{
			DelayMS(10);
			joy_flag = 0;
			return key;
		}
	}
	else				// if key input, check continuous key
	{
		if (joy_flag != 0)	// if continuous key, treat as no key input
			return 0x03E0;
		else			// if new key,delay for debounce
		{
			joy_flag = 1;
			DelayMS(10);
			return key;
		}
	}
}

void BEEP(void)			
{ 	
	GPIOF->ODR |=  0x0200;	// PF9 'H' Buzzer on
	DelayMS(30);		// Delay 30 ms
	GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
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
