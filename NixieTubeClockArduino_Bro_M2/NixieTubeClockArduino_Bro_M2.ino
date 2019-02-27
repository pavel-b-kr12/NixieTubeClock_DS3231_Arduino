//free 3 9 A4 A5 A7 A8

//#define tst //serial out
//#define DateShow_constant //not switch back untill btn
#define bDateShow_untill_td 9 //sec

byte DateShow_onChange_length=12; //cycles  //for(byte i = 0; i < DateShow_onChange_length
#define DateShow_onChange_d	2 //ms //animation speed while switch to data
//100*4=2229
//100*5=2329


#define tstSerSensor //|| Serial.read()>0


#ifdef tst
	long tstTimer_t0=0;
	#define tstTimerStart	tstTimer_t0=millis();
	#define tstTimerPrint(...)	Serial.print(__VA_ARGS__); Serial.print(" take ms: "); Serial.println(millis()-tstTimer_t0); //Serial.print(__func__); => loop
	//usage:
	//tstTimerStart
	//DateShow_onChange();
	//tstTimerPrint("DateShow_onChange")
#else
	#define tstTimerStart
	#define tstTimerPrint(...)
#endif

byte SafeLength=2;

#include <DS3231.h>
#include <Wire.h>

#define  btn_swap_time_date		2 //btn show date
#define  irLED					5  // Timer 2 "A" output: OC2A

//Выводы (если свободны)  для кнопок и транзистора|реле-модуля подсветки:
//#define  btn_backlight 			5 //btn backlight //otherwise - use main btn in mode==mode_show
#define  backlight_out 			10 //out for N ch FET for backlight



//#define HT
#ifdef HT
#include <DHT.h>
#define DHTPIN 3
DHT dht(DHTPIN, DHT11);
bool bShowHT=false;
long	showHT_untill_t=0;
float hum,temper;
#endif


// выводы для дешифратора
const int out1 = 		A3;
const int out2 = 		A1;
const int out4 = 		A0;
const int out8 = 		A2;
// выводы для транзисторных ключей
const int key1= 		4;
const int key2 = 		6;
const int key3 = 		7;
const int key4 = 		8;
const int key5 = 		11;
const int key6 = 		12;

const  int LED_colon=	13;
const int keypad=		A6;// пин для клавиатуры

int keynum=0; //номер нажатой кнопки
byte currentdigit = 0; //0 - установка часов 1- установка минут
bool bShowDigit = 0; // флаг для мигания цифр при установке времени и светодиода (двоеточие)


DS3231 Clock;
bool Century=false;
bool h12;
bool PM;
byte ADay, AHour, AMinute, ASecond, ABits;
bool ADy, A12h, Apm;

//определяем глобальные переменные для различных параметров часов
int second,minute,hour,date,month,year,temperature;

int lastMinute;

#define key_Add_BacklightSw	1
#define key_Menu_modeChange	2
#define key_Sub				3

