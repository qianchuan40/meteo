
export const getWeatherData = (lat, lng) => {
	return new Promise((resolve, reject) => {
		uni.request({
			url: 'https://api.open-meteo.com/v1/forecast',
			// 这里完全对应你截图中的 params
			data: {
				latitude: lat,      // 28.23
				longitude: lng,     // 112.93
				daily: 'sunrise,sunset', // 对应 daily: ["sunrise", "sunset"]
				timezone: 'auto'    // 对应 timezone: "auto"
			},
			method: 'GET',
			success: (res) => {
				// Open-Meteo 的 JSON 格式非常直观
				if (res.statusCode === 200) {
					resolve(res.data);
				} else {
					reject(res);
				}
			},
			fail: (err) => reject(err)
		});
	});
};