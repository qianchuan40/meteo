#include <Arduino.h>     //Arduino库
#include <Wire.h>        //IIC库
#include <BH1750.h>      //光照传感器
#include <Ticker.h>      //定时器库
#include <U8g2lib.h>     //oled屏幕库
#include <WiFi.h>        //wifi
#include <ArduinoJson.h> //json数据发送
#include <String.h>      //字符串
#include <HTTPClient.h>  //连接服务器获取实时时间
#include <NTPClient.h>   //NTP时间
#include <ESP32Time.h>   //esp32时间库

// 创建Ticker定时器对象
Ticker ticker;
// SHT30传感器参数设置
#define SHT30_ADDR 0x44                         // 传感器I2C地址
const uint8_t MEASURE_COMMAND[] = {0x2C, 0x06}; // 高重复性测量命令
// 初始化BH1750传感器，使用默认I2C地址0x23
BH1750 lightSensor(0x23);
// 构造屏幕对象
U8G2_SH1106_128X64_NONAME_F_HW_I2C OLED(U8G2_R0);

// 定义超声波模块引脚
#define TRIG 13
#define ECHO 12
// 定义LED引脚
#define LED_BLUE 27
#define LED_RED 26
#define LED_WHITE1 25
#define LED_WHITE2 33
#define LED_WHITE3 14
// 定义蜂鸣器引脚
#define BUZZER_PIN 32
// 按钮引脚
#define button_pin 15

// 按键控制参数设置
// 定义 LED 逻辑值
int led_logic = 0;
// 判断 LED 的状态是否改变过
bool status = false;

// 存储红蓝LED灯状态（初始状态：LED1亮，LED2灭）
bool LED_BLUEState = HIGH;
bool LED_REDState = LOW;

// 报警功能开关标志位
bool isAlarmEnabled = true; // 默认开启自动报警

// 非阻塞延迟控制变量
unsigned long previousMillis = 0;
const long interval = 1000; // 传感器读取和打印数据的间隔时间（1000毫秒 = 1秒）

// 全局变量区：定义各种量
int scroll_x = 128;         // 用于控制滚动文字的起始X坐标
const int SCROLL_SPEED = 2; // 每次循环移动的像素点
float distance = 0;         // 距离
float temperature = 0.0;    // 温度值
float humidity = 0.0;       // 湿度值
bool dataValid = false;    // 数据有效性标志
float THI = 0.0;           // 温湿指数
float illumination = 0.00; // 光照强度值

// 定义各种阈值
float maxTemp = 30.00;          // 高温度阈值
float minTemp = 10.00;          // 低温度阈值
float maxHumi = 90.00;          // 高湿度阈值
float minHumi = 30.00;          // 低湿度阈值
float maxIllumination = 900.00; // 高光照强度阈值
float minIllumination = 10.00;  // 低光照强度阈值
float levelDistance = 30;       // 距离判断阈值

// 定义状态量
String LED_WHITE_state = "";
String LED_FLICKER_state = "";
String BUZZER_state = "";
// 定义显示屏显示的时间字符串
String dateTime = "";

// 定义标志量
int flag = 0;

WiFiClient client;

JsonDocument doc;      // 发送数据json
JsonDocument response; // 接收数据json

// 声明WiFi名称与密码
const char *ssid = "wifi_jun";      // wifi名称
const char *password = "123456987"; // wifi密码

const char *host = "192.168.0.104"; // 服务器地址
const int port = 9050;              // 服务器端口

// 创建NTP和HTTP客户端对象
WiFiUDP ntpUDP;                                              // NTPClient库需要
NTPClient timeClient(ntpUDP, "192.168.0.104");               // NTP服务器地址
ESP32Time rtc(8 * 3600);                                     // 时区设置为东八区
HTTPClient http;                                             // HTTPClient库需要
const char *serverUrl = "http://192.168.0.104:8020/olddata"; // 不加http://连不上