void setup() {
	TCCR1B=TCCR1B&0b11111000|0x01; // задаем частоту ШИМ на 9 выводе 30кГц    Timer 1
	analogWrite(9,240);

	Wire.begin();

	#ifdef tst
	Serial.begin(57600);
	#endif

	pinMode(out1,OUTPUT);
	pinMode(out2,OUTPUT);
	pinMode(out4,OUTPUT);
	pinMode(out8,OUTPUT);

	pinMode(key1,OUTPUT);
	pinMode(key2,OUTPUT);
	pinMode(key3,OUTPUT);
	pinMode(key4,OUTPUT);
	pinMode(key5,OUTPUT);
	pinMode(key6,OUTPUT);
	pinMode(backlight_out,OUTPUT);


	pinMode(btn_swap_time_date,INPUT_PULLUP);
	
	#ifdef irLED
		pinMode (irLED, OUTPUT);
		//TCCR0B = TCCR0B & 0b11111000 | <setting>;
			//	// set up Timer 2  PIN 10
		//TCCR0A = _BV (COM2A0) | _BV(WGM21);  // CTC, toggle OC2A on Compare Match
		//TCCR0B = _BV (CS20);   // No prescaler
		//OCR0A =  209;          // compare A register value (210 * clock speed)
							 // = 13.125 nS , so frequency is 1 / (2 * 13.125) = 38095
							 
		
		tone(irLED,38000); //! test freq
		
	//	// set up Timer 2  PIN 10
	//	TCCR2A = _BV (COM2A0) | _BV(WGM21);  // CTC, toggle OC2A on Compare Match
	//	TCCR2B = _BV (CS20);   // No prescaler
	//	OCR2A =  209;          // compare A register value (210 * clock speed)
							 // = 13.125 nS , so frequency is 1 / (2 * 13.125) = 38095
	#endif
	
	#ifdef btn_backlight
	pinMode(btn_backlight,INPUT_PULLUP);
	#endif
	
	pinMode(LED_colon,OUTPUT);

	ReadDS3231();
	delay(1000);
	lastMinute = minute%10;

#ifdef HT
 dht.begin();
#endif
}
byte nextReadRTC_i=0;
void ReadDS3231()
{
	switch(nextReadRTC_i%8)
	{
		case 0:
		minute=Clock.getMinute();
		break;
		case 1:
		hour=Clock.getHour(h12, PM);
		break;
		break;
		case 2:
		second=Clock.getSecond();
		break;
		case 3:
		date=Clock.getDate();
		break;
		case 4:
		{
		bool Century=false;
		month=Clock.getMonth(Century);
		}
		break;
		case 5:
		second=Clock.getSecond();
		break;
		case 6:
		year=Clock.getYear();
		break;
		case 7:
		second=Clock.getSecond();
		break;
	}
	#ifdef tst
	// Serial.print(Clock.getDOWStr());	Serial.print(" ");
	//	Serial.print(Clock.getDateStr());	Serial.print(" ");
	// Serial.println(Clock.getTimeStr());	Serial.print(" ");
	// Serial.println(Clock.getTemperature(),2);
	#endif
	nextReadRTC_i++;
}

#define mode_show			0
#define modeH		1
#define modeMin		2
#define modeDay		3
#define modeMon		4
#define modeYear	5

// #define mode_change_pos		1
// #define mode_change_value	2
// #define mode_showHT			3	//temperature=Clock.getTemperature(); //.f
// #define mode_showP			4

byte mode=0;
const byte mode_M=5;

long bDateShow_untill_t=0;
long bbacklight_untill_t=0;
bool bBacklight=false;
bool bDateShow=false;

byte digits[6];
byte digits_last[6];

long nextBtnRead=1000;
long nextReadRTC=0;


void fill_digits_withTimeOrDate()
{
	if(bDateShow)
	{
		digits[0] = date / 10;
		digits[1] = date  % 10;
		digits[2] = month / 10;
		digits[3] = month % 10;
		digits[4] = year / 10;
		digits[5] = year % 10;
	}
	else
	{
		digits[0] = hour/10;
		digits[1] = hour%10;
		digits[2] = minute/10;
		digits[3] = minute%10;
		digits[4] = second/10;
		digits[5] = second%10;
	}
}
#ifdef HT
void fill_digits_withHT()
{

	{
		digits[0] = ((int)temper) / 10;
		digits[1] = ((int)temper)  % 10;
		digits[2] = 0;
		digits[3] = 0;
		digits[4] = ((int)hum) / 10;
		digits[5] = ((int)hum) % 10;
	}

}
#endif

