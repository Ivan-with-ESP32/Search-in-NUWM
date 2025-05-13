#include <Arduino.h>
#include <WiFi.h>
#include "SPIFFS.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <GSON.h>
#include "weather.h"
#include <ArduinoJson.h>

#define WIFI_SSID "TP-Link_BF4A"
#define WIFI_PASS "25659085"
#define BOT_TOKEN "8082471321:AAFUFDJQ5HJAzUyqT9bLdZucVIATpHAChdY"
#define ADMIN_ID "6231436511"

#include <FastBot2.h>
FastBot2 bot;

const char *geminiApiKey = "AIzaSyC_J9gzHlAI_5cvppZRq0z19hdF-79_vjA";
const char* geminiApiUrl = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-pro-exp-03-25:generateContent";
const char *ai_model = "gemini-2.0-flash-lite";

//==========VALUES=================================================
bool question_bool[20];
String specialty = "";
String file_name = "";
String admin_chat_id = "";
String time_id = "";
String file_type = "";
String file_year = "";
String google_pdf[10];
String latitud = "";
unsigned long timer = 0;

const char *raspberryPiIP = "192.168.1.101";
const int raspberryPiPort = 8040;

String askGemini(String prompt) {
  Serial.println(prompt);
  HTTPClient http;
  String url = String(geminiApiUrl) + "?key=" + geminiApiKey;
  
  Serial.println("[HTTP] Починаємо запит до: " + url);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  // Створюємо JSON запит
  DynamicJsonDocument doc(4096);
  JsonArray contents = doc.createNestedArray("contents");
  JsonObject content = contents.createNestedObject();
  JsonArray parts = content.createNestedArray("parts");
  JsonObject textPart = parts.createNestedObject();
  textPart["text"] = prompt;

  String requestBody;
  serializeJson(doc, requestBody);
  Serial.println("[HTTP] Тіло запиту: " + requestBody);

  // Надсилаємо запит
  int httpResponseCode = http.POST(requestBody);
  Serial.println("[HTTP] Код відповіді: " + String(httpResponseCode));

  if (httpResponseCode == HTTP_CODE_OK) {
    String response = http.getString();
    Serial.println("[HTTP] Відповідь: " + response);
    
    DynamicJsonDocument responseDoc(8192);
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (error) {
      Serial.println("[ERROR] Помилка парсингу JSON: " + String(error.c_str()));
      return "Помилка обробки відповіді";
    }
    
    if (responseDoc.containsKey("candidates")) {
      String geminiResponse = responseDoc["candidates"][0]["content"]["parts"][0]["text"];
      return geminiResponse;
    } else {
      Serial.println("[ERROR] Неочікувана структура відповіді");
      return response; // Повертаємо всю відповідь для аналізу
    }
  } else {
    String errorResponse = http.getString();
    Serial.println("[HTTP] Помилка: " + errorResponse);
    return "HTTP помилка: " + String(httpResponseCode) + " - " + errorResponse;
  }
  
  http.end();
}

void sendMess(String id)
{
  bot.sendMessage(fb::Message("Будь ласка, ведіть рік (цифрами), якщо такого року не буде знайдено, я повідомлю.", id));
  question_bool[4] = false;
  question_bool[5] = true;
}

void AI_Helper(String text_for_ai, String chat_id)
{
  Serial.println(text_for_ai.length());
  // String response = sendToGemini(text_for_ai);
  // bot.sendMessage(fb::Message(response, chat_id));
  if (text_for_ai.indexOf("/ai_helper") != -1)
  {
    text_for_ai.replace("/ai_helper", "");
    text_for_ai = "$new_chat$" + text_for_ai;
  }
  String message_for_ai = "#AI_HELP#" + text_for_ai;
  WiFiClient client;
  if (client.connect(raspberryPiIP, raspberryPiPort))
  {
    client.print(message_for_ai);
    String response = "";
    while (client.connected() || client.available())
    {
      if (client.available())
      {
        char c = client.read();
        response += c;
      }
    }
    Serial.println(response.length());
    bot.sendMessage(fb::Message(response, chat_id));
  }
}

