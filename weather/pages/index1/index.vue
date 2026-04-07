<template>
	<view class="container">
		<view class="navbar">
			<view class="nav-left">
				<text class="navbar-title">智慧校园气象监测信息系统</text>
				<text class="status-tag" :class="{online: isConnected}">{{ isConnected ? '● 已连接' : '○ 断开连接' }}</text>
			</view>

			<!-- 右侧新增用户信息和退出 -->
			<view class="nav-right-user">
				<text class="current-time">{{ currentTime }}</text>
				<text class="role-text">{{ userRole === 'admin' ? '管理员' : '普通用户' }}</text>
				<button class="logout-btn" @click="handleLogout">退出</button>
			</view>
		</view>

		<scroll-view scroll-y="true" class="main-content">
			<!-- 统一的网格容器 -->
			<view class="data-grid">
				<!-- 第一排：3个实时传感器卡片 -->
				<view v-for="(item, key) in sensorConfig" :key="key" class="sensor-card"
					:class="{ 'alarm-active': isAlarming(key) }">
					<view class="card-header">
						<text class="card-icon">{{ item.icon }}</text>
						<text class="card-label">{{ item.label }}</text>
					</view>
					<view class="card-body">
						<text class="card-value">{{ getRealValue(key) }}</text>
						<text class="card-unit">{{ item.unit }}</text>
					</view>
					<view class="card-footer">
						<text>阈值范围: {{ activeThresholds[key].min }} ~ {{ activeThresholds[key].max }}
							{{ item.unit }}</text>
					</view>
				</view>

				<!-- 第二排：左侧 日出日落卡片 -->
				<view class="sensor-card extra-card sun-card">
					<view class="card-header">
						<text class="card-icon">☀️</text>
						<text class="card-label">昼夜天文</text>
					</view>
					<view class="card-body sun-display">
						<view class="sun-item">
							<text class="sun-time">{{ sun.sunrise || '--:--' }}</text>
							<text class="sun-sub">日出</text>
						</view>
						<!-- 中间装饰线条 -->
						<view class="sun-progress">
							<view class="sun-dot" :style="{left: '40%'}"></view>
						</view>
						<view class="sun-item">
							<text class="sun-time">{{ sun.sunset || '--:--' }}</text>
							<text class="sun-sub">日落</text>
						</view>
					</view>
					<view class="card-footer">
						<text>今日日照总时长：{{ daylightDuration }}</text>
					</view>
				</view>

				<!-- 第二排：右侧 体感指数卡片 -->
				<view class="sensor-card extra-card thi-card">
					<view class="card-header">
						<text class="card-icon">🧠</text>
						<text class="card-label">体感舒适度</text>
						<text class="thi-tag" :style="{backgroundColor: thiResult.color}">{{ thiResult.level }}</text>
					</view>
					<view class="card-body thi-display">
						<view class="thi-main">
							<text class="card-value">{{ thiResult.val }}</text>
							<text class="card-unit">THI</text>
						</view>

						<view class="thi-advice-box" :style="{ backgroundColor: thiResult.bgColor, borderColor: thiResult.borderColor , borderWidth: '1px',
    borderStyle: 'solid'}">
							<text class="thi-advice-text" :style="{ color: thiResult.textColor }">
								{{ thiResult.advice }}
							</text>
						</view>
					</view>
					<view class="card-footer">
						<text>体感评分: {{ thiResult.comfort }}</text>
					</view>
				</view>
			</view>
			<!--  硬件控制与报警设置 -->
			<view class="control-section" v-if="userRole === 'admin'">
				<!-- 硬件控制 -->
				<view class="card control-card">
					<text class="card-title">硬件设备控制</text>

					<view class="btn-group">
						<view class="control-item">
							<text class="status-label">照明灯状态: <text
									class="status-val">{{ getRelayText('state') }}</text></text>

							<view class="btns">
								<button class="btn on" @click="sendCommand('light:on')">开启灯光</button>
								<button class="btn off" @click="sendCommand('light:off')">关闭灯光</button>
							</view>
						</view>

						<view class="control-item">
							<text class="status-label">报警器功能: <text
									class="status-val">{{ getRelayText('alarm_switch') }}</text></text>
							<view class="btns">
								<button class="btn on" @click="sendCommand('alert:on')">开启报警</button>
								<button class="btn off" @click="sendCommand('alert:off')">关闭报警</button>
							</view>
						</view>

					</view>
					<view class="card-bottom-info">
						<text>最后同步时间: {{ lastUpdateTime }}</text>
						<text>通信方式：WebSocket</text>
					</view>
				</view>

				<view class="card setting-card">
					<view class="card-header-row">
						<text class="card-title">预警阈值设置</text>
					</view>
					<!-- 三列网格布局 -->
					<view class="threshold-grid-row">
						<view v-for="(val, key) in thresholds" :key="key" class="threshold-col-item">
							<!-- 头部：图标 + 标题 -->
							<view class="item-top">
								<text class="item-icon">{{ sensorConfig[key].icon }}</text>
								<text class="item-label">{{ sensorConfig[key].label }}</text>
							</view>

							<!-- 中部：双输入框 -->
							<view class="item-inputs-box">
								<input type="number" v-model.number="val.min" @change="saveThresholds" placeholder="下限"
									class="mini-input" />
								<text class="range-split">~</text>
								<input type="number" v-model.number="val.max" @change="saveThresholds" placeholder="上限"
									class="mini-input" />
							</view>

							<!-- 底部：单位显示 -->
							<text class="item-unit-tag">{{ sensorConfig[key].unit }}</text>
						</view>
					</view>

					<!-- 底部操作区：左右并排 -->
					<view class="card-footer-actions">
						<button class="footer-btn sync-btn" @click="syncThresholdsToHardware">下发配置</button>
						<button class="footer-btn reset-btn" @click="resetThresholds">恢复默认</button>
					</view>
				</view>
			</view>

			<!-- 实时趋势图表  -->
			<view class="card chart-card">
				<text class="card-title">传感器数据趋势图</text>
				<view class="charts-container">
					<view v-for="key in Object.keys(sensorConfig)" :key="key" class="chart-box">
						<text class="chart-subtitle">{{ sensorConfig[key].label }}趋势</text>
						<view :id="'chart-' + key" class="chart-dom"></view>
					</view>
				</view>
			</view>

			<!-- 数据明细列表  -->
			<view class="card list-card">
				<view class="card-header-row">
					<text class="card-title">实时数据明细</text>
					<button class="link-btn" @click="gotochaxun">历史记录查询 →</button>
				</view>
				<view class="data-table">
					<view class="table-thead">
						<text class="th">更新时间</text>
						<text class="th">温度</text>
						<text class="th">湿度</text>
						<text class="th">光照强度</text>
					</view>
					<view class="table-tbody">
						<view class="tr" v-for="(row, index) in historyList.slice(-5).reverse()" :key="index">
							<text class="td">{{ row.time }}</text>
							<text class="td">{{ row.temperature }}℃</text>
							<text class="td">{{ row.humidity }}%</text>
							<text class="td">{{ row.lightintensity }}Lux</text>
						</view>
					</view>
				</view>
			</view>
		</scroll-view>

		<!-- 报警弹窗 -->
		<view class="alarm-popup" v-if="showAlarmPopup">
			<view class="popup-content">
				<text class="icon">⚠️</text>
				<text class="msg">{{ alarmPopupMessage }}</text>
				<button class="close-x" @click="closeAlarmPopup">×</button>
			</view>
		</view>
	</view>