long blink_nextSW_t=0;
long nextBlink_can_t=0;
#define blink_d	500;
void loop() {
	//digitalWrite(LED_colon,!digitalRead(btn_swap_time_date));
	
	if(millis()>  blink_nextSW_t)
	{
		bShowDigit=!bShowDigit; // инвертируем флаг мигающей цифры
		blink_nextSW_t=millis()+blink_d;
				 
		//#	digitalWrite(LED_colon,bShowDigit);
	}
	
	if (millis()> nextReadRTC) // если прошло 500мс
	{
		nextReadRTC=millis()+50;
		tstTimerStart
		ReadDS3231(); 
		tstTimerPrint("ReadDS3231")

		#ifdef HT
		 hum = dht.readHumidity();
		// Read temperature as Celsius (the default)
		 temper = dht.readTemperature();
		#endif
		
		#ifdef tst
		Serial.print("remain time before switch mode  ");
		Serial.print(millis()/1000); Serial.print("    ");	Serial.println(bDateShow_untill_t);
		Serial.print("show date = "); Serial.println(bDateShow);
		
		#ifdef HT
		Serial.print(hum); Serial.print("  HT  ");Serial.println(temper);
		#endif
		#endif

	}
	
	bool bDateShow_last=bDateShow;
	if(mode==mode_show)
	{

		if(millis()/1000>bDateShow_untill_t) bDateShow=false;
		// #ifdef irLED
		// tone(irLED,38000);
		// #endif
		if(!digitalRead(btn_swap_time_date) tstSerSensor)
		{
			#ifdef tst
			Serial.println("change mode by sensor btn");
			#endif

			bDateShow_untill_t=millis()/1000+bDateShow_untill_td;
			
			#ifdef DateShow_constant
			bDateShow=!bDateShow;
			#else
			bDateShow=true;
			#endif
			#ifdef tst
			Serial.print("show date = "); Serial.println(bDateShow);
			#endif
		}
		// #ifdef irLED
		// noTone(irLED);
		// #endif
	}
	
	if(bDateShow!=bDateShow_last) 
	{
		memcpy(digits_last, digits, 6*sizeof(byte)); //for animate change to current while Date-Time show mode change
		fill_digits_withTimeOrDate();
		
		tstTimerStart
		if(mode==mode_show)
			DateShow_onChange();
		else
		{
			byte d=	DateShow_onChange_length;
			DateShow_onChange_length=15;
			DateShow_onChange();
			DateShow_onChange_length=d;
		}
		tstTimerPrint("DateShow_onChange")
	}

	
	#ifdef btn_backlight
	if(!digitalRead(btn_backlight))
	{
		bbacklight_untill_t=millis()/1000+10;
	}
	#endif
	digitalWrite(backlight_out, bBacklight 
	#ifdef btn_backlight
	|| (millis()/1000<bbacklight_untill_t)
	#endif
	);
	


	//проверяем нажатые кнопки
	if(millis()>nextBtnRead) //если разрешено, считываем нажатую кнопку
	{
		
		/*keyval - значение функции analogread 
	* если нажата первая кнопка, то принимает значение около 200
	* если нажата вторая кнопка то значение около 700
	* если третяя кнопка, то значение будет около 1000
	* эти значения зависят от выбранных резисторов в клавиатуре
	*/
		int keyval= analogRead(keypad); // считываем значение с клавиатуры
		if (keyval>150 && keyval<400) keynum=1;
		else
		if (keyval>700 && keyval<900) keynum=2;
		else
		if (keyval>960) keynum=3;
		else keynum=0;
		nextBtnRead=millis()+100;
		if(keynum>0)
		{
			nextBtnRead+=200;
			nextBlink_can_t=millis()+400;
		}

		
		#ifdef tst
		if(keyval>100)
		{
			Serial.print("btn voltage = "); Serial.println(keyval*5/1024);
			Serial.print("key = "); Serial.println(keynum);
		}

		#endif

		if (keynum==0)
		{
			//!!off
		}
		else
		{
			if (keynum==key_Menu_modeChange) // кнопка переключения между режимами    отображение, выбор цифры, изменение цифры
			{

				mode++;			

				if(mode>mode_M) mode=0;
				#ifdef tst
				Serial.print("btn MODE pressed. now mode = ");	Serial.println(mode);
				#endif

				switch(mode)
				{

				case mode_show:		bDateShow=false; break;

				case modeDay:		currentdigit=0; bDateShow=true;	break;
				case modeMon:		currentdigit=1;	bDateShow=true; break;
				case modeYear:		currentdigit=2;	bDateShow=true; break;
				case modeH:			currentdigit=0;	bDateShow=false; break;
				case modeMin:		currentdigit=1;	bDateShow=false; break;

				default: 
					mode=0;
					break;
				}
			}
			else //{if (keynum==key_Sub || keynum==key_Add_BacklightSw)}
			{
				int incr= (keynum==key_Sub) ? -1:1;

				switch(mode)
				{
				case mode_show:
				bBacklight=!bBacklight;
				#ifdef HT
					if(incr==-1)
					{
						bShowHT=!bShowHT;
						showHT_untill_t=millis()+5000;
					}
				#endif
				break;
					
					
				case modeDay:
					date+=incr;
					if (date>31) date=1;
					else
					if (date<1) date=31;
					Clock.setDate(date);
					break;
				case modeMon:
					month+=incr;
					if (month>12) month=1;
					else
					if (month<1) month=12;
					Clock.setMonth(month);
					break;
				case modeYear:
					year+=incr;
					if (year>99) year=0;
					else
					if (year<0) year=99;
					Clock.setYear(year);
					break;


				case modeH:
					hour+=incr;
					if (hour>23) hour=0;
					else
					if (hour<0) hour=23;
					Clock.setHour(hour);
					break;					
				case modeMin:
					minute+=incr;
					if (minute>59) minute=0;
					else
					if (minute<0) minute=59;
					Clock.setMinute(minute);
					Clock.setSecond(0);
					break;
				}
			}
		}
	}

	// ==================================================================================
#ifdef HT
	if(mode==mode_show && bShowHT&& showHT_untill_t>millis())
		fill_digits_withHT();
	else
#endif
		fill_digits_withTimeOrDate();


	//антиотравление
	if (minute%10!=lastMinute && mode==0)
	{
		digits[3] = lastMinute;
		lastMinute = minute%10;

		Safe(digits);
		digits[3] = lastMinute;
	}

	//if(bDateShow!=bDateShow_last) DateShow_onChange();
	//else
	{
		show(digits); // вывести цифры на дисплей
		show(digits);
	}
}

