<template>
	<view class="container">
		<!--  顶部导航与返回 -->
		<view class="header-nav">
			<button class="back-btn" @click="goBack">← 返回看板</button>
			<text class="page-title">历史气象数据深度查询</text>
		</view>

		<scroll-view scroll-y="true" class="scroll-body">
			<!-- 筛选区：时间选择与操作  -->
			<view class="card filter-card">

				<view class="filter-row">
					<text class="label">查询时段：</text>
					<!-- 增加了 border 属性，让它显示出边框 -->
					<uni-datetime-picker v-model="range" type="datetimerange" rangeSeparator="至"
						@change="onPickerChange" :clear-icon="false" border class="date-picker-ui" />
				</view>
				<view class="shortcut-row">
					<button class="mini-btn" :class="{ 'active': activeShortcut === 'today' }"
						@click="setRange('today')">今天</button>
					<button class="mini-btn" :class="{ 'active': activeShortcut === 'yesterday' }"
						@click="setRange('yesterday')">昨天</button>
					<button class="mini-btn" :class="{ 'active': activeShortcut === 'week' }"
						@click="setRange('week')">最近7天</button>
					<button class="query-btn" @click="fetchHistoryData">立即查询</button>
					<button class="export-btn" @click="exportData">导出数据</button>
				</view>
			</view>

			<!--  统计概览区 -->
			<view class="stats-grid" v-if="historyRecords.length > 0">
				<view class="stats-card temp">
					<text class="st-label">时段均温</text>
					<text class="st-value">{{ stats.avgTemp }}<text class="st-unit">℃</text></text>
				</view>
				<view class="stats-card humi">
					<text class="st-label">时段均湿</text>
					<text class="st-value">{{ stats.avgHumi }}<text class="st-unit">%</text></text>
				</view>
				<view class="stats-card light">
					<text class="st-label">平均光照强度</text>
					<text class="st-value">{{ stats.avgLight }}<text class="st-unit">Lux</text></text>
				</view>
			</view>

			<!--  核心图表区：三轴联动折线图  -->
			<view class="card chart-main-card">
				<view class="card-header">
					<text class="card-title">趋势对比分析 (温/湿/光)</text>
					<text class="tip">提示：左右拖动查看更多趋势</text>
				</view>
				<view id="history-linked-chart" class="main-chart"></view>
			</view>

			<!--  数据明细清单 -->
			<view class="card table-card">
				<text class="card-title">历史数据原始清单</text>
				<view class="data-table">
					<view class="thead">
						<text class="th">采集时间</text>
						<text class="th">温度</text>
						<text class="th">湿度</text>
						<text class="th">光强</text>
					</view>
					<view class="tbody">
						<view class="tr" v-for="(item, index) in historyRecords" :key="index">
							<text class="td time">{{ item.time }}</text>
							<text class="td">{{ item.temperature }}℃</text>
							<text class="td">{{ item.humidity }}%</text>
							<text class="td light-td">{{ item.lightintensity }}Lux</text>
						</view>
					</view>
					<view v-if="historyRecords.length === 0" class="empty">暂无查询记录，请选择时间后查询</view>
				</view>
			</view>
		</scroll-view>
	</view>
</template>