</template>

<script>
	import * as echarts from 'echarts';
	import {
		getWeatherData
	} from '@/utils/weather.js'

	export default {
		data() {
			return {
				location: '112.93,28.23',
				weather: {},
				sun: {},
				thi: {},

				currentTime: '',
				isConnected: false,
				showAlarmPopup: false,
				lastUpdateTime: '',
				alarmPopupMessage: '',
				socket: null,
				charts: {},
				userRole: 'user', // 存储当前角色
				// 核心配置
				sensorConfig: {
					temperature: {
						label: '实时温度',
						unit: '℃',
						icon: '🌡️'
					},
					humidity: {
						label: '空气湿度',
						unit: '%',
						icon: '💧'
					},
					lightintensity: {
						label: '光照强度',
						unit: 'Lux',
						icon: '☀️'
					}
				},

				// 阈值定义 
				thresholds: {
					temperature: {
						min: 10,
						max: 35
					},
					humidity: {
						min: 20,
						max: 80
					},
					lightintensity: {
						min: 0,
						max: 5000
					}
				},

				activeThresholds: {
					temperature: {
						min: 10,
						max: 35
					},
					humidity: {
						min: 20,
						max: 80
					},
					lightintensity: {
						min: 0,
						max: 5000
					}
				},


				// 实时数据
				currentData: {
					temperature: 0,
					humidity: 0,
					lightintensity: 0,
					state: 'guan', // 照明灯
					alarm_switch: 'guan' // 报警功能
				},

				// 绘图历史
				historyData: {
					timeStamps: [],
					temperature: [],
					humidity: [],
					lightintensity: []
				},
				historyList: [] // 用于列表展示
			};
		},
		onLoad() {
			//  严格检查登录状态
			const userInfo = uni.getStorageSync('userInfo');
			if (!userInfo) {
				uni.reLaunch({
					url: '/pages/index1/login'
				});
				return;
			}
			this.userRole = userInfo.role;

			//只有确定了权限，再初始化原来的逻辑
			this.loadSavedThresholds();
			this.activeThresholds = JSON.parse(JSON.stringify(this.thresholds));
			this.updateTime();
			setInterval(this.updateTime, 1000);
			this.connectWebSocket();
			this.initWeatherAndSun();


		},
		onReady() {
			this.initAllCharts();
		},
		computed: {
			// 计算温湿指数

			thiResult() {
				const t = this.getRealValue('temperature') || 0;
				const f = this.getRealValue('humidity') || 0;

				const fDecimal = f / 100;
				const thi = (1.8 * t + 32.0) - 0.55 * (1 - fDecimal) * (1.8 * t - 26);
				const val = parseFloat(thi.toFixed(1));

				// 根据分级标准返回结果
				if (val < 40) return {
					val,
					level: "极冷",
					color: "#00EBFF",
					comfort: "极不舒适",
					bgColor: "#f0f9ff",
					textColor: "#0c4a6e",
					borderColor: "#bae6fd",
					advice: "天气极冷，建议穿厚羽绒服，减少户外活动。"
				};
				if (val <= 45) return {
					val,
					level: "寒冷",
					color: "#00A2FF",
					comfort: "不舒适",
					bgColor: "#eff6ff",
					textColor: "#1e3a8a",
					borderColor: "#bfdbfe",
					advice: "天气寒冷，请穿冬装，注意防寒保暖。"
				};
				if (val <= 55) return {
					val,
					level: "偏冷",
					color: "#0051FF",
					comfort: "较不舒适",
					bgColor: "#f0fdfa",
					textColor: "#134e4a",
					borderColor: "#ccfbf1",
					advice: "有点冷，建议增加衣物，防止感冒。"
				};
				if (val <= 60) return {
					val,
					level: "清凉",
					color: "#00FFD5",
					comfort: "舒适",
					bgColor: "#ecfdf5",
					textColor: "#064e3b",
					borderColor: "#d1fae5",
					advice: "天气清凉，体感舒适，适合户外散步。"
				};
				if (val <= 65) return {
					val,
					level: "凉",
					color: "#00FF40",
					comfort: "非常舒适",
					bgColor: "#f0fdf4",
					textColor: "#14532d",
					borderColor: "#dcfce7",
					advice: "非常舒适，享受好天气。"
				};
				if (val <= 70) return {
					val,
					level: "暖",
					color: "#FFD500",
					comfort: "舒适",
					bgColor: "#fffbeb",
					textColor: "#78350f",
					borderColor: "#fef3c7",
					advice: "天气暖和，体感舒适，可以轻装出行。"
				};
				if (val <= 75) return {
					val,
					level: "偏热",
					color: "#FF9100",
					comfort: "较不舒适",
					bgColor: "#fff7ed",
					textColor: "#9a3412",
					borderColor: "#ffedd5",
					advice: "天气偏热，注意补充水分，防止中暑。"
				};
				if (val <= 80) return {
					val,
					level: "闷热",
					color: "#FF5E00",
					comfort: "不舒适",
					bgColor: "#fff1f2",
					textColor: "#9f1239",
					borderColor: "#ffe4e6",
					advice: "天气闷热，请开启风扇或空调，减少运动。"
				};
				return {
					val,
					level: "极闷热",
					color: "#FF0000",
					comfort: "极不舒适",
					bgColor: "#fef2f2",
					textColor: "#991b1b",
					borderColor: "#fee2e2",
					advice: "极其闷热！请避暑降温，严防中暑。"
				};
			},
			//  计算日照总时长
			daylightDuration() {
				if (!this.sun.sunrise || !this.sun.sunset) return '--小时--分';


				const timeToMin = (str) => {
					const [h, m] = str.split(':').map(Number);
					return h * 60 + m;
				};

				const start = timeToMin(this.sun.sunrise);
				const end = timeToMin(this.sun.sunset);
				const diff = end - start;

				const hours = Math.floor(diff / 60);
				const minutes = diff % 60;
				return `${hours}小时 ${minutes}分钟`;
			},

			//  动态计算太阳位置点 
			sunPosition() {
				if (!this.sun.sunrise || !this.sun.sunset) return 0;

				const timeToMin = (str) => {
					const [h, m] = str.split(':').map(Number);
					return h * 60 + m;
				};

				// 获取当前时间的分钟数 (例如 14:30 -> 870)
				const now = new Date();
				const current = now.getHours() * 60 + now.getMinutes();

				const start = timeToMin(this.sun.sunrise);
				const end = timeToMin(this.sun.sunset);

				// 如果还没日出，点在最左边
				if (current <= start) return 0;
				// 如果已经日落，点在最右边
				if (current >= end) return 100;

				// 计算百分比
				const percent = ((current - start) / (end - start)) * 100;
				return parseFloat(percent.toFixed(1));
			},
		},

		methods: {
			// 新增退出登录
			handleLogout() {
				uni.showModal({
					title: '确认退出',
					content: '是否退出当前系统？',
					success: (res) => {
						if (res.confirm) {
							uni.clearStorageSync(); // 清除缓存
							uni.reLaunch({
								url: '/pages/index1/login'
							});
						}
					}
				});
			},
			async initWeatherAndSun() {
				try {
					// 拆分 location 字符串 ：112.93 是经度 lng, 28.23 是纬度 lat
					const coords = this.location.split(',');
					const lng = parseFloat(coords[0]);
					const lat = parseFloat(coords[1]);

					// 调用封装好的请求
					const res = await getWeatherData(lat, lng);
					console.log('Open-Meteo 响应数据：', res);

					// 对称解析数据
					if (res.daily && res.daily.sunrise && res.daily.sunset) {
						this.sun = {
							sunrise: res.daily.sunrise[0].split('T')[1],
							sunset: res.daily.sunset[0].split('T')[1]
						};
						console.log('解析完成：', this.sun);
					}
				} catch (e) {
					console.error("Open-Meteo 请求失败：", e);
				}
			},
			// 逻辑函数：检查是否报警
			isAlarming(key) {
				const val = this.currentData[key];
				const th = this.activeThresholds[key];
				return val < th.min || val > th.max;
			},

			getRealValue(key) {
				return this.currentData[key] || 0;
			},

			getRelayText(key) {
				return this.currentData[key] === 'kai' ? '开启' : '关闭';
			},

			// WebSocket连接逻辑
			connectWebSocket() {
				this.socket = uni.connectSocket({
					url: 'ws://192.168.141.140:3000',
					success: () => {
						this.isConnected = true;
					}
				});
				this.socket.onMessage((res) => {
					const data = JSON.parse(res.data);
					this.processData(data);
				});
				this.socket.onClose(() => {
					this.isConnected = false;
					setTimeout(this.connectWebSocket, 5000);
				});
			},
			processData(resData) {
				try {
					// 处理硬件发的是数组包对象 
					const raw = Array.isArray(resData) ? resData[0] : resData;

					//定位到核心传感器数据
					const sensorRaw = raw.data || {};

					console.log('正在解析传感器原始数据:', sensorRaw);

					//建立硬件字段名与 App 字段名的映射关系
					// 硬件叫 illumination，App 叫 lightintensity
					const mapping = {
						temperature: sensorRaw.temperature,
						humidity: sensorRaw.humidity,
						lightintensity: sensorRaw.illumination
					};

					//循环更新 App 的实时数据
					Object.keys(this.sensorConfig).forEach(key => {
						let valStr = mapping[key];

						if (valStr !== undefined && valStr !== null) {
							// 使用 parseFloat 自动剥离 "℃", "%", "lx" 等单位后缀
							// 例如 parseFloat("20.83℃") 会直接变成数字 20.83
							const cleanVal = parseFloat(valStr);

							// 更新实时数据
							this.currentData[key] = cleanVal;

							// 更新图表数据
							this.updateChartData(key, cleanVal);

							this.lastUpdateTime = new Date().toLocaleTimeString();

							// 报警检测
							if (this.isAlarming(key)) {
								this.triggerAlarm(`${this.sensorConfig[key].label}异常：${valStr}`);
							}
						}
					});

					//更新列表明细
					const now = new Date().toLocaleTimeString();
					this.historyList.unshift({
						time: now,
						...this.currentData
					});
					if (this.historyList.length > 50) this.historyList.pop();

				} catch (e) {
					console.error("解析硬件JSON失败:", e);
				}
			},


			sendCommand(action) {
				if (!this.socket || !this.isConnected) {
					uni.showToast({
						title: '连接已断开',
						icon: 'none'
					});
					return;
				}

				// 建立一个映射表，把按钮动作转成硬件认识的 JSON 对象
				let hwCmd = {};
				switch (action) {
					case 'light:on':
						hwCmd = {
							"cmd": "control",
							"target": "led_on"
						};
						break;
					case 'light:off':
						hwCmd = {
							"cmd": "control",
							"target": "led_off"
						};
						break;
					case 'alert:on':
						hwCmd = {
							"cmd": "control",
							"target": "buzzer_on"
						};
						break;
					case 'alert:off':
						hwCmd = {
							"cmd": "control",
							"target": "buzzer_off"
						};
						break;
					default:
						console.warn("未知指令:", action);
						return;
				}

				// 构造发送给服务器的最终报文
				// 注意：硬件端接收的 code 必须是一个“字符串化的 JSON”
				//末尾加一个换行符用于判断指令结束
				const finalMsg = {
					name: "001",
					code: JSON.stringify(hwCmd) + "\n" // 重点：要把 hwCmd 对象转成字符串
				};

				console.log("正在发送硬件控制指令:", finalMsg);

				this.socket.send({
					data: JSON.stringify(finalMsg),
					success: () => {
						// 只要指令发出去成功了，我们就先让 UI 变过来
						if (action === 'light:on') this.currentData.state = 'kai';
						if (action === 'light:off') this.currentData.state = 'guan';
						if (action === 'alert:on') this.currentData.alarm_switch = 'kai';
						if (action === 'alert:off') this.currentData.alarm_switch = 'guan';
						uni.showToast({
							title: '指令已下发',
							icon: 'none'
						});
					}
				});
			},
			async syncThresholdsToHardware() {
				// 1. 基础检查
				if (!this.isConnected || !this.socket) {
					uni.showToast({
						title: '设备未连接',
						icon: 'none'
					});
					return;
				}

				//开启加载动画（对应 isProcessing 状态）
				uni.showLoading({
					title: '正在下发配置...'
				});

				// 内部小工具：构建硬件要求的 JSON 字符串格式
				const buildRawCmd = (type, min, max) => {
					return JSON.stringify({
						cmd: "set_limit",
						type: type,
						min: parseFloat(min).toFixed(1),
						max: parseFloat(max).toFixed(1)
					}) + "\n"; // 加 \n 方便硬件端按行读取
				};

				// 映射表：把 App 里的 key 映射成硬件认识的缩写
				const typeMapping = {
					temperature: "temp",
					humidity: "humi",
					lightintensity: "illumination"
				};

				try {
					console.log(">>> [阈值设置开始] 目标设备: 001");

					// 循环遍历 thresholds 对象，逐条下发
					const keys = Object.keys(this.thresholds);

					for (let i = 0; i < keys.length; i++) {
						const key = keys[i];
						const p = this.thresholds[key];
						const hardwareType = typeMapping[key] || key;

						// 生成指令字符串
						const cmdStr = buildRawCmd(hardwareType, p.min, p.max);

						// 构造 WebSocket 最终报文
						const finalMsg = {
							name: "001",
							code: cmdStr
						};

						// 执行发送
						this.socket.send({
							data: JSON.stringify(finalMsg)
						});

						console.log(`【${i + 1}/${keys.length} ${key}指令已下发】:`, cmdStr);

						//去掉语音避让，但保留 200ms 的极短延迟，确保硬件端能处理完上一条
						if (i < keys.length - 1) {
							await new Promise(resolve => setTimeout(resolve, 200));
						}
					}
					// 当所有指令发完后，更新“生效阈值”
					this.activeThresholds = JSON.parse(JSON.stringify(this.thresholds));

					// 保存到本地存储
					this.saveThresholds();
					// 全部成功后的反馈
					uni.hideLoading();
					uni.showToast({
						title: '配置下发成功！',
						icon: 'success'
					});

				} catch (err) {
					uni.hideLoading();
					uni.showToast({
						title: '同步异常',
						icon: 'none'
					});
					console.error("参数下发失败:", err);
				}
			},
			// 图表逻辑
			initAllCharts() {
				Object.keys(this.sensorConfig).forEach(key => {
					const dom = document.getElementById('chart-' + key);
					if (dom) {
						this.charts[key] = echarts.init(dom);
						this.updateChartOption(key);
					}
				});
			},

			updateChartData(key, value) {
				const now = new Date().toLocaleTimeString();
				if (key === 'temperature') { // 以温度为基准更新时间轴
					this.historyData.timeStamps.push(now);
					if (this.historyData.timeStamps.length > 15) this.historyData.timeStamps.shift();
				}

				if (this.historyData[key]) {
					this.historyData[key].push(value);
					if (this.historyData[key].length > 15) this.historyData[key].shift();
				}
				this.updateChartOption(key);
			},



			updateChartOption(key) {
				if (!this.charts[key]) return;

				// 获取当前传感器的配置（用于拿到单位和名称）
				const config = this.sensorConfig[key];

				this.charts[key].setOption({
					//增加悬浮提示框，手指点哪里，哪里显示具体数值和时间
					tooltip: {
						trigger: 'axis',
						backgroundColor: 'rgba(255, 255, 255, 0.9)',
						formatter: (params) => {
							const data = params[0];
							return `<div style="font-size:12px;color:#666;">时间：${data.name}</div>
			                        <div style="font-weight:bold;color:#3b82f6;">${config.label}：${data.value} ${config.unit}</div>`;
						}
					},
					grid: {
						top: 40,
						bottom: 30,
						left: 50,
						right: 20
					},
					xAxis: {
						type: 'category',
						data: this.historyData.timeStamps,
						show: true, // 显示 X 轴
						boundaryGap: false,
						axisLabel: {
							fontSize: 10,
							color: '#94a3b8',
							interval: 'auto', // 自动计算间隔，防止时间太挤
							hideOverlap: true // 自动隐藏重叠的标签
						},
						axisLine: {
							lineStyle: {
								color: '#e2e8f0'
							}
						}
					},
					yAxis: {
						type: 'value',
						name: `单位: ${config.unit}`, //显示单位
						nameTextStyle: {
							color: '#94a3b8',
							fontSize: 11,
							align: 'right',
							padding: [0, 5, 0, 0]
						},
						axisLabel: {
							fontSize: 10,
							color: '#94a3b8'
						},
						splitLine: {
							lineStyle: {
								type: 'dashed',
								color: '#f1f5f9'
							}
						}
					},
					series: [{
						data: this.historyData[key],
						type: 'line',
						smooth: true, // 丝滑曲线
						showSymbol: true, // 默认不显示圆点，hover 时才显示
						lineStyle: {
							width: 3,
							color: '#3b82f6'
						},
						areaStyle: {
							color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [{
								offset: 0,
								color: 'rgba(59, 130, 246, 0.3)'
							}, {
								offset: 1,
								color: 'rgba(59, 130, 246, 0)'
							}])
						}
					}]
				});
			},

			// 弹窗与配置持久化 
			triggerAlarm(msg) {
				this.alarmPopupMessage = msg;
				this.showAlarmPopup = true;
				// 5秒后自动关闭
				setTimeout(() => {
					this.showAlarmPopup = false;
				}, 5000);
			},

			closeAlarmPopup() {
				this.showAlarmPopup = false;
			},

			saveThresholds() {
				// 将修改的数值存入本地，下次打开仍旧显示改好的数字
				uni.setStorageSync('weather_thresholds', JSON.stringify(this.thresholds));
			},

			loadSavedThresholds() {
				const saved = localStorage.getItem('weather_thresholds');
				if (saved) {
					this.thresholds = JSON.parse(saved);
					// 初始时，生效状态 = 存储的状态
					this.activeThresholds = JSON.parse(JSON.stringify(this.thresholds));
				}
			},

			resetThresholds() {
				// 仅仅修改“草稿箱”里的数值（输入框会跟着变）
				this.thresholds = {
					temperature: {
						min: 10,
						max: 35
					},
					humidity: {
						min: 20,
						max: 80
					},
					lightintensity: {
						min: 0,
						max: 5000
					}
				};

				//将这些默认值保存到本地缓存（防止刷新页面后又变回乱七八糟的数字）
				this.saveThresholds();

				//给出提示，告诉用户还没完事，得点下发才行
				uni.showToast({
					title: '已重置(请点击下发生效)',
					icon: 'success',
					duration: 2000
				});


			},

			updateTime() {
				const now = new Date();
				const year = now.getFullYear();
				const month = (now.getMonth() + 1).toString().padStart(2, '0');
				const day = now.getDate().toString().padStart(2, '0');
				const hh = now.getHours().toString().padStart(2, '0');
				const mm = now.getMinutes().toString().padStart(2, '0');
				const ss = now.getSeconds().toString().padStart(2, '0');
				this.currentTime = `${year}-${month}-${day} ${hh}:${mm}:${ss}`;
			},

			gotochaxun() {
				uni.navigateTo({
					url: "/pages/index1/chaxun"
				});
			}
		},

	}