void showDigit(byte key)
{
	digitalWrite(key,HIGH);
	delay(2);
	digitalWrite(key,LOW); //потушим  индикатор
	delay(1);
}

void show(byte hhmmss[])
{
	setNumber(hhmmss[0]); //выведем цифру hhmmss[0] на первый индикатор
	if( (mode==mode_show) || (currentdigit!=0) || bShowDigit || millis()<nextBlink_can_t )	showDigit(key1); //если мы в режиме настройки и происходит настройка часов, то в первая цифра будет мигать

	setNumber(hhmmss[1]); //цифра hhmmss[1] на второй индикатор
	if((mode==mode_show) || (currentdigit!=0) || bShowDigit || millis()<nextBlink_can_t )	showDigit(key2);
#ifdef HT
if(!(mode==mode_show && bShowHT&& showHT_untill_t>millis()))
#endif
{
	setNumber(hhmmss[2]);
	if((mode==mode_show) || (currentdigit!=1) || bShowDigit || millis()<nextBlink_can_t )	showDigit(key3);

	setNumber(hhmmss[3]);
	if((mode==mode_show) || (currentdigit!=1) || bShowDigit || millis()<nextBlink_can_t )	showDigit(key4);
}
	setNumber(hhmmss[4]);
	if((mode==mode_show) || (currentdigit!=2) || bShowDigit || millis()<nextBlink_can_t )	showDigit(key5);

	setNumber(hhmmss[5]);
	if((mode==mode_show) || (currentdigit!=2) || bShowDigit || millis()<nextBlink_can_t )	showDigit(key6);
}
// передача цифры на дешифратор
void setNumber(int num) 
{
	switch (num)
	{
	case 0:
		digitalWrite (out1,LOW);
		digitalWrite (out2,LOW);
		digitalWrite (out4,LOW);
		digitalWrite (out8,LOW);
		break;
	case 1:
		digitalWrite (out1,HIGH);
		digitalWrite (out2,LOW);
		digitalWrite (out4,LOW);
		digitalWrite (out8,LOW);
		break;
	case 2:
		digitalWrite (out1,LOW);
		digitalWrite (out2,HIGH);
		digitalWrite (out4,LOW);
		digitalWrite (out8,LOW);
		break;
	case 3:
		digitalWrite (out1,HIGH);
		digitalWrite (out2,HIGH);
		digitalWrite (out4,LOW);
		digitalWrite (out8,LOW);
		break;
	case 4:
		digitalWrite (out1,LOW);
		digitalWrite (out2,LOW);
		digitalWrite (out4,HIGH);
		digitalWrite (out8,LOW);
		break;
	case 5:
		digitalWrite (out1,HIGH);
		digitalWrite (out2,LOW);
		digitalWrite (out4,HIGH);
		digitalWrite (out8,LOW);
		break;
	case 6:
		digitalWrite (out1,LOW);
		digitalWrite (out2,HIGH);
		digitalWrite (out4,HIGH);
		digitalWrite (out8,LOW);
		break;
	case 7:
		digitalWrite (out1,HIGH);
		digitalWrite (out2,HIGH);
		digitalWrite (out4,HIGH);
		digitalWrite (out8,LOW);
		break;
	case 8:
		digitalWrite (out1,LOW);
		digitalWrite (out2,LOW);
		digitalWrite (out4,LOW);
		digitalWrite (out8,HIGH);
		break;
	case 9:
		digitalWrite (out1,HIGH);
		digitalWrite (out2,LOW);
		digitalWrite (out4,LOW);
		digitalWrite (out8,HIGH);
		break;
	}
}



