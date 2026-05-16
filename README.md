缺少一个基于node.js的server服务器

 1. weather/ — 前端客户端

  uni-app(Vue3)跨平台前端，含登录、实时监测看板、历史查询三页面。通过WebSocket接收传感器实时数据并以ECharts绘制趋势图，
  支持设备反向控制（灯光/报警开关）、阈值配置、THI舒适度评估、CSV导出。编译为H5/小程序/App。

  2. 数据库/ — 数据库初始化脚本

  MySQL建表DDL脚本，创建sensordata表（id/name/status/data(json)/recvtime/timestr），采用InnoDB+utf8mb4。是系统中硬件TCP
  数据采集→Node.js服务端→MySQL持久化→WebSocket推送→前端展示整条数据链路的存储层入口。

  3. sketch_apr2b/ — 硬件终端固件

  ESP32 Arduino固件，基于FreeRTOS双核任务架构。通过I2C采集SHT30温湿度+BH1750光照+超声波测距，OLED本地显示，WiFi
  TCP实时上报JSON数据至Node.js服务器(9050端口)，并接收服务器下发的阈值配置与远程控制指令。

  4. Api2Weather/ — 历史查询微服务

  Python Flask轻量级REST API，提供/api/sensors接口按设备名+时间范围查询MySQL中sensordata表的历史数据，支持limit分页，返
  回JSON。作为Node.js主服务的补充，专门承接前端历史数据查询请求，与主服务共享同一MySQL数据库。

  ---
  整体架构关系

  ESP32硬件(sketch_apr2b) ──TCP──→ Node.js+Express+WS服务器 ──→ MySQL(数据库/)
                                        │
                                        ├──WebSocket──→ uni-app前端(weather/)
                                        │
                                        └── Flask API(Api2Weather) ←──HTTP── uni-app前端(weather/)
