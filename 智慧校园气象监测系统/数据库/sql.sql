create database xxx;
use xxx;
CREATE TABLE `sensordata` (
  `id` int NOT NULL AUTO_INCREMENT,
  `name` varchar(50) DEFAULT NULL,
  `status` int DEFAULT NULL,
  `data` json DEFAULT NULL,
  `recvtime` bigint DEFAULT NULL,
  `timestr` varchar(50) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
select *from sensordata;



-- 1. 关闭安全模式 (允许执行不带主键条件的删除)
SET SQL_SAFE_UPDATES = 0;

-- 2. 执行你的删除操作...

-- 3. 开启安全模式 (操作完建议重新开启，保护数据安全)
SET SQL_SAFE_UPDATES = 1;

#如果你想清空表，让 `id` 重新从 1 开始计数，用这个。**它不受安全模式限制**，速度极快。

TRUNCATE TABLE sensordata;