<script>
	import * as echarts from 'echarts';

	export default {
		data() {
			return {
				range: [], // 时间范围 [start, end]
				activeShortcut: 'today', // 默认选中“今天”
				historyRecords: [], // 表格原始数据
				chartInstance: null,
				stats: {
					maxTemp: 0,
					minHumi: 0,
					avgTemp: 0, 
					avgHumi: 0, 
					avgLight: 0
				},
			};
		},
		onLoad() {
			this.setRange('today');
		},
		onReady() {
			this.initChart();
			this.fetchHistoryData();
		},
		methods: {
			goBack() {
				uni.navigateBack();
			},

			// 快捷设置查询时间
			setRange(type) {
				// 设置选中状态
				this.activeShortcut = type;
				const end = new Date();
				const start = new Date();
				if (type === 'today') start.setHours(0, 0, 0, 0);
				if (type === 'yesterday') {
					start.setDate(start.getDate() - 1);
					start.setHours(0, 0, 0, 0);
					end.setDate(end.getDate() - 1);
					end.setHours(23, 59, 59, 999);
				}
				if (type === 'week') start.setDate(start.getDate() - 7);

				this.range = [this.formatDate(start), this.formatDate(end)];

				// 立即执行一次查询，刷新图表和统计卡片
				this.$nextTick(() => {
					this.fetchHistoryData();
				});

				//  增加反馈提示，让操作更明显
				uni.showToast({
					title: '已切换至' + (type === 'today' ? '今日' : type === 'yesterday' ? '昨日' : '近七天'),
					icon: 'none',
					duration: 1000
				});
			},

			// 如果手动选择了日历
			// 用户一旦点日历选择了自定义时间，那么“今天/昨天/七天”的按钮应该全部**熄灭**。
			// 修改立即查询按钮的方法
			onManualQuery() {
				this.activeShortcut = ''; // 清除快捷键选中状态
				this.fetchHistoryData();
			},
			// 当用户手动在日历里选了时间
			onPickerChange(e) {
				this.activeShortcut = ''; // 熄灭“今天/昨天”按钮
				console.log('手动选择的时间范围：', e);
				// 如果想选完立即查询，可以加上下面这句
				// this.fetchHistoryData(); 
			},
			// 格式化时间
			formatDate(date) {
				const y = date.getFullYear();
				const m = (date.getMonth() + 1).toString().padStart(2, '0');
				const d = date.getDate().toString().padStart(2, '0');
				const hh = date.getHours().toString().padStart(2, '0');
				const mm = date.getMinutes().toString().padStart(2, '0');
				const ss = date.getSeconds().toString().padStart(2, '0');
				return `${y}-${m}-${d} ${hh}:${mm}:${ss}`;
			},
fetchHistoryData() {
    if (!this.range || this.range.length < 2) {
        uni.showToast({
            title: '请选择时间范围',
            icon: 'none'
        });
        return;
    }

    uni.showLoading({
        title: '正在调取历史记录...',
        mask: true // 建议增加遮罩，防止加载时用户乱点
    });

    uni.request({
        url: 'http://192.168.141.140:5000/api/sensors',
        method: 'GET',
        data: {
            name: '001', // 硬件 ID
            start: this.range[0], // 开始时间
            end: this.range[1], // 结束时间
            limit: 1000 // 限制条数
        },
        success: (res) => {
            // 【关键点 1】：在处理数据前先关闭 Loading，防止被后续的 Toast 干扰
            uni.hideLoading();

            if (res.statusCode === 200 && res.data) {
                console.log('收到原始数据，条数：', res.data.length);

                const formattedData = res.data.map(item => {
                    try {
                        const sensorObj = JSON.parse(item.data);
                        return {
                            time: item.timestr, 
                            temperature: !isNaN(parseFloat(sensorObj.temperature)) ?
                                parseFloat(sensorObj.temperature).toFixed(2) : '0.00',
                            humidity: !isNaN(parseFloat(sensorObj.humidity)) ?
                                parseFloat(sensorObj.humidity).toFixed(2) : '0.00',
                            lightintensity: !isNaN(parseFloat(sensorObj.illumination)) ?
                                parseFloat(sensorObj.illumination).toFixed(2) : '0.00'
                        };
                    } catch (e) {
                        console.error('单条解析失败', e);
                        return {
                            time: item.timestr,
                            temperature: '0.00',
                            humidity: '0.00',
                            lightintensity: '0.00'
                        };
                    }
                });

                this.historyRecords = formattedData;

                this.$nextTick(() => {
                    this.calculateStats(); 
                    this.renderChart(); 
                });

                uni.showToast({
                    title: '数据同步完成',
                    icon: 'success'
                });
            } else {
                uni.showToast({
                    title: '未查询到相关数据',
                    icon: 'none'
                });
            }
        },
        fail: (err) => {
            // 【关键点 2】：失败时也要第一时间关闭 Loading
            uni.hideLoading();
            
            uni.showToast({
                title: '无法连接服务器',
                icon: 'none'
            });
            console.error('网络请求失败：', err);
        }
        // 【注意】：删除了原来的 complete 回调，因为 success 和 fail 里已经处理了隐藏逻辑
    });
},
			// 计算统计概览
			calculateStats() {
				if (this.historyRecords.length === 0) return;
				const temps = this.historyRecords.map(i => parseFloat(i.temperature));
				const humis = this.historyRecords.map(i => parseFloat(i.humidity));
				const lights = this.historyRecords.map(i => parseFloat(i.lightintensity));

				this.stats.maxTemp = Math.max(...temps);
				this.stats.minHumi = Math.min(...humis);
				this.stats.avgLight = Math.floor(lights.reduce((a, b) => a + b, 0) / lights.length);

				// 计算平均温度
				const sumTemp = temps.reduce((acc, val) => acc + val, 0);
				this.stats.avgTemp = (sumTemp / temps.length).toFixed(1);

				// 计算平均湿度
				const sumHumi = humis.reduce((acc, val) => acc + val, 0);
				this.stats.avgHumi = (sumHumi / humis.length).toFixed(1);
			},

			// 初始化图表结构
			initChart() {
				const dom = document.getElementById('history-linked-chart');
				this.chartInstance = echarts.init(dom);
			},

			// 渲染三轴联动图表 
			renderChart() {
				const times = this.historyRecords.map(i => i.time.split(' ')[1]); // 仅显示时间部分
				const temps = this.historyRecords.map(i => i.temperature);
				const humis = this.historyRecords.map(i => i.humidity);
				const lights = this.historyRecords.map(i => i.lightintensity);

				const option = {
					tooltip: {
						trigger: 'axis',
						axisPointer: {
							type: 'line',
							link: {
								xAxisIndex: 'all'
							}
						}
					},
					grid: [{
							left: 80,
							right: 30,
							height: '22%',
							top: '10%'
						},
						{
							left: 80,
							right: 30,
							height: '22%',
							top: '40%'
						},
						{
							left: 80,
							right: 30,
							height: '22%',
							top: '70%'
						}
					],
					xAxis: [{
							gridIndex: 0,
							type: 'category',
							data: times,
							show: false
						},
						{
							gridIndex: 1,
							type: 'category',
							data: times,
							show: false
						},
						{
							gridIndex: 2,
							type: 'category',
							data: times,
							show: true,
							axisLabel: {
								fontSize: 10,
								color: '#94a3b8'
							}
						}
					],
					yAxis: [ // 温度 Y 轴
						{
							gridIndex: 0,
							name: '℃',
							nameLocation: 'end',
							nameTextStyle: {
								padding: [0, 50, 0, 0],
								color: '#ef4444'
							},
							type: 'value',
							axisLabel: {
								fontSize: 11,
								color: '#64748b'
							},
							splitLine: {
								lineStyle: {
									type: 'dashed',
									color: '#f1f5f9'
								}
							}
						},
						// 湿度 Y 轴
						{
							gridIndex: 1,
							name: '%',
							nameLocation: 'end',
							nameTextStyle: {
								padding: [0, 50, 0, 0],
								color: '#3b82f6'
							},
							type: 'value',
							axisLabel: {
								fontSize: 11,
								color: '#64748b'
							},
							splitLine: {
								lineStyle: {
									type: 'dashed',
									color: '#f1f5f9'
								}
							}
						},
						// 光照 Y 轴
						{
							gridIndex: 2,
							name: 'Lux',
							nameLocation: 'end',
							nameTextStyle: {
								padding: [0, 60, 0, 0],
								color: '#f59e0b'
							},
							type: 'value',
							axisLabel: {
								fontSize: 11,
								color: '#64748b'
							},
							splitLine: {
								lineStyle: {
									type: 'dashed',
									color: '#f1f5f9'
								}
							}
						}
					],
					series: [{
							name: '温度',
							type: 'line',
							xAxisIndex: 0,
							yAxisIndex: 0,
							data: temps,
							color: '#ef4444',
							smooth: true,
							areaStyle: {
								opacity: 0.1
							}
						},
						{
							name: '湿度',
							type: 'line',
							xAxisIndex: 1,
							yAxisIndex: 1,
							data: humis,
							color: '#3b82f6',
							smooth: true,
							areaStyle: {
								opacity: 0.1
							}
						},
						{
							name: '光强',
							type: 'line',
							xAxisIndex: 2,
							yAxisIndex: 2,
							data: lights,
							color: '#f59e0b',
							smooth: true,
							areaStyle: {
								opacity: 0.1
							}
						}
					]
				};
				this.chartInstance.setOption(option);
			},


			jsonToCsv(data) {
				const header = ['更新时间', '温度(℃)', '湿度(%)', '光照强度(Lux)'];
				const rows = data.map(item => [
					`"${item.timestr || item.time}"`,
					item.temperature,
					item.humidity,
					item.lightintensity
				]);

				return "\ufeff" + [header, ...rows].map(e => e.join(",")).join("\n");
			},


			exportData() {
				// 获取数据
				const dataToExport = this.historyRecords || [];
				if (dataToExport.length === 0) {
					uni.showToast({
						title: '没有数据可导出',
						icon: 'none'
					});
					return;
				}

				uni.showLoading({
					title: '正在准备数据...'
				});
				const csvString = this.jsonToCsv(dataToExport);

				// #ifdef H5
				// H5 浏览器环境逻辑
				try {
					const blob = new Blob([csvString], {
						type: 'text/csv;charset=utf-8;'
					});
					const url = window.URL.createObjectURL(blob);
					const link = document.createElement("a");
					link.href = url;
					link.download = `气象数据导出_${new Date().getTime()}.csv`;
					document.body.appendChild(link);
					link.click();
					document.body.removeChild(link);
					window.URL.revokeObjectURL(url);

					uni.hideLoading();
					uni.showToast({
						title: '导出成功，正在下载',
						icon: 'none'
					});
				} catch (e) {
					uni.hideLoading();
					console.error("H5导出失败", e);
				}
				// #endif

				// #ifdef APP-PLUS
				// 手机 App 环境逻辑
				const fileName = `weather_data_${new Date().getTime()}.csv`;
				const filePath = `_doc/${fileName}`;

				plus.io.requestFileSystem(plus.io.PRIVATE_DOC, (fs) => {
					fs.root.getFile(fileName, {
						create: true
					}, (fileEntry) => {
						fileEntry.createWriter((writer) => {
							writer.write(csvString);
							writer.onwriteend = () => {
								uni.hideLoading();
								uni.shareWithSystem({
									summary: '气象数据导出',
									href: filePath,
									success: () => {
										console.log('分享成功');
									},
									fail: () => {
										uni.showModal({
											title: '保存成功',
											content: '文件已存至：' + filePath
										});
									}
								});
							};
						});
					});
				});
				// #endif

				// #ifdef MP-WEIXIN
				//微信小程序逻辑
				uni.setClipboardData({
					data: csvString,
					success: () => {
						uni.hideLoading();
						uni.showModal({
							title: '导出成功',
							content: 'CSV数据已复制，请粘贴到文件保存',
							showCancel: false
						});
					}
				});
				// #endif
			},
		}
	};
