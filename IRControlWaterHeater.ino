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
#define PIN_RELAY 2     //继电器
#define PIN_IR 3        //红外线
#define PIN_SD_CARD 4   //SD卡
#define PIN_LED_GREEN 5 //绿色指示灯
#define PIN_LED_RED 6   //红色指示灯
#define PIN_SPEAKER 10  //音响

//EEPROM变量初始address
//变量定义表：
//https://shimo.im/sheets/9Jgw3HTPVPD6kj89/MODOC
//英文名             中文名        类型                字节长度  位置区间  起始address
//versionCode       版本号        unsigned long         4       0-3       0
//startupCount      开机次数统计   unsigned long         4       4-7       4
//volume            音量          int                   2       8-9       8
//heatedTimeMillis  总共烧的时长   unsigned long long    8      10-17      10
//startCount        开始烧水次数   unsigned long         4      18-21      18
//stopCount         停止烧水次数   unsigned long         4      22-25      22

#define VERSION_CODE 1      //版本号
#define EEPROM_ADDRESS_VERSION_CODE 0
#define EEPROM_ADDRESS_STARTUP_COUNT 4
#define EEPROM_ADDRESS_VOLUME 8
#define EEPROM_ADDRESS_HEATED_TIME_MILLIS 10
#define EEPROM_START_COUNT 18
#define EEPROM_STOP_COUNT 22

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
#define IR_CODE_REMOTE_CONTROL_ON 3810010651
#define IR_CODE_REMOTE_CONTROL_OFF 5316027
#define IR_CODE_REMOTE_CONTROL_HEAT_10_MIN 2534850111
#define IR_CODE_REMOTE_CONTROL_HEAT_20_MIN 1033561079
#define IR_CODE_REMOTE_CONTROL_HEAT_30_MIN 1635910171
#define IR_CODE_REMOTE_CONTROL_HEAT_40_MIN 2351064443
#define IR_CODE_REMOTE_CONTROL_HEAT_50_MIN 1217346747
#define IR_CODE_REMOTE_CONTROL_HEAT_60_MIN 71952287
#define IR_CODE_REMOTE_CONTROL_VOLUME_UP 2747854299
#define IR_CODE_REMOTE_CONTROL_VOLUME_DOWN 4034314555

//最大最小音量
#define MIN_AUDIO_VOLUME 0
#define MAX_AUDIO_VOLUME 6

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
//继电器接通电开始时间
unsigned long heatStartTime=0;
//倒计时开始时间
unsigned long countDownStartTime=0;
//加热时长
unsigned long duration=0;

void setup() {
    Serial.begin(9600);
    initEEPROM();
    pinMode(PIN_RELAY,OUTPUT);
    pinMode(PIN_LED_GREEN,OUTPUT);
    pinMode(PIN_LED_RED,OUTPUT);

    //SD卡
    if (!SD.begin(PIN_SD_CARD)) {
        Serial.println("SD fail");
    }
    //音响
    tmrpcm.speakerPin = PIN_SPEAKER;
    //红外线
    irrecv.enableIRIn();

    //播放开机声音
    tmrpcm.setVolume(audioVolume);
    tmrpcm.play("h/startup");
    
    //所有灯，一起亮一秒
    digitalWrite(PIN_LED_GREEN,HIGH);
    digitalWrite(PIN_LED_RED,HIGH);
    delay(1200);
    digitalWrite(PIN_LED_GREEN,LOW);
    digitalWrite(PIN_LED_RED,LOW);

    //说一共开机几次了
    unsigned long startupCount=0;
    EEPROM.get(EEPROM_ADDRESS_STARTUP_COUNT,startupCount);
    waitForAudioPlayFinish();
    //播放语音：开机次数
    tmrpcm.play("h/ttstart");
    waitForAudioPlayFinish();
    //播放语音：几次
    speakNumber(startupCount);
}