void Google_reqest(String tema, String id)
{
  tema.trim();
  while (tema.indexOf("  ") != -1)
  {
    tema.replace("  ", " ");
  }
  while (tema.indexOf(" ") != -1)
  {
    tema.replace(" ", "+");
  }
  String url_google = "https://scholar.google.com.ua/scholar?as_q=&as_epq=\"" + tema + "\"&as_oq=&as_eq=&as_occt=any&as_sauthors=&as_publication=&as_ylo=2019&as_yhi=2025";
  String server_message = "#GOOGLE#" + url_google;
  Serial.println(server_message);
  WiFiClient client;
  if (client.connect(raspberryPiIP, raspberryPiPort))
  {
    client.print(server_message);
    String response = "";
    while (client.connected() || client.available())
    {
      if (client.available())
      {
        char c = client.read();
        response += c;
      }
    }
    Serial.println(response);
    int num_for_links = response.substring(response.indexOf("$") + 1).toInt();
    if (response == " ")
    {
      bot.sendMessage(fb::Message("На Ваш запит не знайдено відповіді", id));
    }
    else
    {
      for (int i = 0; i < num_for_links; i++)
      {
        google_pdf[i] = response.substring((response.indexOf("<") + 1), response.indexOf(">"));
        response.remove((response.indexOf("<") + 1), response.indexOf(">") + 2);
        Serial.println(google_pdf[i]);
        fb::File f("file.txt", fb::File::Type::document, google_pdf[i]);
        f.chatID = id;
        bot.sendFile(f);
      }
    }
  }
}

void NUWM_file(String site_file)
{
  HTTPClient http;
  http.begin(site_file);
  Serial.println(http.GET());
  String payload = http.getString();
  int download_url = payload.indexOf("Download(");
  payload.remove(download_url);
  payload.remove(0, (payload.lastIndexOf("href=") + 6));
  payload.remove((payload.indexOf("class=") - 2));
  payload.replace("http://", "https://");
  Serial.println(payload);
  http.end();
  fb::File f("file.txt", fb::File::Type::document, payload);
  f.chatID = time_id;
  bot.sendFile(f);
};

void server_NUWM(String type, String tema, String year)
{
  String url_to_file = "https://ep3.nuwm.edu.ua/view/types/" + type + year + ".html";
  String site_message = "#NUWM#" + tema + "<" + url_to_file + ">";
  WiFiClient client;
  if (client.connect(raspberryPiIP, raspberryPiPort))
  {
    client.print(site_message);
    String response = "";
    while (client.connected() || client.available())
    {
      if (client.available())
      {
        char c = client.read();
        response += c;
      }
    }
    Serial.println(response);
    if (response == "")
    {
      bot.sendMessage(fb::Message("Файл за Вашим запитом не знайдено", time_id));
    }
    else
    {
      NUWM_file(response);
    }
  }
  client.stop();
};

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Запис у файл: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Помилка відкриття файлу для запису");
    return;
  }
  if (file.print(message))
  {
    Serial.println("Запис успішний");
  }
  else
  {
    Serial.println("Помилка запису");
  }
  file.close();
}

void readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Читання файлу: %s\n", path);

  File file = fs.open(path, FILE_READ);
  fb::File f("user.txt", fb::File::Type::document, file);
  f.chatID = ADMIN_ID;
  bot.sendFile(f, false);
  file.close();
}

//==========WIFI===================================================
void connectWiFi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Підключення до Wi-Fi...");
  }
  Serial.println(WiFi.localIP());
}

//==========BOT====================================================
void rawh(Text text);
void handleCommand(fb::Update &u);
void handleMessage(fb::Update &u);
void handleDocument(fb::Update &u);
void handleQuery(fb::Update &u);
void updateh(fb::Update &u);

