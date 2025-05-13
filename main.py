import socket
import threading
import requests
from bs4 import BeautifulSoup
import google.generativeai as genai

API_KEY = "AIzaSyC_J9gzHlAI_5cvppZRq0z19hdF-79_vjA"
genai.configure(api_key = API_KEY)
model = genai.GenerativeModel('gemini-2.0-pro-exp-02-05')
chat = model.start_chat(history=[])

HOST = "0.0.0.0"
PORT = 8080


headers = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36"
}

def handle_client(conn, addr):
    try:
        print(f"Підключено клієнта: {addr}")
        data = conn.recv(1024).decode('utf-8')
        if not data:
            print("Клієнт надіслав порожній запит.")
            return

        print(f"Отримано: {data}")
        if data.find("#NUWM#") != -1:
            data = data.replace("#NUWM#", "")
            parts = data.split('<')
            text = parts[0].strip()
            link = parts[1].replace('>', '').strip()
            print(f"Текст: {text}, Посилання: {link}")

            response = requests.get(link)
            soup = BeautifulSoup(response.text, 'html.parser')

            search_text = ' '.join(text.lower().split())

            found_link = None

            for a_tag in soup.find_all('a', href=True):
                a_tag_text = a_tag.get_text().replace('&#13;', '').replace('\n', '').replace('\r', '')
                a_tag_text = ' '.join(a_tag_text.lower().split())

                if search_text in a_tag_text:
                    found_link = a_tag['href']
                    break

            if found_link:
                found_link = found_link.replace("http://", "https://")
                print(f"Знайдено посилання: {found_link}")
                conn.sendall(found_link.encode('utf-8'))
            else:
                print("Не вдалося знайти відповідне посилання.")
                conn.sendall("Посилання не знайдено.")
        if data.find("#GOOGLE#") != -1:
            link_google = data.replace("#GOOGLE#", "")
            google_response = requests.get(link_google, headers=headers)
            google_response.raise_for_status()

            google_soup = BeautifulSoup(google_response.text, 'html.parser')

            pdf_links = [google_link.get('href') for google_link in google_soup.find_all('a', href=True) if ".pdf" in google_link.get('href')]
            print("PDF посилання знайдені на сторінці:")
            google_links = ""
            i = 0
            for google_link in pdf_links:
                print(google_link)
                i += 1
                google_links += "<" + google_link + ">\n"

            google_links += "$" + str(i)
            conn.sendall(google_links.encode('utf-8'))
        elif data.find("#AI_HELP#") != -1:
            if data.find("#AI_HELP#/ai_helper") == -1:
                if  data.find("#AI_HELP#/ai_end") == -1:
                    user_input = data.replace("#AI_HELP#", "")
                    response = chat.send_message(user_input)
                    print("Gemini: ", response.text)
                    ai_text = response.text
                    conn.sendall(ai_text.encode('utf-8'))

    except Exception as e:
        print(f"Помилка під час обробки клієнта {addr}: {e}")
    finally:
        conn.close()
        print(f"Клієнт {addr} відключився.")


def start_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind((HOST, PORT))
        server_socket.listen(5)
        print(f"Сервер запущено на {HOST}:{PORT}. Очікуємо підключень...")

        while True:
            conn, addr = server_socket.accept()
            client_thread = threading.Thread(target=handle_client, args=(conn, addr))
            client_thread.start()

if __name__ == "__main__":
    start_server()
