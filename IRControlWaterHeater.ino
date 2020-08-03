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
//接线定义
#define PIN_IR 3        //红外线
#define PIN_RELAY 2     //继电器
#define PIN_SD_CARD 4   //SD卡
#define PIN_LED_GREEN 5 //绿色指示灯
#define PIN_LED_RED 6   //红色指示灯
#define PIN_SPEAKER 10  //音响

//Android红外信号
#define IR_CODE_ANDROID_ON 1088254603
#define IR_CODE_ANDROID_OFF 3030512581
#define IR_CODE_ANDROID_HEAT_10_MIN 2998241093
#define IR_CODE_ANDROID_HEAT_20_MIN 3719633707
#define IR_CODE_ANDROID_HEAT_30_MIN 3932801309
#define IR_CODE_ANDROID_HEAT_40_MIN 2338481351
#define IR_CODE_ANDROID_HEAT_50_MIN 222279425
#define IR_CODE_ANDROID_HEAT_60_MIN 2347079943

//MP3遥控器红外信号
#define IR_CODE_REMOTE_CONTROL_ON 1
#define IR_CODE_REMOTE_CONTROL_OFF 1
#define IR_CODE_REMOTE_CONTROL_HEAT_10_MIN 1
#define IR_CODE_REMOTE_CONTROL_HEAT_20_MIN 1
#define IR_CODE_REMOTE_CONTROL_HEAT_30_MIN 1
#define IR_CODE_REMOTE_CONTROL_HEAT_40_MIN 1
#define IR_CODE_REMOTE_CONTROL_HEAT_50_MIN 1
#define IR_CODE_REMOTE_CONTROL_HEAT_60_MIN 1
#define IR_CODE_REMOTE_CONTROL_VOLUME_UP 1
#define IR_CODE_REMOTE_CONTROL_VOLUME_DOWN 1

//加热时长，单位毫秒
#define HEAT_TIME_10_MIN 6000
#define HEAT_TIME_20_MIN 1200000
#define HEAT_TIME_30_MIN 1800000
#define HEAT_TIME_40_MIN 2400000
#define HEAT_TIME_50_MIN 3000000
#define HEAT_TIME_60_MIN 3600000

//音频
TMRpcm tmrpcm;
//红外线
IRrecv irrecv(PIN_IR);
//红外线结果
decode_results irResults;

//音量
int audioVolume=4;
//加热开始时间
unsigned long startTime=0;
//加热时长
unsigned long duration=0;

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
    tmrpcm.setVolume(audioVolume);
    tmrpcm.play("h/a/startup.wav");
    
    //所有灯，一起亮一秒
    digitalWrite(PIN_LED_GREEN,HIGH);
    digitalWrite(PIN_LED_RED,HIGH);
    delay(1200);
    digitalWrite(PIN_LED_GREEN,LOW);
    digitalWrite(PIN_LED_RED,LOW);

}