//==========COMMAND=================================================
void handleCommand(fb::Update &u)
{
  Text chat_id = u.message().chat().id();
  String name = u.message().chat().firstName().decodeUnicode();
  String last_name = u.message().chat().lastName().decodeUnicode();

  switch (u.message().text().hash())
  {
  case SH("/start"):
  {
    bot.sendMessage(fb::Message("Вас вітає розробка студента НУВГП - телеграм бот, який має унікальні функції пошуку та обробки інформації. Ми (я, розробник, та бот) гарантуємо, що всі чати та дані, які Ви залишите тут, будуть конфіденційні та не вийдуть за межі пам'яті мікроконтролера. \n\n Дякуємо, що обрали нас!", chat_id));
    bot.sendMessage(fb::Message("Список команд можна подивитися в меню. Деякі команди можуть бути недоступними в певний час, або ж Ви не маєте достатнього рівня доступу. Для того, щоб побачити список всіх команд з розширеним описом скористайтеся командою /help. \n\n Ви можете зв'язатися з адміністрацією, для цього є команда /chat, де у відкритому меню обираєте \"Адміністрація\", якщо Ви не є адміном, то інші чати будуть Вам не доступні.", chat_id));
    bot.sendMessage(fb::Message("УВАГА! Бот працює з 8:30 до 22:00\nякщо Вам потрібно пордовжити час користування -> чат з адміністрацією", chat_id));
    bot.sendMessage(fb::Message("Надалі весь чат буде вести сам бот", chat_id));
    fb::Message msg("Наразі я хочу з Вами познайомитися. \n\nСпочатку я хочу дізнатися, на якій спеціальності Ви навчаєтесь. Відповідайте правдиво, тому що ця інформація буде збережена в файлі, і змінити її можна буде лише за допомогою адміністрації.", chat_id);
    bot.sendMessage(msg);
    question_bool[0] = true;
    bot.sendMessage(fb::Message(u.message().chat().firstName().decodeUnicode(), ADMIN_ID));
    bot.sendMessage(fb::Message(u.message().chat().lastName().decodeUnicode(), ADMIN_ID));
    bot.sendMessage(fb::Message(u.message().chat().id().decodeUnicode(), ADMIN_ID));
    bot.sendMessage(fb::Message(u.message().chat().username().decodeUnicode(), ADMIN_ID));
  }
  break;
  case SH("/chat"):
  {
    fb::Message msg("Оберіть чат, в який Ви хочете (або можете) перейти. \n Для завершення чату використовуйте /chat_end \n Хочете з кимось поспілкуватися, маєте пропозицію, Вам в чат з адміністрацією!", chat_id);
    fb::InlineMenu menu;
    menu.addButton("Локальний", "local");
    menu.newRow();
    menu.addButton("Глобальний", "global");
    menu.newRow();
    menu.addButton("Адміністрація", "admin");
    msg.setInlineMenu(menu);
    bot.sendMessage(msg);
    question_bool[2] = true;
  }
  break;
  case SH("/chat_end"):
  {
    question_bool[3] = false;
    admin_chat_id = "";
  }
  break;
  case SH("/server_NUWM"):
  {
    time_id = chat_id.decodeUnicode();
    Serial.println(time_id);
    bot.sendMessage(fb::Message("Ооо, Ви вирішили скористатися нашою новітньою і унікальною розробкою! \n Ви можете отримати посилання, файл в форматі PDF, файл з текстовим вмістом PDF файлу з цифрового репозиторію НУВГП. Для отримання відповідей, Вам потрібно лише обрати тип, рік, та вказати тему (або ключові слова), і я підберу декілька (поки що один ☺) файл, надішлю Вам опис, і якщо він задовольнить Ваші потреби надішлю його у форматі txt (на жаль, саме в цьому форматі, адже наша платформа ще не вміє напряму працювати з файлами інших типів)", chat_id));
    fb::Message msg("Оберіть, в якому розділі мені шукати файл", chat_id);
    fb::InlineMenu menu;
    menu.addButton("Методичне забезпечення", "type_1");
    menu.newRow();
    menu.addButton("Книги", "type_2");
    menu.addButton("Монографії", "type_3");
    menu.newRow();
    menu.addButton("Автореферати, дисертації", "type_4");
    menu.newRow();
    menu.addButton("Статі", "type_5");
    menu.newRow();
    menu.addButton("Нормативні документи", "type_6");
    menu.newRow();
    menu.addButton("Освітні програми", "type_7");
    menu.newRow();
    menu.addButton("Газети", "type_8");
    menu.addButton("Журнали", "type_9");
    menu.newRow();
    menu.addButton("Законодавчі та нормативно правові документи", "type_10");
    menu.newRow();
    menu.addButton("Робочі програми", "type_11");
    menu.addButton("Силабус", "type_12");
    menu.newRow();
    menu.addButton("Опис дисципліни", "type_13");
    msg.setInlineMenu(menu);

    bot.sendMessage(msg);
    question_bool[4] = true;
  }
  break;
  case SH("/google_server"):
  {
    bot.sendMessage(fb::Message("Так, ми маємо вихід в мережу Google, та можемо надсилати, Вам файли з цифрової бібліотеки. \n\n Для запиту, просто введіть тему (раджу використовувати коротку, так я швидше оброблю даний запит, і якщо виникне проблема на сервері, швидше зреагую. і повідомлю про це в чаті)", chat_id));
    question_bool[11] = true;
  }
  break;
  case SH("/help"):
  {
    bot.sendMessage(fb::Message("Ооо, Вам потрібна допомога? \n Я допоможу Вам всім чим зможу! Якщо я не можу надіслати повідомлення - зверніться до адміністрації (там Вам допоможуть. а може і ні ☻) \n\nвсі потрібні команди: \n/chat - всі чати \n/server_NUWM - тут Ви можете отримати файл з репозиторію НУВГП \n/google_server - отримайте файли з глобальної мережі всього за одним запитом!\n/weather - отримайте прогноз погоди на сьогодні для Вашої локації.\nпокищо все, але в майбутньому, ми обов'язково додамо нові функції!", chat_id));
    bot.sendMessage(fb::Message("Важливе зауваження! Штучний інтелект, що викликається командою /ai_helper - це не ChatGPT, а модель, назву якої я не назву, яка може генерувати ще кращі відповіді на Ваші запити!", chat_id));
    bot.sendMessage(fb::Message("Якщо Ви маєте питання, пропозиції, претензії(?), Вам хочеться з кимось поспілкуватися - Вам в чат з адміністрацією (/chat)", chat_id));
  }
  break;
  case SH("/ai_helper"):
  {
    bot.sendMessage(fb::Message("Ви розпочали чат з штучним інтелектом. наступне Ваше повідомлення буде надіслане AI. Щоб завершити чат - /ai_end", chat_id));
    question_bool[12] = true;
  }
  break;
  case SH("/ai_end"):
  {
    bot.sendMessage(fb::Message("Чат зі штучним інтелектом завершено. Якщо знадобиться знову - /ai_helper", chat_id));
    question_bool[12] = false;
  }
  break;
  case SH("/weather"):
  {
    question_bool[9] = true;
  }
  break;
  }
}