</script>

<style scoped>
	/* 基础样式 */
	.container {
		background-color: #f0f4f8;
		height: 100vh;
		display: flex;
		flex-direction: column;
	}

	/* 导航栏样式升级 */
	.navbar {
		background: linear-gradient(90deg, #1e3a8a 0%, #3b82f6 100%);
		padding: 15px 20px;
		display: flex;
		justify-content: space-between;
		align-items: center;
		color: white;
		box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
	}

	.navbar-title {
		font-size: 20px;
		font-weight: bold;
		letter-spacing: 1px;
	}

	/* 导航栏右侧样式 */
	.nav-right-user {
		display: flex;
		flex-direction: row;
		align-items: center;
	}

	.current-time {
		font-size: 16px;
		/* 稍微调小一点点，更精致 */
		font-family: monospace;
		/* 等宽字体，数字跳动时不会抖动 */
		color: rgba(255, 255, 255, 0.9);
		margin-right: 30rpx;
		/* 和身份标识拉开距离 */
	}

	.role-text {
		font-size: 12px;
		background: rgba(255, 255, 255, 0.2);
		padding: 2px 8px;
		border-radius: 4px;
		margin-right: 15px;
	}


	.logout-btn {
		font-size: 24rpx;
		background: #ef4444;
		color: white;
		padding: 0 20rpx;
		height: 54rpx;
		line-height: 54rpx;
		border-radius: 8rpx;
		margin: 0;
	}

	.status-tag {
		font-size: 12px;
		margin-left: 10px;
		padding: 2px 8px;
		border-radius: 20px;
		background: rgba(0, 0, 0, 0.2);
	}

	.status-tag.online {
		color: #4ade80;
	}

	.main-content {
		flex: 1;
		padding: 15px;
	}

	/* 数据卡片网格 */

	.data-grid {
		display: flex;
		flex-wrap: wrap;
		/* 允许换行 */
		justify-content: space-between;
		padding: 10px;
		gap: 15px;
		/* 卡片之间的间距 */
	}

	/* 所有的基础卡片样式（ */
	.sensor-card {
		background-color: #ffffff;
		border-radius: 12px;
		padding: 15px;
		box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
		/* 默认三个一排的宽度计算 */
		width: calc(33.33% - 12px);
		box-sizing: border-box;
		min-height: 160px;
		display: flex;
		flex-direction: column;
		justify-content: space-between;
	}

	/* 针对第二排两个卡片的特殊处理 */
	.extra-card {
		width: calc(50% - 8px);
		/* 两个一排 */
		margin-top: 5px;
	}

	/* 报警变色样式 */
	.sensor-card.alarm-active {
		background: #fee2e2;
		border-color: #ef4444;
		animation: pulse 2s infinite;
	}

	.card-header {
		display: flex;
		align-items: center;
		margin-bottom: 15px;
	}

	.card-icon {
		font-size: 24px;
		margin-right: 10px;
	}

	.card-label {
		font-size: 16px;
		color: #64748b;
	}

	.card-body {
		text-align: center;
		margin-bottom: 10px;
	}

	.card-value {
		font-size: 36px;
		font-weight: bold;
		color: #1e293b;
	}

	.card-unit {
		font-size: 18px;
		color: #94a3b8;
		margin-left: 5px;
	}

	.card-footer {
		font-size: 12px;
		color: #94a3b8;
		border-top: 1px solid #f1f5f9;
		padding-top: 10px;
	}

	/* 通用卡片样式 */
	.card {
		background: white;
		border-radius: 16px;
		padding: 20px;
		margin-bottom: 0;
		box-shadow: 0 4px 6px rgba(0, 0, 0, 0.05);
	}

	.card-title {
		font-size: 18px;
		font-weight: bold;
		color: #1e3a8a;
		margin-bottom: 15px;
		border-left: 4px solid #3b82f6;
		padding-left: 10px;
	}

	/* 环境指数与日落日出  */
	.weather-extra-info {
		display: flex;
		justify-content: space-between;
		padding: 0 30rpx;
		margin-top: 20rpx;
	}

	.mini-card {
		background: #ffffff;
		border-radius: 16rpx;
		padding: 20rpx;
		box-shadow: 0 4rpx 12rpx rgba(0, 0, 0, 0.05);
	}

	/* 日出日落内部布局 */
	.sun-display {
		display: flex;
		align-items: center;
		justify-content: space-around;
		padding: 10px 0;
	}

	.sun-item {
		display: flex;
		flex-direction: column;
		align-items: center;
	}

	.sun-box {
		width: 42%;
		display: flex;
		flex-direction: column;
		justify-content: space-between;
	}


	.sun-time {
		font-size: 20px;
		font-weight: bold;
		color: #333;
	}

	.sun-sub {
		font-size: 12px;
		color: #999;
	}

	.sun-progress {
		flex: 1;
		height: 2px;
		background: #eee;
		margin: 0 15px;
		position: relative;
	}

	.sun-icon {
		font-size: 32rpx;
		margin-right: 10rpx;
	}

	.sun-label {
		font-size: 20rpx;
		color: #999;
		display: block;
	}


	.sun-line {
		height: 4rpx;
		background: #f0f0f0;
		margin: 15rpx 0;
		position: relative;
	}

	.sun-dot {
		width: 12rpx;
		height: 12rpx;
		background: #ff9800;
		border-radius: 50%;
		position: absolute;
		top: -4rpx;
		left: 40%;
		/* 增加平滑过渡效果 */
		transition: left 1s linear;
		box-shadow: 0 0 8px #ff9800;
	}

	/* 体感指数内部布局 */
	.thi-display {
		display: flex;
		align-items: center;
		justify-content: space-between;
	}

	.thi-tag {
		font-size: 12px;
		color: #fff;
		padding: 2px 8px;
		border-radius: 4px;
		margin-left: 10px;
	}


	.thi-advice-box {
		flex: 1;
		margin-left: 15px;
		padding: 12px 16px;
		border-radius: 8px;
		display: flex;
		align-items: center;
		justify-content: center;
		transition: all 0.3s;
	}

	.thi-advice-text {
		font-size: 14px;
		line-height: 1.6;
		text-align: center;
		font-weight: 500;
	}

	.thi-box {
		width: 48%;
	}

	.thi-header {
		display: flex;
		justify-content: space-between;
		align-items: baseline;
	}

	.thi-title {
		font-size: 24rpx;
		color: #666;
	}

	.thi-num {
		font-size: 36rpx;
		font-weight: bold;
		color: #2196F3;
	}

	.thi-status {
		font-size: 26rpx;
		font-weight: bold;
		display: block;
		margin-top: 10rpx;
		color: #333;
	}

	.thi-advice {
		font-size: 20rpx;
		color: #888;
		margin-top: 6rpx;
		display: block;
	}

	/* 报警激活状态保持不变 */
	.alarm-active {
		background-color: #fff1f0;
		border: 1px solid #ffa39e;
	}

	/* 不同等级的颜色反馈 */
	.thi-lv-1 .thi-num,
	.thi-lv-2 .thi-num {
		color: #003366;
	}

	/* 深蓝 */
	.thi-lv-4 .thi-num,
	.thi-lv-5 .thi-num {
		color: #4CAF50;
	}

	/* 绿色 */
	.thi-lv-8 .thi-num,
	.thi-lv-9 .thi-num {
		color: #F44336;
	}


	/* 硬件控制 */
	.control-section {
		display: grid;
		grid-template-columns: 1fr 1fr;
		gap: 15px;
		margin-bottom: 25px;
		align-items: stretch;
	}

	.control-card {
		display: flex;
		flex-direction: column;
		height: 100%;
		box-sizing: border-box;
	}

	.btn-group {
		flex: 1;
		display: flex;
		flex-direction: column;
		justify-content: space-around;
		padding: 10px 0;
	}

	.control-item {
		margin-bottom: 0;
	}

	.status-label {
		font-size: 14px;
		color: #64748b;
		margin-bottom: 8px;
		display: block;
	}

	.status-val {
		font-weight: bold;
		color: #1e3a8a;
		margin-left: 5px;
	}

	.btns {
		display: flex;
		gap: 10px;
		margin-top: 8px;
	}

	.btn {
		flex: 1;
		font-size: 14px;
		padding: 6px;
		border: none;
		border-radius: 8px;
		color: white;
		cursor: pointer;
	}


	/* 开启状态 (绿色) */
	.btn.on {
		background: #10b981;
		transition: all 0.1s;
	}

	.btn.on:active {
		background: #059669;
	}

	/* 关闭状态 (灰蓝色) */
	.btn.off {
		background: #64748b;
		transition: all 0.1s;
	}

	.btn.off:active {
		background: #475569;
		transform: scale(0.95);
	}

	.card-bottom-info {
		border-top: 1px solid #f1f5f9;
		padding-top: 10px;
		display: flex;
		justify-content: space-between;
		font-size: 11px;
		color: #94a3b8;
	}

	/* 阈值设置 */
	/* 三列并排容器 */
	.threshold-grid-row {
		display: flex;
		justify-content: space-between;
		gap: 12px;
		/* 盒子之间的间距 */
		margin: 15px 0;
	}

	/*单个设置小盒子 */
	.threshold-col-item {
		flex: 1;
		background-color: #f8fafc;
		border-radius: 10px;
		padding: 12px 8px;
		display: flex;
		flex-direction: column;
		align-items: center;
		transition: all 0.2s;
	}

	.threshold-col-item:hover {
		border-color: #007aff;
		background-color: #ffffff;
	}

	/*小盒子内部细节 */
	.item-top {
		display: flex;
		align-items: center;
		margin-bottom: 10px;
	}

	.item-icon {
		font-size: 18px;
		margin-right: 4px;
	}

	.item-label {
		font-size: 16px;
		font-weight: bold;
		color: #475569;
	}

	/* 输入框组合 */
	.item-inputs-box {
		display: flex;
		align-items: center;
		justify-content: center;
		width: 100%;
	}

	.mini-input {
		width: 45px;
		height: 28px;
		background: #ffffff;
		border: 1px solid #cbd5e1;
		border-radius: 4px;
		text-align: center;
		font-size: 15px;
	}

	.range-split {
		margin: 0 4px;
		color: #94a3b8;
		font-size: 12px;
	}

	.item-unit-tag {
		font-size: 15px;
		color: #94a3b8;
		margin-top: 8px;
	}

	/* 图表区域 */
	.charts-container {
		display: flex;
		flex-direction: column;
		gap: 20px;
	}

	.chart-box {
		height: 240px;
		margin-bottom: 15px;
	}

	.chart-subtitle {
		font-size: 14px;
		color: #64748b;
		margin-bottom: 5px;
	}

	.chart-dom {
		height: 200px;
		width: 100%;
	}

	/* 表格样式 */
	.data-table {
		border: 1px solid #e2e8f0;
		border-radius: 8px;
		overflow: hidden;
	}

	.table-thead {
		display: flex;
		background: #f8fafc;
		font-weight: bold;
		padding: 10px;
	}

	.table-tbody .tr {
		display: flex;
		padding: 10px;
		border-top: 1px solid #f1f5f9;
	}

	.th,
	.td {
		flex: 1;
		text-align: center;
		font-size: 13px;
	}

	/* 报警弹窗 */
	.alarm-popup {
		position: fixed;
		top: 20px;
		left: 50%;
		transform: translateX(-50%);
		width: 90%;
		max-width: 400px;
		background: #ef4444;
		color: white;
		padding: 15px;
		border-radius: 12px;
		box-shadow: 0 10px 15px rgba(0, 0, 0, 0.2);
		z-index: 9999;
	}

	.popup-content {
		display: flex;
		align-items: center;
		position: relative;
	}

	.popup-content .icon {
		margin-right: 10px;
		font-size: 20px;
	}

	.close-x {
		background: transparent;
		border: none;
		color: white;
		font-size: 24px;
		position: absolute;
		right: 0;
	}

	@keyframes pulse {
		0% {
			transform: scale(1);
		}

		50% {
			transform: scale(1.02);
			box-shadow: 0 0 15px rgba(239, 68, 68, 0.3);
		}

		100% {
			transform: scale(1);
		}
	}

	.mini-btn {
		font-size: 12px;
		padding: 0 10px;
		height: 28px;
		line-height: 28px;
		margin: 0;
	}

	.link-btn {
		font-size: 14px;
		background-color: #007aff;
		color: #fff;
		border: none;
		transition: all 0.1s;
		/* 防止按钮瞬间回弹 */
	}

	.link-btn:active {
		background-color: #0056b3;
		transform: scale(0.98);
	}

	.card-header-row {
		display: flex;
		justify-content: space-between;
		align-items: center;
		margin-bottom: 15px;
	}


	.card-footer-actions {
		margin-top: 20px;
		padding-top: 15px;
		border-top: 1px solid #eee;
		/* 加一条细线分隔 */
		display: flex;
		flex-direction: column;
		/* 垂直排列 */
		align-items: center;
		gap: 10px;
	}

	.footer-btn {
		width: 100%;
		height: 42px;
		line-height: 42px;
		border-radius: 8px;
		font-size: 14px;
		font-weight: 500;
		margin-bottom: 10px;
		/* 两个按钮之间的间距 */
		border: none;
		transition: all 0.1s;
		/* 让点击效果更平滑 */
		display: flex;
		justify-content: center;
		align-items: center;
	}


	/* 下发按钮用蓝色 */
	.sync-btn {
		background-color: #007aff;
		color: #fff;
		border: none;
		transition: all 0.1s;
	}

	.sync-btn:active {
		background-color: #0056b3;
		transform: scale(0.98);
	}

	/* 恢复默认 - 次按钮 (浅灰色) */
	.reset-btn {
		background-color: #f5f5f5;
		color: #666666;
		transition: all 0.1s;
		/* 防止按钮瞬间回弹 */
	}

	/* 恢复默认 - 点击效果：背景变灰一点 */
	.reset-btn:active {
		background-color: #e0e0e0;
		transform: scale(0.98);
		/* 让按钮稍微变小 模拟物理反馈 */
	}

@media (max-width: 768px) {
    /* 导航栏适配：允许换行，高度自适应 */
	  .container {
	        padding: 0 !important; 
	    }
  .main-content {
        padding: 20rpx 15rpx !important; /* 缩小左右间距，让卡片看起来更宽、更靠左 */
    }

    .navbar {
		  width: 100%;
        height: auto;
          padding: 20rpx 20rpx !important;
		  box-sizing: border-box;
		   border-radius: 0;
        flex-direction: column; /* 手机端标题和用户信息上下排布 */
        align-items: flex-start;
    }
      .navbar-title {
            font-size: 32rpx !important;
        }

    .nav-left {
        margin-bottom: 15rpx;
		  gap: 10rpx;
    }
    
    .nav-right-user {
        width: 100%;
        justify-content: space-between; /* 手机端时间在左，退出在右 */
    }

    .current-time {
        font-size: 22rpx;
        margin-right: 10rpx;
    }

    /* 传感器卡片适配：全部变为一列撑满 */
    .sensor-card, .extra-card {
        width: 100% !important; /* 强制手机端占满一行 */
		margin-left: 0 !important;
		box-sizing: border-box;
        margin-bottom: 20rpx;
    }
  .data-grid {
        padding: 0 !important;
        margin: 0 !important;
        gap: 20rpx !important; /* 卡片之间的上下间距 */
    }

    /* 硬件控制适配：垂直堆叠 */
    .control-section {
        grid-template-columns: 1fr; /* 变为一列 */
		  margin-top: 20rpx;
    }
    
    /* 调整下发配置按钮间距 */
    .card-footer-actions {
        flex-direction: column;
    }
	.card {
	        padding: 20rpx !important;
	        border-radius: 12rpx !important;
	    }
	
	    .card-title {
	        font-size: 28rpx !important;
	        margin-bottom: 20rpx !important;
	    }
	
	    /* 2. 硬件控制区域适配 */
	    .control-item {
	        margin-bottom: 15rpx !important;
	    }
	    
	    .status-label {
	        font-size: 24rpx !important;
	        margin-bottom: 10rpx !important;
	    }
	
	    .btns {
	        gap: 8rpx !important; /* 缩小按钮间距 */
	    }
	
	    .btn {
	        height: 60rpx !important;   /* 减小按钮高度 */
	        line-height: 60rpx !important;
	        font-size: 24rpx !important; /* 减小文字 */
	        padding: 0 !important;
	    }
	
	    /* 3. 预警阈值设置区域适配（防止被砍掉的核心） */
	    .threshold-grid-row {
	        gap: 8rpx !important; /* 极度缩小卡片间距 */
	        margin: 10rpx 0 !important;
	        flex-wrap: nowrap !important; /* 强制不换行 */
	    }
	
	    .threshold-col-item {
	        padding: 10rpx 4rpx !important; /* 极简内边距 */
	        min-width: 0 !important;        /* 允许盒子收缩 */
	    }
	
	    .item-top {
	        margin-bottom: 8rpx !important;
	    }
	
	    .item-icon {
	        font-size: 28rpx !important; /* 缩小图标 */
	    }
	
	    .item-label {
	        font-size: 22rpx !important; /* 缩小标签文字 */
	    }
	
	    /* 4. 输入框“瘦身” */
	    .item-inputs-box {
	        transform: scale(0.9); /* 整体微调缩小，防止撑破盒子 */
	    }
	
	    .mini-input {
	        width: 70rpx !important; /* 严格限制输入框宽度 */
	        height: 44rpx !important;
	        font-size: 22rpx !important;
	    }
	
	    .range-split {
	        margin: 0 2rpx !important;
	    }
	
	    .item-unit-tag {
	        font-size: 20rpx !important;
	        margin-top: 5rpx !important;
	    }
	
	    /* 5. 下方操作按钮 */
	    .card-footer-actions {
	        margin-top: 15rpx !important;
	        padding-top: 10rpx !important;
	        gap: 10rpx !important;
	    }
	
	    .footer-btn {
	        height: 70rpx !important;
	        line-height: 70rpx !important;
	        font-size: 26rpx !important;
	    }
}
</style>