void loop() {
    //如果有烧水任务
    if(duration>0){
        //已加热时长
        unsigned long countDownTimePeriod=calculateTimePeriod(countDownStartTime,millis());
        //如果到了指定时间，执行自动停止
        if(countDownTimePeriod>=duration){
            //就关掉继电器
            digitalWrite(PIN_RELAY,LOW);
            duration=0;
            //烧水时长，注意不是倒计时时长
            unsigned long heatedTimePeriod=calculateTimePeriod(heatStartTime,millis());
            //当结束烧水时
            onStopHeat(heatedTimePeriod);
            //播放语音：自动停止
            tmrpcm.play("h/autooff");
            waitForAudioPlayFinish();
            //播放语音：烧了
            tmrpcm.play("h/heated");
            waitForAudioPlayFinish();
            //播放语音：多长时间
            speakTime(heatedTimePeriod);
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
    delay(75);
}

/**
 * 开机初始化EEPROM
 */
unsigned long initEEPROM(){
    //这里默认arduino已全部清零后，再执行本程序
    //先检查版本号
    unsigned long arduinoVersionCode=0;
    EEPROM.get(EEPROM_ADDRESS_VERSION_CODE,arduinoVersionCode);
    //如果版本号为0.说明这是第一次运行本程序
    if(arduinoVersionCode==0){
        //写入版本号
        arduinoVersionCode=VERSION_CODE;
        EEPROM.put(EEPROM_ADDRESS_VERSION_CODE,arduinoVersionCode);
        //写入开机次数
        unsigned long startupCount=1;
        EEPROM.put(EEPROM_ADDRESS_STARTUP_COUNT,startupCount);
        //写入音量
        int audioVolume=4;
        EEPROM.put(EEPROM_ADDRESS_VOLUME,audioVolume);
        //写入总共烧的时长
        unsigned long long heatedTimeMillis=0;
        EEPROM.put(EEPROM_ADDRESS_HEATED_TIME_MILLIS,heatedTimeMillis);
        //写入总开始烧水次数
        unsigned long startCount=0;
        EEPROM.put(EEPROM_START_COUNT,startCount);
        //写入总停止烧水次数
        unsigned long stopCount=0;
        EEPROM.put(EEPROM_STOP_COUNT,stopCount);
        
        //看是不是我程序中最新的版本号
    }else if(arduinoVersionCode==VERSION_CODE){
        //如果是最新版本号
        //读开机次数
        unsigned long startupCount=0;
        EEPROM.get(EEPROM_ADDRESS_STARTUP_COUNT, startupCount);
        //加开机次数
        //这里有bug，我不知道怎么修复。就是烧录启动会记两次，而reset正常，只记一次
        startupCount++;
        //写开机次数
        EEPROM.put(EEPROM_ADDRESS_STARTUP_COUNT,startupCount);

        //读音量
        EEPROM.get(EEPROM_ADDRESS_VOLUME,audioVolume);

//        unsigned long heatedTimeMillis;
//        EEPROM.get(EEPROM_ADDRESS_HEATED_TIME_MILLIS,heatedTimeMillis);
//        Serial.println(heatedTimeMillis);
    }else{
        //能到这里的情况是，有版本号，但是arduino和程序最新的版本号不一致
        //说明需要升级操作
        
    }
}

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
    //当前继电器状态
    int relayState=digitalRead(PIN_RELAY);
    //处理加热开始时间
    //如果没接通
    if(relayState==LOW){
        //接通继电器，开始加热
        digitalWrite(PIN_RELAY,HIGH);
        //记录继电器接通开始时间
        heatStartTime=millis();
    }else{
        //如果继电器已经接通了
        //如果duration为零，说明是直接按on启动的
        if(duration==0){
            //记录继电器接通开始时间
            heatStartTime=millis();
        }else{
            //duration不等于零，说明是先按了on，再按定时的，则不记录开始加热时间
        }
    }
    //处理倒计时开始时间
    if(duration!=0){
        //记录倒计时开始时间
        countDownStartTime=millis();
    }
}

/**
 * 计算时间段时长
 */
unsigned long calculateTimePeriod(unsigned long startTime,unsigned long endTime){
    unsigned long timePeriod=0;
    //没有溢出，正常做减法
    if(endTime>=startTime){
        timePeriod=endTime-startTime;
    }else{
        //解决溢出问题
        timePeriod=4294967295-startTime;
        timePeriod=timePeriod+endTime;
    }
    return timePeriod;
}

/**
 * 停止加热的时候
 */
void onStopHeat(unsigned long heatedTime){
    //把加热时长保存到EEPROM
    //读之前的加热时长
    unsigned long long lastHeatedTime;
    EEPROM.get(EEPROM_ADDRESS_HEATED_TIME_MILLIS,lastHeatedTime);
    //更新
    lastHeatedTime=lastHeatedTime+heatedTime;
    //保存
    EEPROM.put(EEPROM_ADDRESS_HEATED_TIME_MILLIS,lastHeatedTime);
}

/**
 * 处理接收到的红外线信号
 */
void handleIRSingnal(long long code){
    if(code==IR_CODE_ANDROID_ON||code==IR_CODE_REMOTE_CONTROL_ON){
        //如果继电器还没接通，就正常开烧
        if(digitalRead(PIN_RELAY)==LOW){
            onStartHeat();
            tmrpcm.play("h/on");
        }else{
            //如果已经开烧了
            tmrpcm.play("h/reopen");
        }
        //指示灯
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_ANDROID_OFF||code==IR_CODE_REMOTE_CONTROL_OFF){
        //按键手动停止
        //如果正在烧，就停止
        if(digitalRead(PIN_RELAY)==HIGH){
            digitalWrite(PIN_RELAY,LOW);
            duration=0;
            //烧水时长，注意不是倒计时时长
            unsigned long heatedTimePeriod=calculateTimePeriod(heatStartTime,millis());
            //当结束烧水时
            onStopHeat(heatedTimePeriod);
            //播放语音：手动停止
            tmrpcm.play("h/manoff");
            //指示灯
            blinkForTime(PIN_LED_GREEN,2);
            waitForAudioPlayFinish();
            //播放语音：烧了多长时间
            tmrpcm.play("h/heated");
            waitForAudioPlayFinish();
            speakTime(heatedTimePeriod);
        }else{
            //如果已经停了
            //播放语音：已经停了
            tmrpcm.play("h/reclose");
            //指示灯
            blinkForTime(PIN_LED_GREEN,2);
        }
    }else if(code==IR_CODE_ANDROID_HEAT_10_MIN||code==IR_CODE_REMOTE_CONTROL_HEAT_10_MIN){
        duration=HEAT_TIME_10_MIN;
        onStartHeat();
        tmrpcm.play("h/h10");
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_ANDROID_HEAT_20_MIN||code==IR_CODE_REMOTE_CONTROL_HEAT_20_MIN){
        duration=HEAT_TIME_20_MIN;
        onStartHeat();
        tmrpcm.play("h/h20");
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_ANDROID_HEAT_30_MIN||code==IR_CODE_REMOTE_CONTROL_HEAT_30_MIN){
        duration=HEAT_TIME_30_MIN;
        onStartHeat();
        tmrpcm.play("h/h30");
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_ANDROID_HEAT_40_MIN||code==IR_CODE_REMOTE_CONTROL_HEAT_40_MIN){
        duration=HEAT_TIME_40_MIN;
        onStartHeat();
        tmrpcm.play("h/h40");
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_ANDROID_HEAT_50_MIN||code==IR_CODE_REMOTE_CONTROL_HEAT_50_MIN){
        duration=HEAT_TIME_50_MIN;
        onStartHeat();
        tmrpcm.play("h/h50");
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_ANDROID_HEAT_60_MIN||code==IR_CODE_REMOTE_CONTROL_HEAT_60_MIN){
        onStartHeat();
        duration=HEAT_TIME_60_MIN;
        tmrpcm.play("h/h60");
        blinkForTime(PIN_LED_GREEN,2);
    }else if(code==IR_CODE_REMOTE_CONTROL_VOLUME_UP){
        //如果当前音量还没有达到最大音量，还有加音空间
        if(audioVolume<MAX_AUDIO_VOLUME){
            audioVolume++;
            tmrpcm.setVolume(audioVolume);
            tmrpcm.play("h/volup");
            blinkForTime(PIN_LED_GREEN,2);
            //保存音量设置
            EEPROM.put(EEPROM_ADDRESS_VOLUME,audioVolume);
            waitForAudioPlayFinish();
            tmrpcm.play("h/curvol");
            waitForAudioPlayFinish();
            speakNumber(audioVolume);
        }else{
            //如果已经到了最大音量
            tmrpcm.play("h/maxvol");
        }
    }else if(code==IR_CODE_REMOTE_CONTROL_VOLUME_DOWN){
        //如果还没达到最小音量，还有减音空间
        if(audioVolume>MIN_AUDIO_VOLUME){
            audioVolume--;
            tmrpcm.setVolume(audioVolume);
            tmrpcm.play("h/voldown");
            blinkForTime(PIN_LED_GREEN,2);
            //保存音量设置
            EEPROM.put(EEPROM_ADDRESS_VOLUME,audioVolume);
            waitForAudioPlayFinish();
            tmrpcm.play("h/curvol");
            waitForAudioPlayFinish();
            speakNumber(audioVolume);
        }else{
            //已经达到最小音量，不能再减少了
            tmrpcm.play("h/minvol");
        }
    }else{
        //收到未知红外线指令
        tmrpcm.play("h/retry");
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
    if(number==0){
        tmrpcm.play("h/0");
    }else if(number==1){
        tmrpcm.play("h/1");
    }else if(number==2){
        tmrpcm.play("h/2");
    }else if(number==3){
        tmrpcm.play("h/3");
    }else if(number==4){
        tmrpcm.play("h/4");
    }else if(number==5){
        tmrpcm.play("h/5");
    }else if(number==6){
        tmrpcm.play("h/6");
    }else if(number==7){
        tmrpcm.play("h/7");
    }else if(number==8){
        tmrpcm.play("h/8");
    }else if(number==9){
        tmrpcm.play("h/9");
    }else if(number==10){
        tmrpcm.play("h/10");
    }
}

/**
 * 播放语音数字
 */
void speakNumber(unsigned long number){
    unsigned long gewei=number%10;
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
    tmrpcm.play("h/second");
}

/**
 * 说分钟
 */
void speakMintue(){
    tmrpcm.play("h/minute");
}

/**
 * 说小时
 */
void speakHour(){
    tmrpcm.play("h/hour");
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