//==========MESSAGE=================================================
void handleMessage(fb::Update &u)
{
  if (question_bool[0] == true)
  {
    specialty = "";
    specialty = u.message().text().decodeUnicode();
    bot.sendMessage(fb::Message("якщо маєте бажання, можете написати свою мрію. Можливо я зможу втілити її в життя", u.message().chat().id()));
    question_bool[0] = false;
    question_bool[1] = true;
  }
  else if (question_bool[1] == true)
  {
    String file_conteiner = "Спеціальність: " + specialty + "; \nМрія: " + u.message().text().decodeUnicode();
    file_name = "";
    file_name = file_name + "/" + u.message().chat().id().decodeUnicode() + ".txt";
    writeFile(SPIFFS, file_name.c_str(), file_conteiner.c_str());
    readFile(SPIFFS, file_name.c_str());
    if (u.message().chat().id() == "1801244168")
    {
      bot.sendMessage(fb::Message("Ну нарешті, Ви Анно, приєдналася до бота (тобто чату зі мною), який зможе допомогти тобі в навчанні! Мої можливості необмежені! (образно, на жаль. Обмеження торкаються всього: від швидкодії - до неможливості обробляти великі тексти, і навіть надсилати їх). Але! незважаючи на всі ці труднощі, команда розробників (одна людина, ChatGPT і Shift) буде й надалі розробляти та вдосконалювати всі функції, які вже є, і, звісно, додавати нові!", u.message().chat().id()));
      bot.sendMessage(fb::Message("Якщо Вам цікаво, то це версія 6.0, і Ви перші, хто нею скористається (якщо бути чесним, то не лише цією. Ви перші, поза командою, хто отримає можливість протестувати та користуватися нашими напрацюваннями!)", u.message().chat().id()));
    }
    question_bool[1] = false;
  }
  if (u.isMessage())
  {
    if (u.message().text().startsWith('/'))
    {
      handleCommand(u);
    }
    if (u.message().hasLocation())
    {
      String message_locate = "Користувач " + u.message().chat().username().decodeUnicode() + "надіслав координати:\nШирота: " + u.message().location().latitude().toString() + "\nДовгота: " + u.message().location().longitude().toString();
      bot.sendMessage(fb::Message(message_locate, u.message().chat().id()));
    }
  }
  if (question_bool[3] == true)
  {
    if (u.message().chat().id() == admin_chat_id)
    {
      bot.sendMessage(fb::Message(u.message().text().decodeUnicode(), ADMIN_ID));
    }
    else if (u.message().chat().id() == ADMIN_ID)
    {
      bot.sendMessage(fb::Message(u.message().text().decodeUnicode(), admin_chat_id));
    }
  }
  else if (question_bool[5] == true)
  {
    file_year = u.message().text().decodeUnicode();
    bot.sendMessage(fb::Message("Ввадіть назву, або якість ключові слова. Раджу використовувати короткі запити. А також, якщо Ви допустите орфографічну помилку в темі, я можу не знайти відповідь на запит. (Варото зазначити, що якщо файлів багато (наприклад статей за 2024 рік близько 400), то я можу довго обробляти запит. Також якщо відповідь: \"на Ваш запит відповіді не знайдено\", скористайтеся /google_server)", time_id));
    question_bool[5] = false;
    question_bool[6] = true;
  }
  else if (question_bool[6] == true)
  {
    Serial.println(file_type);
    Serial.println(file_year);
    Serial.println(u.message().text().decodeUnicode());
    server_NUWM(file_type, u.message().text().decodeUnicode(), file_year);
    question_bool[6] = false;
  }
  else if (question_bool[7] == true)
  {
    Google_reqest(u.message().text().decodeUnicode(), u.message().chat().id());
    question_bool[7] = false;
  }
  else if (question_bool[8] == true)
  {
    AI_Helper(u.message().text().decodeUnicode(), u.message().chat().id());
  }
  else if (question_bool[9] == true)
  {
    bot.sendMessage(fb::Message("За змовчуванням в налаштуваннях місто, для якого показується погода -Здолбунів. Але Ви можете отримати прогноз погоди для Вашої локації. Для цього просто надішліть GEO-точку в чат. Якщо не хочете надсилати GPS дані, просто відправте в чат будь-яке слово або символ", u.message().chat().id()));
    question_bool[10] = true;
    question_bool[9] = false;
  }
  else if (question_bool[10] == true)
  {
    if (u.message().hasLocation())
    {
      String latitud = u.message().location().latitude().toString();
      latitud = latitud.substring(0, (latitud.indexOf(".") + 5));
      String longitud = u.message().location().longitude().toString();
      longitud = longitud.substring(0, (longitud.indexOf(".") + 5));
      Serial.println(latitud);
      Serial.println(longitud);
      bot.sendMessage(fb::Message(weather(latitud, longitud), u.message().chat().id())); // weather(latitud, longitud);
    }
    else
    {
      bot.sendMessage(fb::Message(weather(), u.message().chat().id()));
    }
    question_bool[10] = false;
  }
  else if (question_bool[11] == true && !u.message().text().startsWith('/'))
  {
    Google_reqest(u.message().text().decodeUnicode(), u.message().chat().id());
    question_bool[11] = false;
  }
  else if (question_bool[12] == true)
  {
    if (!u.message().text().startsWith('/'))
    {
      String mess = askGemini(u.message().text().decodeUnicode());
      Serial.println(mess.length());
      if(mess.length() <= 4000){
        bot.sendMessage(fb::Message(mess, u.message().chat().id()));
      }else if(mess.length() > 4000 && mess.length() <= 8000){
        bot.sendMessage(fb::Message(mess.substring(4001, 8000), u.message().chat().id()));
      }else if(mess.length() > 8000 && mess.length() <= 12000){
        bot.sendMessage(fb::Message(mess.substring(8001, 12000), u.message().chat().id()));
      }else if(mess.length() > 12000 && mess.length() <= 16000){
        bot.sendMessage(fb::Message(mess.substring(12001, 16000), u.message().chat().id()));
      }
      // bot.sendMessage(fb::Message(mess, u.message().chat().id()));
    }
  }
}

