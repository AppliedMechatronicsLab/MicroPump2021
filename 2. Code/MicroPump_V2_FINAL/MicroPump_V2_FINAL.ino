#include "avr/interrupt.h"
#include <string.h>

#define ena 11
#define dir 12
#define pul 13
#define dungKhanCap 2
#define Buzzers 6

// cac thong so
#define McSt 256   // Microstep
#define l 0.2      // lamda
#define degree 1.8 // goc full-step


int check; //bien kiem tra
int trangThaiNgat; // 1 = dang ngat , 0 = khong ngat
int Direction; //chieu dong co
int checkMotor; // kiem tra xem co dang trong mode push pull hay khong
long countPulse, totalPulse, State, Previous;
long Butt_State, Butt_Previous = 0; 

float counterInit;

int theTichXilanh = 1; // the tich xilanh
int type = 1;  // loai xilanh
float dungTichDungDich = 1;      // thong so dung totalPulse co trong xilanh
float tocDoBom = 1;        // thong so toc do bom
float hanhTrinhXilanh = 1; // thong so hanh trinh xilanh
float dungDichDaBom;
float heSoDonVi = 1;

String DataFromDisplay = "";  // du lieu tu man hinh
String DonVi = "ml/h";  // xau chua dang don vi

void setup()
{
  Serial.begin(9600);

  pinMode(ena, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(pul, OUTPUT);
  pinMode(dungKhanCap, INPUT_PULLUP);
  
  pinMode(Buzzers, OUTPUT);

  attachInterrupt(0, ngatKhanCap, FALLING);

  digitalWrite(ena, HIGH);
}

void ngatKhanCap()
{
  Butt_State = millis();
 if ((Butt_State - Butt_Previous) >= 500)
 {
    Butt_Previous = Butt_State;
  if (1 == checkMotor)    // neu dang trong mode chay
  {
    if (0 == trangThaiNgat)    // ngat
    {
      Serial.print("push.state.txt=\"STATE: PAUSING\"");
      CachDong(); 
      Serial.print("pull.state.txt=\"STATE: PAUSING\"");
      CachDong();

      digitalWrite(ena, LOW);
      trangThaiNgat = 1;
      TimerDisable();
      check = 0;
    }
    else              //bat lai  
    {
      Serial.print("push.state.txt=\"STATE: PUSHING\"");
      CachDong(); 
      Serial.print("pull.state.txt=\"STATE: PULLING\"");
      CachDong();
      digitalWrite(ena, HIGH);
      trangThaiNgat = 0;
      TimerEnable();
    }   
  }
 }
}

void PlayPause()
{
  if (1 == checkMotor)  // neu dang trong mode chay
  {
    if (0 == trangThaiNgat)    // ngat
    {
      trangThaiNgat = 1;
      TimerDisable();
      check = 0;
    }
    else              //bat lai  
    {
      digitalWrite(ena, HIGH);
      trangThaiNgat = 0;
      TimerEnable();
    }
  }
}

void TimerEnable()   //cau hinh timer va bat len
{
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 = 0;

  TCCR1B = (0 << CS12) | (1 << CS11) | (1 << CS10);  // cau hinh 3 bit chinh precaler 1/64
  TCNT1 = counterInit;   // dat bottom
  TIMSK1 = (1 << TOIE1); // bat su kien ngat
  sei();
}

void TimerDisable()  //tat timer
{
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 = 0;

  TCCR1B |= (0 << CS12) | (0 << CS11) | (0 << CS10);
  TCNT1 = counterInit;
  TIMSK1 = (1 << TOIE1);
  sei();
}

void CachDong()   //viet ki hieu de Nextion nhan biet xuong dong
{
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}

ISR(TIMER1_OVF_vect)  //khi counter tran
{
  digitalWrite(13, !digitalRead(13));
  countPulse++;
  TCNT1 = counterInit; 
}

void GetDataFromDisplay()  //lay du lieu tu man hinh
{
  if (Serial.available())
  {
    DataFromDisplay = "";
    delay(30);
    while (Serial.available())
    {
      DataFromDisplay += char(Serial.read());
    }
   
    XuLy(DataFromDisplay);
  }
}

int XuLy(String DataFromDisplay)  // xu ly du lieu thu duoc
{
  if (DataFromDisplay.substring(0, 8) == "capacity") // DataFromDisplay = "capacity_xxxx"
  {
    SetCapacity(DataFromDisplay.substring(8));
    return 0;
  }

  if (DataFromDisplay.substring(0, 5) == "speed")  //DataFromDisplay = "speedxxxx"
  {
    SetSpeed(DataFromDisplay.substring(5));
    return 0;
  }

  if (DataFromDisplay == "ml/h")
  {
    heSoDonVi = 1;
    DonVi = "ml/h";
    return 0;
  }

  if (DataFromDisplay == "ul/min")
  {
    heSoDonVi = pow(10, 6) / 60;
    DonVi = "ul/min";
    return 0;
  }

  if (DataFromDisplay.substring(0, 3) == "1ml")
  {
    theTichXilanh = 1;
    hanhTrinhXilanh = 5.78;
    return 0;
  }

  if (DataFromDisplay.substring(0, 3) == "5ml")
  {
    theTichXilanh = 5;
    hanhTrinhXilanh = 4.08;
    return 0;
  }

  if (DataFromDisplay.substring(0, 4) == "10ml")
  {
    theTichXilanh = 10;
    hanhTrinhXilanh = 5.48;
    return 0;
  }

  if (DataFromDisplay == "push")
  {
    Push();
    return 0;
  }

  if (DataFromDisplay == "pull")
  {
    Pull();
    return 0;
  }

  if (DataFromDisplay == "vao")  //manual
  {
    digitalWrite(ena, HIGH);    
    digitalWrite(dir, 1);
    counterInit = 65529;
    TimerEnable();
    return 0;
  }

  if (DataFromDisplay == "ra") //manual
  {
    digitalWrite(ena, HIGH);    
    digitalWrite(dir, 0);
    counterInit = 65529;
    TimerEnable();
    return 0;
  }

  if (DataFromDisplay == "ngung")  //tin hieu ngung manual
  {
    TimerDisable();
    return 0;
  }

// ham if 
  if (DataFromDisplay == "pause")  // play/pause khi push pull
  {
    PlayPause();
    return 0;
  }

  if (DataFromDisplay == "back")  //thoat che do push pull tren giao dien
  {
    checkMotor = 0;
    return 0;
  }
}

void SetCapacity(String str)
{
  //chuyen kieu String sang mang ki tu de dung ham atoif()
  int str_len = str.length() + 1;
  char char_array[str_len];
  str.toCharArray(char_array, str_len);
  dungTichDungDich = atof(char_array);
  
}

void SetSpeed(String str)
{
  //chuyen kieu String sang mang ki tu de dung ham atoif()
  int str_len = str.length() + 1;
  char char_array[str_len];
  str.toCharArray(char_array, str_len);
  tocDoBom = atof(char_array);
 
}


void Push() //chế độ bơm
{
  Direction = 0;
  digitalWrite(ena, HIGH);
  digitalWrite(dir, Direction);

  check = 0;
  countPulse = 0;
  trangThaiNgat = 0;

  counterInit = heSoDonVi * (65538 - ((degree * theTichXilanh * 10 * 16000000 * l) / (tocDoBom * hanhTrinhXilanh * McSt * 64))); // tinh bang don vi mm   //tinh gia tri khoi tao cho TCNT1 
  totalPulse = (dungTichDungDich * hanhTrinhXilanh * 360 * McSt / (theTichXilanh * l * degree));  //tong so xung can phat
  countPulse = 0;   
  TimerEnable();

  checkMotor = 1;  //đánh dấu là đang chạy chế độ push/pull
  Serial.print("push.state.txt=\"STATE: PUSHING\"");
  CachDong();
  Serial.print("push.capacity.txt=\"Capacity=" + String(dungTichDungDich) + "ml\"");
  CachDong();
  Serial.print("push.speed.txt=\"Speed=" + String(tocDoBom)+ " " + DonVi +"\"");
  CachDong();
  Serial.print("push.type.txt=\"Type:" + String(theTichXilanh) + "ml\"");
  CachDong();

  while ((1 == checkMotor) && (countPulse < totalPulse))
  {
    while ((0 == trangThaiNgat) && (countPulse < totalPulse))
    {
      GetDataFromDisplay();
     if (0 == checkMotor)  // khi nguoi dung nhan "back"
     {
      break;
     } 
     
      State = millis();
     if ((State - Previous) > 150)   // 0.5s refresh thong tin tren man 1 lan          // print trang thai
     {  
      Serial.print("push.state.txt=\"STATE: PUSHING\"");
      CachDong(); 
      dungDichDaBom = (countPulse * dungTichDungDich) / totalPulse;
      Serial.print("push.ed.txt=\"" + String(dungDichDaBom) + "\"");
      CachDong();
      Serial.print("push.ConLai.txt=\"" + String(dungTichDungDich - dungDichDaBom) + "\"");
      CachDong();
      Serial.print("push.j0.val=" + String(round((dungDichDaBom / dungTichDungDich) * 100)));
      CachDong();
      Previous = State;
     } 
    }
    
    GetDataFromDisplay();

  if (check == 0)
   {
    Serial.print("push.state.txt=\"STATE: PAUSING\"");
    CachDong();
    check = 1;
   }  
  }
  TimerDisable();
  Serial.print("push.j0.val=" + String(100));
  CachDong();
  Serial.print("push.state.txt=\"STATE: Done\"");  // chay xong
  CachDong();
  dungDichDaBom = (countPulse / totalPulse)* dungTichDungDich;
  Serial.print("push.ed.txt=\"" + String(dungDichDaBom) + "\"");
  CachDong();
  Serial.print("push.ConLai.txt=\"0\"");
  CachDong();
  Serial.print("push.j0.val=" + String(round((dungDichDaBom / dungTichDungDich) * 100)));
  CachDong();

 if (checkMotor == 1)
 {
  analogWrite(Buzzers, 255);
  delay(1000);
  analogWrite(Buzzers, 0);
  delay(400);
  analogWrite(Buzzers, 255);
  delay(1000);
  analogWrite(Buzzers, 0);
  delay(400);
  analogWrite(Buzzers, 255);
  delay(1000);
  analogWrite(Buzzers, 0);    
 } 

  checkMotor = 0;
  State = 0;
  Previous = 0;

}

void Pull() // chế độ hút
{
  Direction = 1;
  digitalWrite(ena, HIGH);
  digitalWrite(dir, Direction);

  check = 0;
  countPulse = 0;
  trangThaiNgat = 0;

  counterInit = heSoDonVi * (65538 - ((degree * theTichXilanh * 10 * 16000000 * l) / (tocDoBom * hanhTrinhXilanh * McSt * 64))); // tinh bang don vi mm
  totalPulse = (dungTichDungDich * hanhTrinhXilanh * 360 * McSt / (theTichXilanh * l * degree));
  countPulse = 0;  
  TimerEnable();

  checkMotor = 1;  //danh dau dang chay push/pull

  Serial.print("pull.state.txt=\"STATE: PULLING\"");
  CachDong();
  Serial.print("pull.capacity.txt=\"Capacity=" + String(dungTichDungDich) + "ml\"");
  CachDong();
  Serial.print("pull.speed.txt=\"Speed=" + String(tocDoBom) + DonVi +"\"");
  CachDong();
  Serial.print("pull.type.txt=\"Type:" + String(theTichXilanh) + "ml\"");
  CachDong();

  while ((1 == checkMotor) && (countPulse < totalPulse))
  {
    while ((0 == trangThaiNgat) && (countPulse < totalPulse))
    {
      GetDataFromDisplay();
     if (0 == checkMotor)  //kiem tra xem dang chay dong co khong
     {
       break;
     }

      State = millis();
     if ((State - Previous) > 150)    //0.5s refresh thong tin tren man 1 lan    // print trang thai
     { 
      Serial.print("pull.state.txt=\"STATE: PULLING\"");
      CachDong();
      dungDichDaBom = (countPulse * dungTichDungDich) / totalPulse; 
      Serial.print("pull.ed.txt=\"" + String(dungDichDaBom) + "\"");
      CachDong();
      Serial.print("pull.ConLai.txt=\"" + String(dungTichDungDich - dungDichDaBom) + "\"");
      CachDong();
      Serial.print("pull.j0.val=" + String(round((dungDichDaBom / dungTichDungDich) * 100)));
      CachDong();
      Previous = State;
     } 
    }
   
    GetDataFromDisplay();

  if (check == 0)
   {
    Serial.print("pull.state.txt=\"STATE: PAUSING\"");
    CachDong();
    check = 1;
   } 
  }
  TimerDisable();
  Serial.print("pull.j0.val=" + String(100));
  CachDong();  
  Serial.print("pull.state.txt=\"STATE: Done\"");   //chay xong
  CachDong();
  dungDichDaBom = (countPulse * dungTichDungDich) / totalPulse; 
  Serial.print("pull.ed.txt=\"" + String(dungDichDaBom) + "\"");
  CachDong();
  Serial.print("pull.ConLai.txt=\"0\"");
  CachDong();
  Serial.print("pull.j0.val=" + String(round((dungDichDaBom / dungTichDungDich) * 100)));
  CachDong();  

 if (checkMotor == 1)
 {
  analogWrite(Buzzers, 255);
  delay(1000);
  analogWrite(Buzzers, 0);
  delay(400);
  analogWrite(Buzzers, 255);
  delay(1000);
  analogWrite(Buzzers, 0);
  delay(400);
  analogWrite(Buzzers, 255);
  delay(1000);
  analogWrite(Buzzers, 0);    
 } 

  checkMotor = 0;  
  State = 0;
  Previous = 0;

}

void loop()
{
  GetDataFromDisplay();
}