void safeDigit(byte i, byte n[])
{
	byte t=n[i];
	n[i]++;
	while(n[i]!=t)
	{
		for(byte x=0;x<SafeLength;x++)
		{
			show(n);
		}
		n[i]++;
		if (n[i]>9) n[i]=0;
	}
}

void Safe(byte n[])
{
	for (int i = 0; i < 6; ++i)
	{
		safeDigit(i,n);
		//if(bDateShow) //skip seconds
	}
}

void test_printDigits()
{
	Serial.println("-----digits last, now------");
	Serial.print(digits_last[0]); Serial.print(" ");
	Serial.print(digits_last[1]); Serial.print(" ");
	Serial.print(digits_last[2]); Serial.print(" ");
	Serial.print(digits_last[3]); Serial.print(" ");
	Serial.print(digits_last[4]); Serial.print(" ");
	Serial.println(digits_last[5]);
	Serial.print(digits[0]); Serial.print(" ");
	Serial.print(digits[1]); Serial.print(" ");
	Serial.print(digits[2]); Serial.print(" ");
	Serial.print(digits[3]); Serial.print(" ");
	Serial.print(digits[4]); Serial.print(" ");
	Serial.println(digits[5]);

}
void DateShow_onChange() //change numbers untill reach target
{
	bool bContinue=true;
	while(bContinue)
	{
		#ifdef tst
		test_printDigits();
		#endif
		bContinue=false;
		for(byte i=0;i<6;i++)
		{
			if(digits_last[i]!=digits[i]) 
			{
				digits_last[i]++;
				if(digits_last[i]>9) digits_last[i]=0;
				bContinue=true;
			}
		}
		
		//tstTimerStart
		for(byte i = 0; i < DateShow_onChange_length; i++)
		{
			show(digits_last); delay(DateShow_onChange_d);
		}
		//tstTimerPrint("show(digits_last); delay(DateShow_onChange_d);")
	}
}
