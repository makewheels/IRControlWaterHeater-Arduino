#include <IRremote.h>
#include <SD.h>
#include <TMRpcm.h>
#include <WString.h>
//#include <avr/pgmspace.h>

/**
 * 红外线接线，从左到右：
 * 输出信号，GND，VCC-5V
 */

#define PIN_IR 2        //红外线
#define PIN_RELAY 3     //继电器
#define PIN_SPEAKER 9   //音响
#define PIN_SD_CARD 4   //SD卡
#define PIN_LED 7       //指示灯

#define IR_CODE_ON 1088254603
#define IR_CODE_OFF 3030512581
#define IR_CODE_HEAT_10_MIN 2998241093
#define IR_CODE_HEAT_20_MIN 3719633707
#define IR_CODE_HEAT_30_MIN 3932801309
#define IR_CODE_HEAT_40_MIN 2338481351
#define IR_CODE_HEAT_50_MIN 222279425
#define IR_CODE_HEAT_60_MIN 2347079943

#define HEAT_TIME_10_MIN 30*1000   //10*60*1000
#define HEAT_TIME_20_MIN 20*60*1000
#define HEAT_TIME_30_MIN 30*60*1000
#define HEAT_TIME_40_MIN 40*60*1000
#define HEAT_TIME_50_MIN 50*60*1000
#define HEAT_TIME_60_MIN 60*60*1000

TMRpcm tmrpcm;
IRrecv irrecv(PIN_IR);

decode_results irResults;

void setup() {
    Serial.begin(9600);
    pinMode(PIN_RELAY,OUTPUT);
    pinMode(PIN_LED,OUTPUT);

    //红外线
    irrecv.enableIRIn();
    Serial.println(F("Enabled IRin"));

    //音响
    tmrpcm.speakerPin = 9;

    //SD卡
    if (!SD.begin(PIN_SD_CARD)) {
        Serial.println(F("SD fail"));
    }

    lightUpForTime(PIN_LED,1000);
    
    tmrpcm.setVolume(6);
    tmrpcm.play("heat/a/startup.wav");
}

unsigned long startTime=0;
unsigned long duration=0;

void loop() {
    //如果有烧水任务
    if(duration>0){
        //看是不是已经到时间了
        unsigned long currentTimeMillis=millis();
        //如果到了时间
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
            //OFF
        }else if(code==IR_CODE_OFF){
            digitalWrite(PIN_RELAY,LOW);
            duration=0;
            tmrpcm.play("heat/a/off.wav");
        }else if(code==IR_CODE_HEAT_10_MIN){
            //加热指定时长
            //记录开始时间
            startTime=millis();
            //记录加热时长
            duration=HEAT_TIME_10_MIN;
            //接通继电器，开始加热
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("heat/a/heat10.wav");
        }else if(code==IR_CODE_HEAT_20_MIN){
            startTime=millis();
            duration=HEAT_TIME_20_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("heat/a/heat20.wav");
        }else if(code==IR_CODE_HEAT_30_MIN){
            startTime=millis();
            duration=HEAT_TIME_30_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("heat/a/heat30.wav");
        }else if(code==IR_CODE_HEAT_40_MIN){
            startTime=millis();
            duration=HEAT_TIME_40_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("heat/a/heat40.wav");
        }else if(code==IR_CODE_HEAT_50_MIN){
            startTime=millis();
            duration=HEAT_TIME_50_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("heat/a/heat50.wav");
        }else if(code==IR_CODE_HEAT_60_MIN){
            startTime=millis();
            duration=HEAT_TIME_60_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("heat/a/heat60.wav");
        }
        irrecv.resume();
        blinkForTime(PIN_LED,2);
    }
    delay(100);
}

/**
 * 点亮
 */
void lightUpForTime(int pin,long lightUpTime){
    digitalWrite(pin,HIGH);
    delay(lightUpTime);
    digitalWrite(pin,LOW);
}

/**
 * 闪烁
 */
void blinkForTime(int pin,int times){
    for(int i=0;i<times;i++){
        digitalWrite(pin,HIGH);
        delay(250);
        digitalWrite(pin,LOW);
        delay(80);
    }
}