//==========DOCUMENT===============================================
void handleDocument(fb::Update &u)
{
}

//==========QUERY==================================================
void handleQuery(fb::Update &u)
{
  fb::QueryRead q = u.query();
  bot.answerCallbackQuery(q.id(), q.data());
  if (question_bool[2] == true)
  {
    if (q.data() == "local")
    {
      if (u.message().chat().id() != ADMIN_ID)
      {
        bot.sendMessage(fb::Message("Ви не можете скористатися даною командою!", u.message().chat().id()));
      }
    }
    else if (q.data() == "global")
    {
      if (u.message().chat().id() != ADMIN_ID)
      {
        bot.sendMessage(fb::Message("Ви не можете скористатися даною командою!", u.message().chat().id()));
      }
    }
    else if (q.data() == "admin")
    {
      bot.sendMessage(fb::Message("Ви почали чат з адміністрацією. всі наступні повідомлення будуть надіслані адміну.", u.message().chat().id()));
      admin_chat_id = u.message().chat().id();
      question_bool[3] = true;
    }
    question_bool[2] = false;
  }
  else if (question_bool[4] == true)
  {
    if (q.data() == "type_1")
    {
      file_type = "";
      file_type = "metods/";
      sendMess(time_id);
    }
    else if (q.data() == "type_2")
    {
      file_type = "";
      file_type = "book/";
      sendMess(time_id);
    }
    else if (q.data() == "type_3")
    {
      file_type = "";
      file_type = "monograph/";
      sendMess(time_id);
    }
    else if (q.data() == "type_4")
    {
      file_type = "";
      file_type = "avdis/";
      sendMess(time_id);
    }
    else if (q.data() == "type_5")
    {
      file_type = "";
      file_type = "article/";
      sendMess(time_id);
    }
    else if (q.data() == "type_6")
    {
      file_type = "";
      file_type = "ndoc/";
      sendMess(time_id);
    }
    else if (q.data() == "type_7")
    {
      file_type = "";
      file_type = "edu=5Fprograms/";
      sendMess(time_id);
    }
    else if (q.data() == "type_8")
    {
      file_type = "";
      file_type = "newspaper/";
      sendMess(time_id);
    }
    else if (q.data() == "type_9")
    {
      file_type = "";
      file_type = "magazine/";
      sendMess(time_id);
    }
    else if (q.data() == "type_10")
    {
      file_type = "";
      file_type = "legal=5Fdocuments/";
      sendMess(time_id);
    }
    else if (q.data() == "type_11")
    {
      file_type = "";
      file_type = "woks=5Fprograms/";
      sendMess(time_id);
    }
    else if (q.data() == "type_12")
    {
      file_type = "";
      file_type = "syllabus/";
      sendMess(time_id);
    }
    else if (q.data() == "type_13")
    {
      file_type = "";
      file_type = "disc/";
      sendMess(time_id);
    }
  }
}

//==========UPDATEH=================================================
void updateh(fb::Update &u)
{
  if (u.isMessage())
    handleMessage(u);
  if (u.isMessage() && u.message().hasDocument())
    handleDocument(u);
  if (u.isQuery())
    handleQuery(u);
  Serial.print("heap: ");
  Serial.println(ESP.getFreeHeap());
}

void setup()
{
  Serial.begin(115200);
  connectWiFi();

  if (!SPIFFS.begin(true))
  {
    Serial.println("Помилка ініціалізації SPIFFS");
    return;
  }
  bot.attachUpdate(updateh);
  bot.setToken(F(BOT_TOKEN));
  bot.setPollMode(fb::Poll::Long, 20000);
  bot.sendMessage(fb::Message("Bot online!", ADMIN_ID));
}

void loop()
{
  bot.tick();
  if (millis() - timer >= 10000)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      connectWiFi();
    }
    timer = millis();
  }
}