// 时间解析函数，直接用时间戳解析24小时制时间
void time()
{
  // 获取UTC时间戳
  time_t epochTime = rtc.getEpoch();
  struct tm *timeInfo = localtime(&epochTime); // 解析为本地时间并做时区偏移，转换成为东八区的时间

  // 直接提取24小时制的时间分量
  int year = timeInfo->tm_year + 1900;
  int month = timeInfo->tm_mon + 1;
  int day = timeInfo->tm_mday;
  int hour = timeInfo->tm_hour;
  int minute = timeInfo->tm_min;
  int second = timeInfo->tm_sec;

  // 格式化北京时间字符串
  dateTime = String(year) + "-" + (month < 10 ? "0" : "") 
  + String(month) + "-" + (day < 10 ? "0" : "") + String(day) + " " 
  + (hour < 10 ? "0" : "") + String(hour) + ":" + (minute < 10 ? "0" : "") 
  + String(minute) + ":" + (second < 10 ? "0" : "") + String(second);
}

// 定时器回调函数：切换两个LED的状态（确保状态相反）
void toggleLeds()
{
  // 翻转第一个LED的状态
  LED_BLUEState = !LED_BLUEState;
  // 第二个LED的状态与第一个相反（直接取反）
  LED_REDState = !LED_REDState;

  // 更新LED的实际状态
  digitalWrite(LED_BLUE, LED_BLUEState);
  digitalWrite(LED_RED, LED_REDState);
}

// CRC8校验函数（SHT30专用）
uint8_t crc8(const uint8_t *data, uint8_t len)
{
  uint8_t crc = 0xFF;
  for (uint8_t i = 0; i < len; i++)
  {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++)
    {
      crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
  }
  return crc;
}

void scroll_text_logic()
{
  // 屏幕宽度是 128 像素。U8g2 字体 u8g2_font_wqy12_t_gb2312 的汉字宽度大约是 12 像素。
  // 完整文本宽度 (W) = n 个字 * 12 像素/字
  const int TEXT_WIDTH = 500;
  const int SCREEN_WIDTH = 128;

  // 每次向左移动 SCROLL_SPEED 像素
  scroll_x -= SCROLL_SPEED;

  // 检查是否已经完全滚出屏幕左侧
  // 当起始 X 坐标 小于 (负的文本宽度) 时，就认为已经完全滚出
  if (scroll_x < -TEXT_WIDTH)
  {
    // 将起始 X 坐标重置到屏幕右侧外，等待下次滚动进入
    scroll_x = SCREEN_WIDTH;
  }
}

void temperature_humidity()
{
  uint8_t data[6]; // 存储传感器返回的数据

  // 发送测量命令
  Wire.beginTransmission(SHT30_ADDR);
  Wire.write(MEASURE_COMMAND, sizeof(MEASURE_COMMAND));
  if (Wire.endTransmission() != 0)
  {
    Serial.println("错误:无法与SHT30通信");
    delay(1000);
    return;
  }

  // 等待测量完成
  delay(150);

  // 读取6字节数据（温度2字节+温度CRC1字节+湿度2字节+湿度CRC1字节）
  if (Wire.requestFrom(SHT30_ADDR, 6) == 6)
  {
    for (int i = 0; i < 6; i++)
    {
      data[i] = Wire.read();
    }

    // 校验温度数据
    uint8_t tempCrc = crc8(data, 2);
    // 校验湿度数据
    uint8_t humiCrc = crc8(data + 3, 2);

    // 检查CRC是否匹配
    if (tempCrc == data[2] && humiCrc == data[5])
    {
      // 计算温度值
      uint16_t rawTemp = (data[0] << 8) | data[1];
      temperature = (rawTemp * 175.0) / 65535.0 - 45.0;

      // 计算湿度值
      uint16_t rawHumi = (data[3] << 8) | data[4];
      humidity = (rawHumi * 100.0) / 65535.0;

      // 计算温湿指数
      THI = (1.8 * temperature + 32.0) - 0.55 * (1 - 0.01 * humidity) * (1.8 * temperature - 26);

      dataValid = true;
    }
    else
    {
      Serial.println("错误:数据CRC校验失败");
    }
  }
  else
  {
    Serial.println("错误:读取数据失败");
  }
}

void light()
{
  // 读取光照强度（单位：勒克斯lx）
  illumination = lightSensor.readLightLevel();
}

void HC_SR04()
{
  // 清除trigPin
  digitalWrite(TRIG, LOW);
  delay(5);
  // 将trigPin设置为HIGH状态10微秒
  digitalWrite(TRIG, HIGH);
  delay(10);
  digitalWrite(TRIG, LOW);
  // 读取echoPin，返回声波传播时间(微秒)
  float time = pulseIn(ECHO, HIGH);
  // 计算距离
  distance = time * 0.03432 / 2;
}

