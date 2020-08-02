#include <IRremote.h>
#include <SD.h>
#include <TMRpcm.h>
//#include <WString.h>
//#include <avr/pgmspace.h>
#include <EEPROM.h>

/**
 * 红外线接线，从左到右：
 * 输出信号，GND，VCC-5V
 */
 
#define PIN_IR 2        //红外线
#define PIN_RELAY 3     //继电器
#define PIN_SD_CARD 4   //SD卡
#define PIN_LED_GREEN 5 //绿色指示灯
#define PIN_LED_RED 6   //红色指示灯
#define PIN_SPEAKER 10  //音响

#define IR_CODE_ON 1088254603
#define IR_CODE_OFF 3030512581
#define IR_CODE_HEAT_10_MIN 2998241093
#define IR_CODE_HEAT_20_MIN 3719633707
#define IR_CODE_HEAT_30_MIN 3932801309
#define IR_CODE_HEAT_40_MIN 2338481351
#define IR_CODE_HEAT_50_MIN 222279425
#define IR_CODE_HEAT_60_MIN 2347079943

#define HEAT_TIME_10_MIN 5000   //10*60*1000
#define HEAT_TIME_20_MIN 1200000
#define HEAT_TIME_30_MIN 1800000
#define HEAT_TIME_40_MIN 2400000
#define HEAT_TIME_50_MIN 3000000
#define HEAT_TIME_60_MIN 3600000

TMRpcm tmrpcm;
IRrecv irrecv(PIN_IR);

decode_results irResults;

void setup() {
    Serial.begin(9600);
    pinMode(PIN_RELAY,OUTPUT);
    pinMode(PIN_LED_GREEN,OUTPUT);
    pinMode(PIN_LED_RED,OUTPUT);

    //红外线
    irrecv.enableIRIn();

    //音响
    tmrpcm.speakerPin = 9;

    //SD卡
    if (!SD.begin(PIN_SD_CARD)) {
        Serial.println("SD fail");
    }

    //所有灯，一起亮一秒
    digitalWrite(PIN_LED_GREEN,HIGH);
    digitalWrite(PIN_LED_RED,HIGH);
    delay(1000);
    digitalWrite(PIN_LED_GREEN,LOW);
    digitalWrite(PIN_LED_RED,LOW);

    //播放开机声音
    tmrpcm.setVolume(6);
    tmrpcm.play("heat/a/startup.wav");
}

//加热开始时间
unsigned long startTime=0;
//加热时长
unsigned long duration=0;

void loop() {
    Serial.println(duration);
    //如果有烧水任务
    if(duration>0){
        //看是不是已经到时间了
        unsigned long currentTimeMillis=millis();
        //如果到了指定时间
        if(currentTimeMillis-startTime>=duration){
            //就关掉继电器
            digitalWrite(PIN_RELAY,LOW);
            //恢复duration
            duration=0;
        }
    }
    //接收红外信号
    if (irrecv.decode(&irResults)) {
        long long code=irResults.value;
        Serial.println(irResults.value);
        //ON
        if(code==IR_CODE_ON){
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("heat/a/on.wav");
            blinkForTime(PIN_LED_GREEN,2);
            //OFF
        }else if(code==IR_CODE_OFF){
            digitalWrite(PIN_RELAY,LOW);
            duration=0;
            tmrpcm.play("heat/a/off.wav");
            blinkForTime(PIN_LED_GREEN,2);
        }else if(code==IR_CODE_HEAT_10_MIN){
            //加热指定时长
            //记录开始时间
            startTime=millis();
            //记录加热时长
            duration=HEAT_TIME_10_MIN;
            //接通继电器，开始加热
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("heat/a/heat10.wav");
            blinkForTime(PIN_LED_GREEN,2);
        }else if(code==IR_CODE_HEAT_20_MIN){
            startTime=millis();
            duration=HEAT_TIME_20_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("heat/a/heat20.wav");
            blinkForTime(PIN_LED_GREEN,2);
        }else if(code==IR_CODE_HEAT_30_MIN){
            startTime=millis();
            duration=HEAT_TIME_30_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("heat/a/heat30.wav");
            blinkForTime(PIN_LED_GREEN,2);
        }else if(code==IR_CODE_HEAT_40_MIN){
            startTime=millis();
            duration=HEAT_TIME_40_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("heat/a/heat40.wav");
            blinkForTime(PIN_LED_GREEN,2);
        }else if(code==IR_CODE_HEAT_50_MIN){
            startTime=millis();
            duration=HEAT_TIME_50_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("heat/a/heat50.wav");
            blinkForTime(PIN_LED_GREEN,2);
        }else if(code==IR_CODE_HEAT_60_MIN){
            startTime=millis();
            duration=HEAT_TIME_60_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("heat/a/heat60.wav");
            blinkForTime(PIN_LED_GREEN,2);
        }else{
            //没有找到红外线指令
            for(int i=0;i<3;i++){
                digitalWrite(PIN_LED_RED,HIGH);
                delay(150);
                digitalWrite(PIN_LED_RED,LOW);
                delay(65);
            }
        }
        irrecv.resume();
    }
    delay(100);
}

/**
 * 点亮指定时长
 */
//void lightUpForTime(int pin,long lightUpTime){
//    digitalWrite(pin,HIGH);
//    delay(lightUpTime);
//    digitalWrite(pin,LOW);
//}

/**
 * 闪烁
 */
void blinkForTime(int pin,int times){
    for(int i=0;i<times;i++){
        digitalWrite(pin,HIGH);
        delay(230);
        digitalWrite(pin,LOW);
        delay(70);
    }
}
