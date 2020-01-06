#include <MAX30100_PulseOximeter.h>
#include <Wire.h>
#include "Filter.h"
#include <SoftwareSerial.h>

//Đặt tên các chân của VDK
#define buttonPin 3
#define RED_PIN 4    // led đỏ
#define GREEN_PIN 6 // led xanh lá
#define BLUE_PIN 8  // led xanh dương

//Các thời gian của cảm biến
#define REPORTING_PERIOD_MS 200
#define MEASURE_PERIOD_MS 20000
#define INIT_PERIOD_MS 9000
#define SAMPLING_PERIOD_MS 1000

//Mảng chuyển giá trị từ cảm biến sang ký tự
#define MAXSIZE 3

//Thông số của bộ lọc cơ bản
#define OFFSET 50
#define STACK 4

//Đối tượng cảm biến
PulseOximeter pox;

//Mở thêm cổng Serial
SoftwareSerial mySerial(10, 11); // RX, TX

//Đối tượng bộ lọc cơ bản
FilterT myFilter;

//Các biến liên quan
uint32_t tsLastReport = 0;
char c = 'o';
char tmp = 'o';
uint32_t tsStartMeasure = 0;
int bpm = 0, spO2 = 0, maxBpm = 0, minBpm = 0, avgBpm = 0;
uint32_t tsCheckPoint = 0;

char buf[MAXSIZE];

boolean isRuning = false;

int buttonState = 0;

int ledMode = 1;

//Khởi động lại VDK khi cảm biến đo sai
void ResetBoard()
{
  asm volatile("jmp 0");
}

//In ra serial khi kết nối với arduino IDE
void serialPrint()
{
  Serial.print(maxBpm);
  Serial.print(" ");
  Serial.print(minBpm);
  Serial.print(" ");
  Serial.print(avgBpm);
  Serial.print(" ");
  Serial.println(spO2);
  Serial.println(); 
}

//Hàm điều khiển led
void noticeLed(int led)
{
  switch(led)
  {
    //Standby
    case 1:
    {
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, HIGH);
      break;
    }
    //Loading
    case 2:
    {
      digitalWrite(RED_PIN, HIGH);
      digitalWrite(GREEN_PIN, HIGH);
      digitalWrite(BLUE_PIN, LOW);
      break;
    }
    //Sending
    case 3:
    {
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, HIGH);
      digitalWrite(BLUE_PIN, LOW);
      break;
    }
    //Low signal
    case 4:
    {
      digitalWrite(RED_PIN, HIGH);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
      break;
    }
  }
}

//Hàm gửi dữ liệu
void sendDatas(int maxBmp, int minBmp, int avgBmp, int spO2)
{
  //gui data qua bluetooth
  mySerial.write('*');
  mySerial.write('S');
//  maxBpm
  itoa(maxBpm, buf, 10);
  mySerial.write(buf);
  mySerial.write('T');
//  minBpm
  itoa(minBpm, buf, 10);
  mySerial.write(buf);
  mySerial.write('T');
//  avgBpm
  itoa(avgBpm, buf, 10);
  mySerial.write(buf);
  mySerial.write('T');
//  spO2
  itoa(spO2, buf, 10);
  mySerial.write(buf);
  mySerial.write('A');
  mySerial.write('*');
}

//Hàm ngắt phát hiện beat
void onBeatDetected()
{
  Serial.println(" Beat!");
  mySerial.write("/BB/");
}

//Hàm cài đặt ban đầu, chỉ chạy 1 lần
void setup()
{
  Serial.begin(9600);
  Serial.print("Initializing pulse oximeter..");
  if (!pox.begin())
  {
    Serial.println("FAILED, Press reset button!!!");
    delay(1000);
    ResetBoard();
    //for(;;);
  } else 
  {
    Serial.println("SUCCESS");
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_11MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);

  pinMode(buttonPin, INPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  myFilter.init(OFFSET , STACK, 0, 120);

  mySerial.begin(9600);
  mySerial.write("*S45T45T45T45A*");
}

// Vòng lặp để chạy chương trình
void loop()
{
  //Luôn lấy dữ liệu từ cảm biến
  pox.update();
  
  //Sau 0.2s thì tính toán
  if (millis() - tsLastReport > REPORTING_PERIOD_MS)
  {
    //Đọc giá trị của nút bấm
    buttonState = digitalRead(buttonPin);
    if(buttonState == LOW && isRuning == false)
    {
      mySerial.write("/Y/");
      myFilter.init(OFFSET , STACK, 0, 120);
      isRuning = true;
      //c = 's';
      //Led báo "Loading"
      ledMode = 2;
      tsStartMeasure = millis();
    }

    //Lấy dữ liệu cảm biến
    bpm = pox.getHeartRate();
    spO2 = pox.getSpO2();    
    tsLastReport = millis();

    //Cập nhật trạng thái của đèn
    noticeLed(ledMode);
        
    //Lệnh tiến hành đo
    //if(c == 's')
    if(isRuning == true)
    {
      //Thêm bpm vào bộ lọc cơ bản nếu bmp > OFFSET
      if(bpm > OFFSET)
      {
        myFilter.add(round(bpm));
        Serial.print(bpm);
        Serial.print("-");
        Serial.println(spO2);
      }
      
      //Trước 10s, chưa làm gì thêm ngoài đo và thêm dữ liệu vào bộ lọc
      //10s tính từ lúc bấm đến lúc so sánh
      if(tsLastReport - tsStartMeasure < INIT_PERIOD_MS)
      {
        //Led báo "Loading"
        ledMode = 2;
      }
      
      //Từ 10s đến 20s, bắt đầu lọc và gửi dữ liệu đi
      else
      {
        //Led báo "Sending"
        ledMode = 3;
        
        //Sau mỗi 1s trong khoảng từ 10s đến 20s
        if(tsLastReport - tsCheckPoint >= SAMPLING_PERIOD_MS)
        {
          //Chạy bộ lọc cơ bản
          myFilter.filtering(&maxBpm, &minBpm, &avgBpm);

          //Sai, đo lại
          if(maxBpm < minBpm || avgBpm < myFilter.offSet)
          {
            serialPrint();
            
            myFilter.log();
            Serial.println("Low signal, try again!_10-20");
            mySerial.write("/E/");

            //Trở lại ban đầu
            isRuning = false;
            //c = 'o';

            //Led báo "Error" luôn và chờ 2s
            noticeLed(4);
            delay(2000);
            ResetBoard();
          }

          //Đúng, gửi đi
          else
          {
            sendDatas(maxBpm, minBpm, avgBpm, spO2);
          }

          //Lấy thời điểm hiện tại để so sánh lần sau
          tsCheckPoint = millis();
        }
      }

      //Hết 20s, kết thúc
      if(tsLastReport - tsStartMeasure >= MEASURE_PERIOD_MS)
      {
          myFilter.filtering(&maxBpm, &minBpm, &avgBpm);

          //Sai, đo lại
          if(maxBpm < minBpm || avgBpm < myFilter.offSet)
          {
            Serial.println("Low signal, try again!_20");
            mySerial.write("/E/");

            //Led báo "Error" luôn và chờ 2s
            noticeLed(4);
            delay(2000);
            ResetBoard();
          }

          //Đúng, gửi đi kèm thông báo kết thúc
          else
          {
            serialPrint();
            sendDatas(maxBpm, minBpm, avgBpm, spO2);
            mySerial.write("/DD/");
            myFilter.log();
          }

          Serial.println("End");
          //Trở lại ban đầu
          isRuning = false;
          ledMode = 1;
          //c = 'o';
       }
    }

    tsLastReport = millis();
  }
}
