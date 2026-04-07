from flask import Flask, request, jsonify
from flask_cors import CORS
import mysql.connector

app = Flask(__name__)
CORS(app)  # 允许跨域，方便 Vue 访问

# 数据库连接配置
db_config = {
    "host": "localhost",
    "user": "root",
    "password": "zzzzzz",
    "database": "peipei"
}

@app.route('/api/sensors', methods=['GET'])
def get_sensor_data():
    device_name = request.args.get('name')
    start_time = request.args.get('startTime')  # 格式: 2026-03-18 12:00:00
    end_time = request.args.get('endTime')  # 格式: 2026-03-18 18:00:00
    limit = request.args.get('limit', 5000)

    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor(dictionary=True)

        # 基础 SQL
        query = "SELECT * FROM sensordata WHERE 1=1"
        params = []

        # 按设备名过滤
        if device_name:
            query += " AND name = %s"
            params.append(device_name)

        # 按时间范围过滤 (使用 timestr 字段)
        if start_time and end_time:
            query += " AND timestr BETWEEN %s AND %s"
            params.append(start_time)
            params.append(end_time)

        query += " ORDER BY recvtime DESC LIMIT %s"
        params.append(int(limit))

        cursor.execute(query, params)
        result = cursor.fetchall()

        cursor.close()
        conn.close()
        return jsonify(result)
    except Exception as e:
        print(f"数据库错误:{e}")
        return jsonify({"code": 500, "msg": f"<UNK>:{e}"})
    finally:
        if 'conn' in locals()and conn.is_connected():
            cursor.close()
            conn.close()


if __name__ == '__main__':
    app.run(host='0.0.0.0',port=5000, debug=True)