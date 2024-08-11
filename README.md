# whisen-iot

LG Whisen 에어컨 IR원격 IOT

## 구성요소

- ESP8266 NodeMCU V1.0
- IR LED
- TMP36

![sketch](https://github.com/user-attachments/assets/413f3dbc-7688-4cc2-a401-7969003ea90a)


## 구현기능

- 에어컨 전원 On/Off
- 운전모드 (냉방, 파워냉방, 난방, 제습, 바람)
- 온도조절 및 현재온도 표시
- 바람세기
- 꺼짐예약
- 설정저장

## 사용법

1. `src/config.h` 파일을 열어서 `WIFI_SSID`, `WIFI_PASSWORD`, `HTTP_USERNAME`, `HTTP_PASSWORD`를 설정합니다.
2. ESP8266에 코드를 업로드한 후, `LED_BUILTIN` LED가 꺼질 때까지 기다립니다.
3. 만약 `LED_BUILTIN` LED가 꺼지지 않거나 깜빡인다면, 시리얼 모니터를 열어서 에러 코드를 확인합니다.
4. `http://<ESP8266_IP>/status`로 접속하여 API가 정상적으로 동작하는지 확인합니다.

### 아이폰 단축어

아이폰의 단축어 앱을 이용하여 손쉽게 에어컨을 제어할 수 있습니다.

<img src="https://github.com/user-attachments/assets/5e61d074-16a4-4014-9fd4-7fce30695df7" align="center" width="32%">
<img src="https://github.com/user-attachments/assets/f0cea4e1-6e73-4ec1-9929-8b0790c9cbae" align="center" width="32%">
<img src="https://github.com/user-attachments/assets/76db7d50-0e97-4c0d-a2ce-7dda0e3acc5f" align="center" width="32%">

1. [Whisen 제어](https://www.icloud.com/shortcuts/0e0be05ac42d4c2d922b8386ec511a6c) 단축어를 추가합니다.
2. 단축어 수정에서 첫 번째 텍스트를 자신의 ESP8266 IP로 수정합니다.
   <img src="https://github.com/user-attachments/assets/537c1d1d-8646-47dc-a3bf-edab5ce971fa" align="center" width="32%">
3. 본인이 설정한 `HTTP_USERNAME`과 `HTTP_PASSWORD`를 `{HTTP_USERNAME}:{HTTP_PASSWORD}` 형식으로 두 번째 텍스트에 입력합니다.
   <img src="https://github.com/user-attachments/assets/b59f86f8-e595-4cec-b094-10cfbba2589e" align="center" width="32%">
4. 단축어를 실행하여 정상적으로 API가 호출되는지 확인합니다.

### API

#### GET /status

에어컨의 현재 상태를 조회합니다.

##### Response

```json
{
    "state": {
        "power": true,          // boolean: 전원 상태
        "mode": "Cool",         // string:  운전모드 (Cool, Dry, Fan, Heat, Jet)
        "temperature": 24,      // number:  설정 온도 (15 ~ 30)
        "fan": "Fan0"           // string:  바람세기 (Fan0, Fan1, Fan2, Fan3, Fan4, NaturalWind)
    },
    "temperature": 25.17382813, // number: 현재 온도
    "uptime": 3729,             // number: 동작 시간 (초)
    "status": "ok"
}
```

#### PATCH /status

에어컨의 상태를 변경합니다.

##### Body

Content Type: application/json

```json
{
    "power": true,      // boolean?: 전원 상태
    "mode": "Cool",     // string?:  운전모드 (Cool, Dry, Fan, Heat, Jet)
    "temperature": 24,  // number?:  설정 온도 (15 ~ 30)
    "fan": "Fan0",      // string?:  바람세기 (Fan0, Fan1, Fan2, Fan3, Fan4, NaturalWind)
    "timer": 0,         // number?:  꺼짐 예약 (분)
    "save": true        // boolean?: 설정 저장
}
```

## License

MIT