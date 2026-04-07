<script>
	export default {
		onLaunch: function() {
			console.log('App Launch')
			  // 模拟路由守卫
			    const whiteList = ['pages/index1/login']; // 白名单列表
			    
			    // 拦截所有跳转API
			    const navList = ['navigateTo', 'redirectTo', 'reLaunch', 'switchTab'];
			    navList.forEach(item => {
			        uni.addInterceptor(item, {
			            invoke(e) {
			                const url = e.url.split('?')[0].replace(/^\//, '');
			                const userInfo = uni.getStorageSync('userInfo');
			                if (!userInfo && !whiteList.includes(url)) {
			                    uni.navigateTo({ url: '/pages/index1/login' });
			                    return false;
			                }
			                return true;
			            }
			        });
			    });
		},
		onShow: function() {
			console.log('App Show')
		},
		onHide: function() {
			console.log('App Hide')
		}
	}
</script>

<style>
	/*每个页面公共css */
</style>