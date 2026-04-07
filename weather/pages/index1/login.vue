<template>
	<view class="login-page">
		<view class="login-card">
			<view class="login-header">
				<text class="system-name">智慧校园气象监测系统</text>
				<text class="sub-title">请验证身份以开启监测服务</text>
			</view>

			<view class="form-container">
				<!-- 统一风格：账号输入盒子 -->
				<view class="item-box">
					<view class="icon-wrap">👤</view>
					<input v-model="username" placeholder="请输入登录账号" placeholder-class="p-style" />
				</view>

				<!-- 统一风格：密码输入盒子 -->
				<view class="item-box">
					<view class="icon-wrap">🔒</view>
					<input v-model="password" type="password" placeholder="请输入登录密码" placeholder-class="p-style" />
				</view>

				<!-- 统一风格：身份选择盒子 -->
				<view class="role-selector-wrap">
					<view class="item-box selector-trigger" @click.stop="toggleMenu">
						<view class="trigger-content">
							<view class="icon-wrap">{{ selectedRole.icon || '❓' }}</view>
							<text :class="['role-text', !selectedRole.value ? 'placeholder' : '']">
								{{ selectedRole.label || '请选择登录身份' }}
							</text>
						</view>
						<text class="arrow" :class="{ 'arrow-active': showMenu }">▼</text>
					</view>

					<!-- 下拉菜单：对齐触发盒子 -->
					<view class="dropdown-menu" v-if="showMenu">
						<view class="menu-item" v-for="(item, index) in roles" :key="index" @click="selectRole(item)">
							<text class="m-icon">{{ item.icon }}</text>
							<view class="m-body">
								<text class="m-label">{{ item.label }}</text>
								<text class="m-desc">{{ item.desc }}</text>
							</view>
							<text class="m-check" v-if="selectedRole.value === item.value">✓</text>
						</view>
					</view>
				</view>

				<button class="login-btn" @click="handleLogin">立 即 登 录</button>
			</view>

			<view class="copyright">Power by 校园环境监测实验室</view>
		</view>
		//
	</view>
</template>

<script>
	export default {
		data() {
			return {
				username: '',
				password: '',
				showMenu: false,
				roles: [{
						label: '管理员',
						value: 'admin',
						icon: '🛠️',
					},
					{
						label: '普通用户',
						value: 'user',
						icon: '👤',
					}
				],
				selectedRole: {
					label: '',
					value: '',
					icon: ''
				}
			}
		},
		methods: {
			toggleMenu() {
				this.showMenu = !this.showMenu;
			},
			selectRole(item) {
				this.selectedRole = item;
				this.showMenu = false;
				this.username = item.value;
				this.password = '123456';
			},
			handleLogin() {
				// 1. 先检查是否选了身份
				if (!this.selectedRole.value) {
					return uni.showToast({
						title: '请选择登录身份',
						icon: 'none'
					});
				}
				// 2. 检查账号密码是否为空
				if (!this.username || !this.password) {
					return uni.showToast({
						title: '账号或密码不能为空',
						icon: 'none'
					});
				}

				// 3. 验证身份匹配逻辑 (例如管理员账号必须是 admin)
				if (this.username === this.selectedRole.value && this.password === '123456') {
					uni.setStorageSync('userInfo', {
						role: this.selectedRole.value,
						name: this.username
					});
					uni.reLaunch({
						url: '/pages/index1/index'
					});
				} else {
					uni.showToast({
						title: '账号、密码或身份不匹配',
						icon: 'none'
					});
				}
			}
		}
	}
</script>

<style lang="scss" scoped>
	.login-page {
		height: 100vh;
		background: #2b5bc7; // 使用纯净深蓝色
		display: flex;
		justify-content: center;
		align-items: center;
		
	}

	.login-card {
		width: 90%;  
		 max-width: 650rpx;
		background: #ffffff;
		border-radius: 30rpx;
		padding: 60rpx 40rpx;
		box-shadow: 0 20rpx 50rpx rgba(0, 0, 0, 0.15);
		
	}

	.login-header {
		text-align: center;
		margin-bottom: 70rpx;

		.system-name {
			display: block;
			font-size: 42rpx;
			font-weight: bold;
			color: #1a2a52;
			letter-spacing: 2rpx;
		}

		.sub-title {
			display: block;
			font-size: 24rpx;
			color: #a0aec0;
			margin-top: 15rpx;
		}
	}

	/* 统一盒子样式：这是实现“整齐”的关键 */
	.item-box {
		height: 100rpx;
		background: #f7fafc;
		border: 2rpx solid #edf2f7;
		border-radius: 16rpx;
		display: flex;
		align-items: center;
		padding: 0 30rpx;
		margin-bottom: 30rpx;
		transition: all 0.2s;

		&:active {
			border-color: #cbd5e0;
		}

		.icon-wrap {
			width: 60rpx;
			font-size: 36rpx;
			display: flex;
			align-items: center;
			color: #4a5568;
		}

		input {
			flex: 1;
			font-size: 28rpx;
			color: #2d3748;
			height: 100%;
		}

		.p-style {
			color: #a0aec0;
		}
	}

	/* 身份选择器的特殊处理 */
	.role-selector-wrap {
		position: relative;
		margin-bottom: 70rpx;

		.selector-trigger {
			margin-bottom: 0; // 覆盖掉底边距
			justify-content: space-between;

			.trigger-content {
				display: flex;
				align-items: center;
				flex: 1;
			}

			.role-text {
				font-size: 28rpx;
				color: #2d3748;
			}

			.role-text.placeholder {
				color: #a0aec0;
			}

			.arrow {
				font-size: 20rpx;
				color: #a0aec0;
				transition: transform 0.3s;
			}

			.arrow-active {
				transform: rotate(180deg);
			}
		}
	}

	/* 下拉菜单：悬浮效果 */
	.dropdown-menu {
		position: absolute;
		top: 115rpx;
		left: 0;
		width: 100%;
		background: #ffffff;
		border-radius: 16rpx;
		z-index: 99;
		box-shadow: 0 10rpx 30rpx rgba(0, 0, 0, 0.1);
		border: 1rpx solid #edf2f7;
		padding: 10rpx 0;

		.menu-item {
			display: flex;
			align-items: center;
			padding: 25rpx 35rpx;

			&:active {
				background: #f7fafc;
			}

			.m-icon {
				font-size: 40rpx;
				margin-right: 25rpx;
			}

			.m-body {
				flex: 1;

				.m-label {
					display: block;
					font-size: 28rpx;
					color: #2d3748;
					font-weight: bold;
				}

				.m-desc {
					display: block;
					font-size: 22rpx;
					color: #718096;
					margin-top: 4rpx;
				}
			}

			.m-check {
				color: #3182ce;
				font-weight: bold;
			}
		}
	}

	.login-btn {
		width: 100%;
		height: 100rpx;
		line-height: 100rpx;
		background: #3182ce;
		color: #ffffff;
		border-radius: 16rpx;
		font-size: 32rpx;
		font-weight: bold;
		border: none;
		box-shadow: 0 10rpx 20rpx rgba(49, 130, 206, 0.3);

		&:active {
			opacity: 0.9;
			transform: scale(0.99);
		}
	}

	.copyright {
		text-align: center;
		font-size: 22rpx;
		color: #cbd5e0;
		margin-top: 70rpx;
	}
	
	@media (max-width: 350px) {
	    .system-name { font-size: 36rpx !important; }
	    .item-box { height: 90rpx !important; }
	}
</style>