void LED()
{
  if (temperature >= maxTemp || temperature <= minTemp || humidity >= maxHumi || humidity <= minHumi || illumination >= maxIllumination)
  {
    // 配置定时器：每500毫秒（0.5秒）执行一次toggleLeds()函数
    // 第二个参数为间隔时间（秒），这里0.5秒即500ms
    if (!ticker.active())
    {
      ticker.attach(0.5, toggleLeds);
    }
    flag = 1;
  }
  else
  {
    // 关闭定时器，熄灭LED
    if (ticker.active())
    {
      ticker.detach(); // 停止定时器
    }
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_RED, LOW);
    flag = 0;
  }

  if (illumination < minIllumination)
  {
    digitalWrite(LED_WHITE1, HIGH);
  }
  if (illumination >= minIllumination)
  {
    digitalWrite(LED_WHITE1, LOW);
  }
}

void BUZZER()
{
  if (isAlarmEnabled) // 只有当报警功能开启时才执行报警逻辑
  {
    if (distance <= levelDistance)
    {
      digitalWrite(BUZZER_PIN, HIGH);
    }
    if (distance > levelDistance)
    {
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
  else
  {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void SPT()
{
  Serial.print("温度: ");
  Serial.print(temperature);
  Serial.println("°C");

  Serial.print("湿度: ");
  Serial.print(humidity);
  Serial.println("%");

  Serial.print("光照强度: ");
  Serial.print(illumination);
  Serial.println("lx");

  if (distance <= levelDistance)
  {
    Serial.print("距离: ");
    Serial.print(distance);
    Serial.println("cm");
  }
  if (distance > levelDistance)
  {
    Serial.println("暂无物体");
  }
}

// 辅助函数：将浮点型的 THI 值转换为对应的整数赋（档）值 (1-9)
int BREAKDOWN(float THI)
{
  if (THI < 40.0)
    return 1;
  if (THI >= 40.0 && THI < 45.0)
    return 2;
  if (THI >= 45.0 && THI < 55.0)
    return 3;
  if (THI >= 55.0 && THI < 60.0)
    return 4;
  if (THI >= 60.0 && THI < 65.0)
    return 5;
  if (THI >= 65.0 && THI < 70.0)
    return 6;
  if (THI >= 70.0 && THI < 75.0)
    return 7;
  if (THI >= 75.0 && THI < 80.0)
    return 8;
  if (THI >= 80.0)
    return 9;

  // 否则错误返回0
  return 0;
}

void OLED_PT()
{
  // 清除缓存区内容
  OLED.clearBuffer();
  // 绘制内容
  OLED.setCursor(0, 10); // 位置
  OLED.print("温度:");   // 内容
  OLED.print(temperature);
  OLED.print("°C");

  OLED.setCursor(69, 10); // 位置
  OLED.print("THI:");
  OLED.println(THI);

  OLED.setCursor(0, 22); // 位置
  OLED.print("湿度:");   // 内容
  OLED.print(humidity);
  OLED.print("%");

  if (LED_WHITE_state == "kai")
  {
    OLED.setCursor(69, 22);   // 位置
    OLED.print("照明灯开启"); // 内容
  }
  if (LED_WHITE_state == "guang")
  {
    OLED.setCursor(69, 22);   // 位置
    OLED.print("照明灯关闭"); // 内容
  }

  OLED.setCursor(0, 34); // 位置
  OLED.print("光强:");   // 内容
  OLED.print(illumination);
  OLED.print("lx");

  if (LED_FLICKER_state == "kai")
  {
    OLED.setCursor(69, 34);   // 位置
    OLED.print("提示灯闪烁"); // 内容
  }
  if (LED_FLICKER_state == "guang")
  {
    OLED.setCursor(69, 34);   // 位置
    OLED.print("提示灯关闭"); // 内容
  }

  if (distance <= levelDistance)
  {
    OLED.setCursor(0, 46); // 位置
    OLED.print("距离:");   // 内容
    OLED.print(distance);
    OLED.print("cm");
  }
  if (distance > levelDistance)
  {
    OLED.setCursor(0, 46); // 位置
    OLED.print("暂无物体");
  }

  if (BUZZER_state == "kai")
  {
    OLED.setCursor(69, 46);   // 位置
    OLED.print("蜂鸣器报警"); // 内容
  }
  if (BUZZER_state == "guang")
  {
    OLED.setCursor(69, 46);   // 位置
    OLED.print("蜂鸣器关闭"); // 内容
  }

  OLED.setCursor(scroll_x, 58); // 位置
  OLED.print("当前时间:");       // 内容
  OLED.print(dateTime);         // 先打印时间
  OLED.print(" ");              // 留个空格隔开
  // 打印 THI 数值
  OLED.print("THI温湿指数:");
  OLED.print(THI);
  OLED.print(" ");
  int level = BREAKDOWN(THI);
  if (level == 1)
  {
    OLED.print("人体感觉:极冷 舒适度:极不舒适");
  }
  else if (level == 2)
  {
    OLED.print("人体感觉:寒冷 舒适度:不舒适");
  }
  else if (level == 3)
  {
    OLED.print("人体感觉:偏冷 舒适度:较不舒适");
  }
  else if (level == 4)
  {
    OLED.print("人体感觉:清凉 舒适度:舒适");
  }
  else if (level == 5)
  {
    OLED.print("人体感觉:凉 舒适度:非常舒适");
  }
  else if (level == 6)
  {
    OLED.print("人体感觉:暖 舒适度:舒适");
  }
  else if (level == 7)
  {
    OLED.print("人体感觉:偏热 舒适度:较舒适");
  }
  else if (level == 8)
  {
    OLED.print("人体感觉:闷热 舒适度:不舒适");
  }
  else if (level == 9)
  {
    OLED.print("人体感觉:极闷热 舒适度:极不舒适");
  }
  else
  {
    OLED.print("计算错误");
  }

  // 发送缓存区内容到OLED
  OLED.sendBuffer();
}

void send()
{
  // 发送
  if (client.connected())
  {
    // 实时发送数据
    doc.clear();
    doc["name"] = "001";
    doc["temperature"] = String(temperature) + "℃";
    doc["humidity"] = String(humidity) + "%";
    doc["distance"] = String(distance) + "cm";
    doc["illumination"] = String(illumination) + "lx";
    String json;
    serializeJson(doc, json);
    client.print(json); // 发送数据
    doc.clear();
    // 打印发送到服务器的数据，方便调试
    Serial.print("发送实时数据成功：");
    Serial.println(json);
  }
}

// JSON指令接收函数
void accept()
{
  if (client.available())
  {
    // 读取指令串
    String receiveData = client.readStringUntil('\n');
    receiveData.trim();
    if (receiveData.length() == 0)
      return;

    // 解析 JSON
    response.clear(); // 先清空，防止数据残留
    DeserializationError error = deserializeJson(response, receiveData);
    if (error)
    {
      Serial.print(">>> [JSON错误] 解析失败: ");
      Serial.println(error.f_str());
      return;
    }

    // 根据指令类型cmd分发逻辑
    const char *cmd = response["cmd"];

    // 阈值设置指令
    if (strcmp(cmd, "set_limit") == 0)
    {
      String type = response["type"];
      float maxVal = response["max"];
      float minVal = response["min"];

      // 更新逻辑数值
      if (type == "temp")
      {
        maxTemp = maxVal;
        minTemp = minVal;
      }
      else if (type == "humi")
      {
        maxHumi = maxVal;
        minHumi = minVal;
      }
      else if (type == "illumination")
      {
        maxIllumination = maxVal;
        minIllumination = minVal;
      }

      // 串口同步显示
      Serial.printf(">>> [配置] %s 阈值已更新: %.2f ~ %.2f\n", type.c_str(), minVal, maxVal);
    }

    // 远程授权指令
    else if (strcmp(cmd, "control") == 0)
    {
      // 建议服务器发送格式：{"cmd":"control", "target":"led_on"}
      String target = response["target"];

      if (target == "led_on")
      {
        digitalWrite(LED_WHITE3, HIGH);
        Serial.println(">>> 远程开灯");
      }
      else if (target == "led_off")
      {
        digitalWrite(LED_WHITE3, LOW);
        Serial.println(">>> 远程关灯");
      }
      else if (target == "buzzer_on")
      {
        isAlarmEnabled = true; // 开启报警功能
        Serial.println(">>> 远程开启报警功能");
      }
      else if (target == "buzzer_off")
      {
        isAlarmEnabled = false; // 禁用报警逻辑
        digitalWrite(BUZZER_PIN, LOW);
        Serial.println(">>> 远程关闭报警功能");
      }
    }

    // 给服务器反馈
    client.print("{\"status\":\"success\",\"msg\":\"cmd_executed\"}\n");
  }
}

void STATE()
{
  if (digitalRead(BUZZER_PIN) == HIGH)
    BUZZER_state = "kai";
  if (digitalRead(BUZZER_PIN) == LOW)
    BUZZER_state = "guang";

  if ((digitalRead(LED_WHITE1) == HIGH) || (digitalRead(LED_WHITE2) == HIGH) || (digitalRead(LED_WHITE3) == HIGH))
    LED_WHITE_state = "kai";
  else
    LED_WHITE_state = "guang";

  if (flag == 1)
    LED_FLICKER_state = "kai";
  if (flag == 0)
    LED_FLICKER_state = "guang";
}

// 传感器任务
void vTaskSensor1(void *pvParameters)
{
  while (1)
  {
    HC_SR04();
    temperature_humidity();
    light();
    LED();
    BUZZER();
    STATE();
    OLED_PT();
    scroll_text_logic();
  }
}

// 按钮任务
void vTaskBUTTON(void *pvParameters)
{
  // 定义一个局部变量来存储上一次按钮读取状态，用于边沿检测
  int lastButtonState = LOW;

  while (1)
  {
    int currentButtonState = digitalRead(button_pin);

    // 1. 检测按键的“上升沿” (从 LOW 变为 HIGH)
    // 并且状态与上次不同，这样可以避免在长按期间重复切换
    if (currentButtonState == HIGH && lastButtonState == LOW)
    {
      // 简单的软件消抖：等待一小段时间
      vTaskDelay(pdMS_TO_TICKS(50)); // 使用 FreeRTOS 延迟，避免阻塞

      // 1.1 再次读取，确认不是抖动
      if (digitalRead(button_pin) == HIGH)
      {
        // 2. 切换逻辑值
        led_logic = !led_logic;
        digitalWrite(LED_WHITE2, led_logic);

        // 可选：打印状态以便调试
        // Serial.printf("LED_WHITE2 状态切换为: %s\n", led_logic ? "HIGH" : "LOW");
      }
    }

    // 3. 更新上一次的状态
    lastButtonState = currentButtonState;

    // 4. 任务延迟：让出 CPU 给其他任务和系统
    // 这个延迟决定了按钮的扫描频率（例如 20ms 扫描一次）
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_WHITE1, OUTPUT);
  pinMode(LED_WHITE2, OUTPUT);
  pinMode(LED_WHITE3, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(button_pin, INPUT_PULLDOWN);

  // 设置初始状态为灭
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_WHITE1, LOW);
  digitalWrite(LED_WHITE2, LOW);
  digitalWrite(LED_WHITE3, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // 初始化I2C通信（SDA=21, SCL=22为ESP32默认I2C引脚）
  Wire.begin(21, 22);
  // 初始化BH1750传感器，使用高分辨率模式
  if (!lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
  {
    Serial.println("无法找到BH1750传感器,请检查接线！");
    while (1)
      ; // 初始化失败则停在这里
  }

  WiFi.begin(ssid, password);
  Serial.print("正在连接 Wi-Fi..");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n连接成功");
  Serial.print("IP 地址：");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.forceUpdate();
  rtc.setTime(timeClient.getEpochTime());
  http.begin(serverUrl);

  // 初始化OLED
  OLED.begin();
  // 开启中文字符集支持
  OLED.enableUTF8Print();
  // 设置字体
  OLED.setFont(u8g2_font_wqy12_t_gb2312);

  // 按钮任务（绑定到核心0，与传感器任务核心1分开，负载更均衡）
  xTaskCreatePinnedToCore(vTaskBUTTON, "BUTTON", 2048, NULL, 7, NULL, 0);
  // 创建次线程
  xTaskCreatePinnedToCore(vTaskSensor1, "Sensor1", 4096, NULL, 7, NULL, 1);
}

void loop()
{
  time();
  accept();
  // 获取当前时间
  unsigned long currentMillis = millis();
  // 检查是否到达预定的时间间隔
  if (currentMillis - previousMillis >= interval)
  {
    // 保存本次执行的时间
    previousMillis = currentMillis;
    SPT();
    send();
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wi-Fi已断开,请重启以重新连接");
    delay(1000);
  }

  if (!client.connected())
  {
    Serial.println("服务器断开了连接,正在重新连接");
    if (client.connect(host, port))
    {
      Serial.println("服务器重连成功");
    }
    delay(1000);
  }
}