void loop() {
    //如果有烧水任务
    if(duration>0){
        //已加热时长
        unsigned long heatedTime=calculateHeatedTime();
        //如果到了指定时间
        if(heatedTime>=duration){
            //就关掉继电器
            digitalWrite(PIN_RELAY,LOW);
            //恢复duration
            duration=0;
            //播放语音：自动关闭
            tmrpcm.play("h/a/autooff.wav");
            waitForAudioPlayFinish();
            onStopHeat(heatedTime);
        }
    }
    //接收红外信号
    if (irrecv.decode(&irResults)) {
        long long code=irResults.value;
        Serial.println(irResults.value);
        //处理红外信号
        handleIRSingnal(code);
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
 * 等待音频播放完毕
 */
void waitForAudioPlayFinish(){
    while(tmrpcm.isPlaying()){
        ;
    }
}

/**
 * 开始加热的时候
 */
void onStartHeat(){
    //接通继电器，开始加热
    digitalWrite(PIN_RELAY,HIGH);
    //记录开始时间
    startTime=millis();
}

/**
 * 计算已加热时长
 */
unsigned long calculateHeatedTime(){
    unsigned long heatedTime=0;
    //获取现在时间
    unsigned long currentTime=millis();
    //计算已加热时长
    if(currentTime>=startTime){
        heatedTime=currentTime-startTime;
    }else{
        //解决溢出问题
        heatedTime=4294967295-startTime;
        heatedTime=heatedTime+currentTime;
    }
    return heatedTime;
}

/**
 * 停止加热的时候
 */
void onStopHeat(unsigned long heatedTime){
    //播放语音：烧了多长时间
    tmrpcm.play("h/a/heated.wav");
    waitForAudioPlayFinish();
    speakTime(heatedTime);
}

/**
 * 处理接收到的红外线信号
 */
void handleIRSingnal(long long code){
    if(code==IR_CODE_ANDROID_ON||code==IR_CODE_REMOTE_CONTROL_ON){
        //如果继电器还没接通，就正常开烧
        if(digitalRead(PIN_RELAY)==LOW){
            onStartHeat();
            tmrpcm.play("h/a/on.wav");
        }else{
            //如果已经开烧了
            tmrpcm.play("h/a/reopen.wav");
        }
        //指示灯
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_ANDROID_OFF||code==IR_CODE_REMOTE_CONTROL_OFF){
        //如果正在烧，正常停止
        if(digitalRead(PIN_RELAY)==HIGH){
            digitalWrite(PIN_RELAY,LOW);
            duration=0;
            unsigned long heatedTime=calculateHeatedTime();
            //播放语音：手动停止
            tmrpcm.play("h/a/manoff.wav");
            //指示灯
            blinkForTime(PIN_LED_GREEN,2);
            waitForAudioPlayFinish();
            onStopHeat(heatedTime);
        }else{
            //如果已经停了
            tmrpcm.play("h/a/reclose.wav");
            //指示灯
            blinkForTime(PIN_LED_GREEN,2);
        }
    }else if(code==IR_CODE_ANDROID_HEAT_10_MIN||code==IR_CODE_REMOTE_CONTROL_HEAT_10_MIN){
        onStartHeat();
        duration=HEAT_TIME_10_MIN;
        tmrpcm.play("h/a/heat10.wav");
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_ANDROID_HEAT_20_MIN||code==IR_CODE_REMOTE_CONTROL_HEAT_30_MIN){
        onStartHeat();
        duration=HEAT_TIME_20_MIN;
        tmrpcm.play("h/a/heat20.wav");
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_ANDROID_HEAT_30_MIN||code==IR_CODE_REMOTE_CONTROL_HEAT_30_MIN){
        onStartHeat();
        duration=HEAT_TIME_30_MIN;
        tmrpcm.play("h/a/heat30.wav");
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_ANDROID_HEAT_40_MIN||code==IR_CODE_REMOTE_CONTROL_HEAT_40_MIN){
        onStartHeat();
        duration=HEAT_TIME_40_MIN;
        tmrpcm.play("h/a/heat40.wav");
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_ANDROID_HEAT_50_MIN||code==IR_CODE_REMOTE_CONTROL_HEAT_50_MIN){
        onStartHeat();
        duration=HEAT_TIME_50_MIN;
        tmrpcm.play("h/a/heat50.wav");
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_ANDROID_HEAT_60_MIN||code==IR_CODE_REMOTE_CONTROL_HEAT_60_MIN){
        onStartHeat();
        duration=HEAT_TIME_60_MIN;
        tmrpcm.play("h/a/heat60.wav");
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_REMOTE_CONTROL_VOLUME_UP){
        audioVolume++;
        tmrpcm.setVolume(audioVolume);
        tmrpcm.play("h/a/vol+.wav");
        blinkForTime(PIN_LED_GREEN,2);
        waitForAudioPlayFinish();
        tmrpcm.play("h/a/curvol.wav");
        waitForAudioPlayFinish();
        speakNumber(audioVolume);
    }else if(code==IR_CODE_REMOTE_CONTROL_VOLUME_DOWN){
        audioVolume--;
        tmrpcm.setVolume(audioVolume);
        tmrpcm.play("h/a/vol-.wav");
        blinkForTime(PIN_LED_GREEN,2);
        waitForAudioPlayFinish();
        tmrpcm.play("h/a/curvol.wav");
        waitForAudioPlayFinish();
        speakNumber(audioVolume);
    }else{
        //收到未知红外线指令
        tmrpcm.play("h/a/retry.wav");
        for(int i=0;i<3;i++){
            digitalWrite(PIN_LED_RED,HIGH);
            delay(170);
            digitalWrite(PIN_LED_RED,LOW);
            delay(70);
        }
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
        waitForAudioPlayFinish();
        speakOneBitNumber(number%10);        
    }else if(number>=20&&number<=99){
        speakOneBitNumber(number/10);
        waitForAudioPlayFinish();
        speakOneBitNumber(10);
        if(gewei!=0){
            waitForAudioPlayFinish();
            speakOneBitNumber(gewei);
        }
    }
}

/**
 * 说秒
 */
void speakSecond(){
    tmrpcm.play("h/a/second.wav");
}

/**
 * 说分钟
 */
void speakMintue(){
    tmrpcm.play("h/a/minute.wav");
}

/**
 * 说小时
 */
void speakHour(){
    tmrpcm.play("h/a/hour.wav");
}

/**
 * 语音播报时长
 */
void speakTime(unsigned long timeInMillis){
    unsigned long totalSeconds=timeInMillis/1000;
    unsigned long totalMinutes=totalSeconds/60;
    unsigned long totalHours=totalMinutes/60;
    //少于1秒
    if(timeInMillis<1000){
        speakNumber(1);
        waitForAudioPlayFinish();
        speakSecond();
    }else if(totalSeconds<=59){
        //少于1分钟
        speakNumber(timeInMillis/1000);
        waitForAudioPlayFinish();
        speakSecond();
    }else if(totalMinutes<=59){
        //少于1小时
        speakNumber(totalMinutes);
        waitForAudioPlayFinish();
        speakMintue();
        waitForAudioPlayFinish();
        if(totalSeconds%60!=0){
            speakNumber(totalSeconds%60);
            waitForAudioPlayFinish();
            speakSecond();
        }
    }else{
        //大于1小时
        speakNumber(totalHours);
        waitForAudioPlayFinish();
        speakHour();
        if(totalMinutes%60!=0){
            waitForAudioPlayFinish();
            speakNumber(totalMinutes%60);
            waitForAudioPlayFinish();
            speakMintue();
        }
        if(totalSeconds%60!=0){
            waitForAudioPlayFinish();
            speakNumber(totalSeconds%60);
            waitForAudioPlayFinish();
            speakSecond();
        }
    }
}
