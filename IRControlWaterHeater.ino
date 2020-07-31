#include <IRremote.h>
#include <SD.h>
#include <TMRpcm.h>
#include <WString.h>
#include <avr/pgmspace.h>

#define PIN_IR 2        //红外线
#define PIN_RELAY 3     //继电器
#define PIN_SPEAKER 9   //音响
#define PIN_SD_CARD 4   //SD卡

#define IR_CODE_ON 1088254603
#define IR_CODE_OFF 3030512581
#define IR_CODE_HEAT_10_MIN 2998241093
#define IR_CODE_HEAT_20_MIN 3719633707
#define IR_CODE_HEAT_30_MIN 3932801309
#define IR_CODE_HEAT_40_MIN 2338481351
#define IR_CODE_HEAT_50_MIN 222279425
#define IR_CODE_HEAT_60_MIN 2347079943

#define HEAT_TIME_10_MIN 30*1000 //10*60*1000
#define HEAT_TIME_20_MIN 20*60*1000
#define HEAT_TIME_30_MIN 30*60*1000
#define HEAT_TIME_40_MIN 40*60*1000
#define HEAT_TIME_50_MIN 50*60*1000
#define HEAT_TIME_60_MIN 60*60*1000

IRrecv irrecv(PIN_IR);
TMRpcm tmrpcm;
decode_results irResults;

void setup() {
    Serial.begin(9600);
    pinMode(PIN_RELAY,OUTPUT);

    //红外线
    irrecv.enableIRIn();
    Serial.println(F("Enabled IRin"));

    //音响
    tmrpcm.speakerPin = PIN_SPEAKER;

    //SD卡
    if (!SD.begin(PIN_SD_CARD)) {
        Serial.println(F("SD fail"));
    }

    const static PROGMEM char startup[] = "irheat/audios/system/startup.wav";
    tmrpcm.play(startup);
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
    if (irrecv.decode(&irResults)) {
        long long code=irResults.value;
        Serial.println(irResults.value);
        //ON
        if(code==IR_CODE_ON){
            digitalWrite(PIN_RELAY,HIGH);
            //  tmrpcm.setVolume(6);
            tmrpcm.play(PSTR("irheat/audios/system/on.wav"));
            //OFF
        }else if(code==IR_CODE_OFF){
            digitalWrite(PIN_RELAY,LOW);
            duration=0;
            tmrpcm.play(PSTR("irheat/audios/system/off.wav"));
        }else if(code==IR_CODE_HEAT_10_MIN){
            //加热指定时长
            //记录开始时间
            startTime=millis();
            //记录加热时长
            duration=HEAT_TIME_10_MIN;
            //接通继电器，开始加热
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play(PSTR("irheat/audios/heat/heat10.wav"));
        }else if(code==IR_CODE_HEAT_20_MIN){
            startTime=millis();
            duration=HEAT_TIME_20_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play(PSTR("irheat/audios/heat/heat20.wav"));
        }else if(code==IR_CODE_HEAT_30_MIN){
            startTime=millis();
            duration=HEAT_TIME_30_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play(PSTR("irheat/audios/heat/heat30.wav"));
        }else if(code==IR_CODE_HEAT_40_MIN){
            startTime=millis();
            duration=HEAT_TIME_40_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play(PSTR("irheat/audios/heat/heat40.wav"));
        }else if(code==IR_CODE_HEAT_50_MIN){
            startTime=millis();
            duration=HEAT_TIME_50_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play(PSTR("irheat/audios/heat/heat50.wav"));
        }else if(code==IR_CODE_HEAT_60_MIN){
            startTime=millis();
            duration=HEAT_TIME_60_MIN;
            digitalWrite(PIN_RELAY,HIGH);
            tmrpcm.play(PSTR("irheat/audios/heat/heat60.wav"));
        }
        irrecv.resume();
    }
    delay(100);
}