</script>

<style scoped>
	.container {
		background-color: #f8fafc;
		min-height: 100vh;
		display: flex;
		flex-direction: column;
	}

	/* 顶部导航 */
	.header-nav {
		background: #fff;
		padding: 40px 15px 15px;
		display: flex;
		align-items: center;
		box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
		position: sticky;
		top: 0;
		z-index: 10;
	}

	.back-btn {
		background: #007aff;
		border: none;
		font-size: 14px;
		padding: 4px 12px;
		border-radius: 4px;
		color: #fff;
	}

	.back-btn:active {
		background: #0056b3;
		transform: scale(0.98);
	}

	.page-title {
		flex: 1;
		text-align: center;
		font-weight: bold;
		color: #1e293b;
		font-size: 17px;
	}

	.scroll-body {
		flex: 1;
		padding: 15px;
	}

	.card {
		background: #fff;
		border-radius: 12px;
		padding: 16px;
		margin-bottom: 16px;
		box-shadow: 0 2px 8px rgba(0, 0, 0, 0.04);
	}

	.card-title {
		font-size: 16px;
		font-weight: bold;
		color: #1e3a8a;
		margin-bottom: 12px;
		display: block;
	}


	/* 容器调整 */
	.filter-row {
		 flex-direction: column;  /* 标签和日期选择器垂直排列 */
		    align-items: flex-start;
	}

	/* 核心：给日历组件设置宽度 */
	.date-picker-ui {
		 width: 100% !important; 
		  flex: 1;
		margin-right: 20px;
	}

	/* 调整标签字体 */
	.filter-row .label {
		font-size: 14px;
		color: #64748b;
		width: 80px;
		flex-shrink: 0;
	}

	/* 让快捷键行往下排或者在后面 */
	.shortcut-row {
		display: flex;
		gap: 12px;
		align-items: center;
	}

	.mini-btn {
		font-size: 12px;
		background: #f1f5f9;
		border: 1px solid #e2e8f0;
		padding: 6px 14px;
		color: #475569;
		transition: all 0.2s ease;
		/* 增加平滑过渡效果 */
	}

	/* 增加选中后的 active 样式 */
	.mini-btn.active {
		background-color: #2563eb;
		color: #ffffff;
		border-color: #1d4ed8;
		font-weight: bold;
		box-shadow: 0 2px 4px rgba(37, 99, 235, 0.2);
	}

	.query-btn {
		background: #2563eb;
		color: #fff;
		border: none;
		font-size: 13px;
		padding: 4px 16px;
		border-radius: 4px;

	}

	.query-btn:active {
		background-color: #0056b3;
		transform: scale(0.98);

	}

	.export-btn {
		background: #10b981;
		color: #fff;
		border: none;
		font-size: 13px;
		padding: 4px 16px;
		border-radius: 4px;
	}

	.export-btn:active {
		background-color: #10a999;
		transform: scale(0.98);
	}

	/* 统计概览区 */
	.stats-grid {
		display: grid;
		grid-template-columns: repeat(3, 1fr);
		gap: 10px;
		margin-bottom: 16px;
	}

	.stats-card {
		padding: 12px;
		border-radius: 10px;
		display: flex;
		flex-direction: column;
	}

	.stats-card.temp {
		background: linear-gradient(135deg, #fee2e2, #fecaca);
	}

	.stats-card.humi {
		background: linear-gradient(135deg, #dbeafe, #bfdbfe);
	}

	.stats-card.light {
		background: linear-gradient(135deg, #fef3c7, #fde68a);
	}

	.st-label {
		font-size: 11px;
		color: #64748b;
		margin-bottom: 4px;
	}

	.st-value {
		font-size: 18px;
		font-weight: bold;
		color: #1e293b;
	}

	.st-unit {
		font-size: 12px;
		font-weight: normal;
		margin-left: 2px;
	}

	/* 图表区 */
	.chart-main-card {
		padding-bottom: 0;
	}

	.card-header {
		display: flex;
		justify-content: space-between;
		align-items: center;
		margin-bottom: 10px;
	}

	.tip {
		font-size: 11px;
		color: #94a3b8;
		font-style: italic;
	}

	.main-chart {
		height: 450px;
		width: 100%;
	}

	/* 数据表格区 */
	.table-card {
		padding: 0;
		overflow: hidden;
	}

	.data-table {
		width: 100%;
	}

	.thead {
		display: flex;
		background: #f8fafc;
		border-bottom: 1px solid #f1f5f9;
	}

	.th {
		flex: 1;
		padding: 12px;
		font-size: 13px;
		font-weight: bold;
		color: #475569;
		text-align: center;
	}

	.tr {
		display: flex;
		border-bottom: 1px solid #f8fafc;
	}

	.td {
		flex: 1;
		padding: 12px;
		font-size: 12px;
		color: #64748b;
		text-align: center;
	}

	.td.time {
		flex: 1.5;
		font-size: 11px;
		color: #94a3b8;
	}

	.light-td {
		color: #b45309;
		font-weight: 500;
	}

	.empty {
		text-align: center;
		padding: 40px;
		color: #94a3b8;
		font-size: 14px;
	}
	
	@media (max-width: 768px) {
	    /* 1. 核心：让页面根容器不再有内边距，实现内容贴边 */
	    .container {
	        padding: 0 !important;
	    }
	
	    /* 2. 顶部导航：像首页一样全屏撑满，去除圆角 */
	    .header-nav {
	        width: 100%;
	        padding: 40rpx 20rpx 15rpx !important; /* 顶部留出状态栏高度 */
	        background: linear-gradient(90deg, #1e3a8a 0%, #3b82f6 100%) !important; /* 强制背景色与首页一致 */
	        border-radius: 0 !important;
	        box-sizing: border-box;
	    }
	    
	    .page-title {
	        color: #ffffff !important; /* 导航栏变蓝，文字变白 */
	        font-size: 32rpx !important;
	    }
	    
	    .back-btn {
	        background: rgba(255, 255, 255, 0.2) !important; /* 按钮半透明，更高级 */
	        color: #ffffff !important;
	        border: none !important;
	        font-size: 24rpx !important;
	        padding: 10rpx 20rpx !important;
	    }
	
	    /* 3. 滚动区域：减小边距，让卡片变宽 */
	    .scroll-body {
	        padding: 20rpx 15rpx !important; /* 左右各留 15rpx，与首页一致 */
	    }
	
	    /* 4. 卡片统一处理：100% 宽度，贴合边缘 */
	    .card {
	        width: 100% !important;
	        padding: 20rpx !important;
	        margin-bottom: 20rpx !important;
	        border-radius: 12rpx !important;
	        box-sizing: border-box;
	    }
	
	    /* 5. 筛选区域：解决组件溢出 */
	    .filter-row {
	        flex-direction: column !important;
	        align-items: flex-start !important;
	    }
	    .date-picker-ui {
	        width: 100% !important; /* 强制日期选择器撑满卡片 */
	        margin-top: 10rpx;
	    }
	    .shortcut-row {
	        gap: 10rpx !important;
	        margin-top: 20rpx;
	    }
	    .mini-btn {
	        padding: 12rpx 15rpx !important;
	        font-size: 20rpx !important;
	    }
	
	    /* 6. 统计概览：1列堆叠 */
	    .stats-grid {
	        grid-template-columns: 1fr !important;
	        gap: 15rpx !important;
	    }
	    .stats-card {
	        flex-direction: row !important;
	        justify-content: space-between;
	        align-items: center;
	        padding: 20rpx 30rpx !important;
	    }
	
	    /* 7. 表格深度对齐（保证 LUX 和时间不被切断） */
	    .table-card {
	        padding: 0 !important; /* 表格卡片内边距清零，让表格更宽 */
	        overflow-x: auto !important;
	    }
	    .data-table {
	        min-width: 100% !important;
	    }
	    /* 精确分配百分比：时间40%, 温度18%, 湿度18%, 光强24% */
	    .th:nth-child(1), .td:nth-child(1) { width: 28% !important; flex: none; font-size: 19rpx !important; }
	    .th:nth-child(2), .td:nth-child(2) { width: 10% !important; flex: none; }
	    .th:nth-child(3), .td:nth-child(3) { width: 18% !important; flex: none; }
	    .th:nth-child(4), .td:nth-child(4) { width: 10% !important; flex: none; text-align: right; }
	}
</style>