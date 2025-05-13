#include <GSON.h>
#include <GyverHTTP.h>

String HTTP = "https://api.open-meteo.com/v1/forecast?latitude=50.5206&longitude=26.2425&hourly=apparent_temperature,weather_code,wind_speed_10m&wind_speed_unit=ms&timezone=Europe%2FMoscow&forecast_days=1";

float temps[24];
float winds[24];
uint8_t codes[24];
float temp_min[4];
float temp_max[4];
float wind_mid[4];
uint8_t code_max[4];
uint8_t weth_code;
uint8_t w_code[3];
String wether_name[3];

String weather(String latitude = "50.5206", String longitude = "26.2425", String mess = ""){
    String HTTP = "https://api.open-meteo.com/v1/forecast?latitude=" + latitude + "&longitude=" + longitude + "&hourly=apparent_temperature,weather_code,wind_speed_10m&wind_speed_unit=ms&timezone=Europe%2FMoscow&forecast_days=1";
    Serial.println(HTTP);
    ghttp::EspInsecureClient http("api.open-meteo.com", 443);
    if (http.request(HTTP)) {
      ghttp::Client::Response resp = http.getResponse();
      if (resp) {
        String s = resp.body().readString();
        gson::Parser json;
        if (json.parse(s)) {
          json["hourly"]["apparent_temperature"].parseTo(temps);
          json["hourly"]["wind_speed_10m"].parseTo(winds);
          json["hourly"]["weather_code"].parseTo(codes);
          json["hourly"].stringify(Serial);
        } else {
          Serial.println("parse error");
        }
      } else {
          Serial.println("response error");
      }
    }
    temp_min[0] = (temps[6] += temps[7] += temps[8] += temps[9] += temps[10] += temps[11]) / 6;
  temp_min[1] = (temps[12] += temps[13] += temps[14] += temps[15] += temps[16] += temps[17]) / 6;
  temp_min[2] = (temps[18] += temps[19] += temps[20] += temps[21] += temps[22] += temps[23]) / 6;

  wind_mid[0] = (winds[6] + winds[7] + winds[8] + winds[9] + winds[10] + winds[11]) / 6;
  wind_mid[1] = (winds[12] + winds[13] + winds[14] + winds[15] + winds[16] + winds[17]) / 6;
  wind_mid[2] = (winds[18] + winds[19] + winds[20] + winds[21] + winds[22] + winds[23]) / 6;

  weth_code = winds[6];
  for(int i = 6; i < 12; i++){
    weth_code = max(weth_code, codes[i]);
  }
  code_max[0] = weth_code;

  weth_code = winds[12];
  for(int i = 12; i < 17; i++){
    weth_code = max(weth_code, codes[i]);
  }
  code_max[1] = weth_code;

  weth_code = winds[18];
  for(int i = 18; i < 23; i++){
    weth_code = max(weth_code, codes[i]);
  }
  code_max[2] = weth_code;
  for(int i = 0; i < 3; i++){
        switch(code_max[i]){
            case 0: wether_name[i] = "чисте небо";
            break;
            case 1: wether_name[i] = "переважно хмарно";
            break;
            case 2: wether_name[i] = "мінлива хмарність";
            break;
            case 3: wether_name[i] = "похмуро";
            break;
            case 45: wether_name[i] = "туман";
            break;
            case 48: wether_name[i] = "відкладання інею";
            break;
            case 51: wether_name[i] = "слабка мряка";
            break;
            case 53: wether_name[i] = "помірна мряка";
            break;
            case 55: wether_name[i] = "сильна мряка";
            break;
            case 56: wether_name[i] = "легкий крижаний дощ";
            break;
            case 57: wether_name[i] = "щільний крижаний дощ";
            break;
            case 61: wether_name[i] = "слабкий дощ";
            break;
            case 63: wether_name[i] = "помірний дощ";
            break;
            case 65: wether_name[i] = "сильний дощ";
            break;
            case 66: wether_name[i] = "слабкий крижаний дощ";
            break;
            case 67: wether_name[i] = "сильний крижаний дощ";
            break;
            case 71: wether_name[i] = "слабкий снігопад";
            break;
            case 73: wether_name[i] = "помірний снігопад";
            break;
            case 75: wether_name[i] = "сильний снігопад";
            break;
            case 77: wether_name[i] = "сніжна крупа";
            break;
            case 80: wether_name[i] = "невеликий дощ";
            break;
            case 81: wether_name[i] = "помірний дощ";
            break;
            case 82: wether_name[i] = "сильний дощ";
            break;
            case 85: wether_name[i] = "невеликий снігопад";
            break;
            case 86: wether_name[i] = "сильний снігопад";
            break;
            case 95: wether_name[i] = "гроза";
            break;
            case 96: wether_name[i] = "гроза з невеликим градом";
            break;
            case 99: wether_name[i] = "гроза з сильнийм градом";
            break;
        }
    }

//   Serial.print(temp_min[0]);
//   Serial.print('\t');
//   Serial.print(temp_min[1]);
//   Serial.print('\t');
//   Serial.print(temp_min[2]);
//   Serial.println('\n');
//   Serial.print(wind_mid[0]);
//   Serial.print('\t');
//   Serial.print(wind_mid[1]);
//   Serial.print('\t');
//   Serial.print(wind_mid[2]);
//   Serial.println('\n');
//   Serial.print(wether_name[0]);
//   Serial.print('\t');
//   Serial.print(wether_name[1]);
//   Serial.print('\t');
//   Serial.print(wether_name[2]);
  mess = mess + "Прогноз погоди для Вашої локації на сьогодні:\n\tТемпература зранку становитиме " + temp_min[0] + ", вдень: " + temp_min[1] + ", а ввечері: " + temp_min[2];
  mess = mess + "\n\tВітер з ранку буде " + wind_mid[0] + " м/с, вдень: " + wind_mid[1] + " м/с, і ввечері" + wind_mid[2] + " м/с";
  mess = mess + "\n\tОпади, хмари та інші погодні явища: \nзранку: " + wether_name[0] + " вдень: " + wether_name[1] + ", а ввечері: " + wether_name[2];
  return mess;
}
