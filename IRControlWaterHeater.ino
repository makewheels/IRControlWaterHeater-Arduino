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

#define IR_CODE_ANDROID_ON 1088254603
#define IR_CODE_ANDROID_OFF 3030512581
#define IR_CODE_ANDROID_HEAT_10_MIN 2998241093
#define IR_CODE_ANDROID_HEAT_20_MIN 3719633707
#define IR_CODE_ANDROID_HEAT_30_MIN 3932801309
#define IR_CODE_ANDROID_HEAT_40_MIN 2338481351
#define IR_CODE_ANDROID_HEAT_50_MIN 222279425
#define IR_CODE_ANDROID_HEAT_60_MIN 2347079943

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
    tmrpcm.speakerPin = PIN_SPEAKER;

    //SD卡
    if (!SD.begin(PIN_SD_CARD)) {
        Serial.println("SD fail");
    }

    //播放开机声音
    tmrpcm.setVolume(5);
    tmrpcm.play("h/a/startup.wav");
    while(tmrpcm.isPlaying()){
        ;
    }
//    tmrpcm.play("h/a/off.wav");
    //所有灯，一起亮一秒
//    digitalWrite(PIN_LED_GREEN,HIGH);
//    digitalWrite(PIN_LED_RED,HIGH);
//    delay(1000);
//    digitalWrite(PIN_LED_GREEN,LOW);
//    digitalWrite(PIN_LED_RED,LOW);

     speakNumber(35);
}

//加热开始时间
unsigned long startTime=0;
//加热时长
unsigned long duration=0;

void loop() {
    //如果有烧水任务
    if(duration>0){
        //获取现在时间
        unsigned long currentTime=millis();
        //已加热时长
        unsigned long heatedTime=0;
        //计算已加热时长
        if(currentTime>=startTime){
            heatedTime=currentTime-startTime;
        }else{
            //解决溢出问题
            heatedTime=4294967295-startTime+currentTime;
        }
        //如果到了指定时间
        if(heatedTime>=duration){
            //就关掉继电器
            digitalWrite(PIN_RELAY,LOW);
            //恢复duration
            duration=0;
            //播放时间语音，说明烧的时间
            
        }
    }
    //接收红外信号
    if (irrecv.decode(&irResults)) {
        long long code=irResults.value;
        Serial.println(irResults.value);
        //ON
        if(code==IR_CODE_ANDROID_ON){
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("h/a/on.wav");
            blinkForTime(PIN_LED_GREEN,2);
            //OFF
        }else if(code==IR_CODE_ANDROID_OFF){
            digitalWrite(PIN_RELAY,LOW);
            duration=0;
            tmrpcm.play("h/a/off.wav");
            blinkForTime(PIN_LED_GREEN,2);
        }else if(code==IR_CODE_ANDROID_HEAT_10_MIN){
            //加热指定时长
            //记录开始时间
            startTime=millis();
            //记录加热时长
            duration=HEAT_TIME_10_MIN;
            //接通继电器，开始加热
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("h/a/heat10.wav");
            blinkForTime(PIN_LED_GREEN,2);
        }else if(code==IR_CODE_ANDROID_HEAT_20_MIN){
            startTime=millis();
            duration=HEAT_TIME_20_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("h/a/heat20.wav");
            blinkForTime(PIN_LED_GREEN,2);
        }else if(code==IR_CODE_ANDROID_HEAT_30_MIN){
            startTime=millis();
            duration=HEAT_TIME_30_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("h/a/heat30.wav");
            blinkForTime(PIN_LED_GREEN,2);
        }else if(code==IR_CODE_ANDROID_HEAT_40_MIN){
            startTime=millis();
            duration=HEAT_TIME_40_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("h/a/heat40.wav");
            blinkForTime(PIN_LED_GREEN,2);
        }else if(code==IR_CODE_ANDROID_HEAT_50_MIN){
            startTime=millis();
            duration=HEAT_TIME_50_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("h/a/heat50.wav");
            blinkForTime(PIN_LED_GREEN,2);
        }else if(code==IR_CODE_ANDROID_HEAT_60_MIN){
            startTime=millis();
            duration=HEAT_TIME_60_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play("h/a/heat60.wav");
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

/**
 * 说一位数字
 */
void speakOneBitNumber(int number){
    if(number==1){
        tmrpcm.play("h/a/1.wav");
    }else if(number==2){
        tmrpcm.play("h/a/2.wav");
    }else if(number==3){
        tmrpcm.play("h/a/3.wav");
    }else if(number==4){
        tmrpcm.play("h/a/4.wav");
    }else if(number==5){
        tmrpcm.play("h/a/5.wav");
    }else if(number==6){
        tmrpcm.play("h/a/6.wav");
    }else if(number==7){
        tmrpcm.play("h/a/7.wav");
    }else if(number==8){
        tmrpcm.play("h/a/8.wav");
    }else if(number==9){
        tmrpcm.play("h/a/9.wav");
    }else if(number==10){
        tmrpcm.play("h/a/10.wav");
    }
}

/**
 * 播放语音数字
 */
void speakNumber(int number){
    int gewei=number%10;
    if(number<=10){
        speakOneBitNumber(number);
    }else if(number>=11&&number<=19){
        speakOneBitNumber(10);
        while(tmrpcm.isPlaying()){
            ;
        }
        speakOneBitNumber(number%10);        
    }else if(number>=20&&number<=99){
        speakOneBitNumber(number/10);
        while(tmrpcm.isPlaying()){
            ;
        }
        speakOneBitNumber(10);
        while(tmrpcm.isPlaying()){
            ;
        }
        if(gewei!=0){
            speakOneBitNumber(gewei);
        }
    }
}
