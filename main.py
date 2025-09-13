#!/usr/bin/env python3
"""
Telnet Listener - слушает на порту 23 и логирует входящие подключения
Выводит IP и порт каждого подключившегося клиента
"""

import socket
import threading
import time
import sys
from datetime import datetime

class TelnetListener:
    def __init__(self, host='0.0.0.0', port=8080):
        self.host = host
        self.port = port
        self.running = True
        self.connections_count = 0
        self.start_time = time.time()

    def handle_client(self, client_socket, client_address):
        """Обработка клиентского соединения"""
        client_ip = client_address[0]
        client_port = client_address[1]
        
        # Вывод информации о подключении
        timestamp = datetime.now().strftime('%H:%M:%S')
        print(f"[{timestamp}] CONNECTION: {client_ip}:{client_port}")
        
        # Увеличиваем счетчик подключений
        self.connections_count += 1
        
        try:
            # Отправляем простой Telnet banner
            welcome_msg = f"Welcome! Your IP: {client_ip}:{client_port}\r\n"
            client_socket.send(welcome_msg.encode('utf-8'))
            
            # Слушаем данные от клиента (но не обрабатываем)
            client_socket.settimeout(30.0)  # 30 секунд таймаут
            
            while self.running:
                try:
                    data = client_socket.recv(1024)
                    if not data:
                        break
                    # Просто эхо для поддержания соединения
                    if data.strip():
                        echo = f"Echo: {data.decode('utf-8', errors='ignore').strip()}\r\n"
                        client_socket.send(echo.encode('utf-8'))
                except socket.timeout:
                    break
                except:
                    break
                    
        except Exception as e:
            print(f"[ERROR] Client {client_ip}:{client_port} - {e}", file=sys.stderr)
        finally:
            try:
                client_socket.close()
            except:
                pass
            print(f"[{datetime.now().strftime('%H:%M:%S')}] DISCONNECTED: {client_ip}:{client_port}")

    def stats_worker(self):
        """Поток статистики"""
        while self.running:
            time.sleep(10)  # Статистика каждые 10 секунд
            uptime = time.time() - self.start_time
            uptime_str = f"{int(uptime//3600):02d}:{int((uptime%3600)//60):02d}:{int(uptime%60):02d}"
            
            print(f"[STATS] Connections: {self.connections_count} | Uptime: {uptime_str}", file=sys.stderr)

    def run(self):
        """Запуск Telnet слушателя"""
        print("🚀 Telnet Connection Listener", file=sys.stderr)
        print(f"📡 Listening on: {self.host}:{self.port}", file=sys.stderr)
        print("🎯 Logging all incoming connections", file=sys.stderr)
        print("=" * 40, file=sys.stderr)
        
        # Создаем серверный сокет
        try:
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind((self.host, self.port))
            server_socket.listen(100)  # Очередь до 100 подключений
            
            print(f"✅ Server started on {self.host}:{self.port}", file=sys.stderr)
            
        except PermissionError:
            print("❌ Permission denied! Port 23 requires root privileges", file=sys.stderr)
            print("💡 Try: sudo python3 telnet_listener.py", file=sys.stderr)
            return
        except Exception as e:
            print(f"❌ Failed to start server: {e}", file=sys.stderr)
            return

        # Запускаем поток статистики
        stats_thread = threading.Thread(target=self.stats_worker)
        stats_thread.daemon = True
        stats_thread.start()

        try:
            print("🎲 Waiting for connections... Press Ctrl+C to stop", file=sys.stderr)
            
            while self.running:
                try:
                    # Принимаем входящие соединения
                    client_socket, client_address = server_socket.accept()
                    
                    # Запускаем отдельный поток для каждого клиента
                    client_thread = threading.Thread(
                        target=self.handle_client,
                        args=(client_socket, client_address)
                    )
                    client_thread.daemon = True
                    client_thread.start()
                    
                except Exception as e:
                    if self.running:
                        print(f"[ERROR] Accept failed: {e}", file=sys.stderr)
                        
        except KeyboardInterrupt:
            print("\n🛑 Stopping Telnet listener...", file=sys.stderr)
        finally:
            self.running = False
            try:
                server_socket.close()
            except:
                pass
            print("👋 Server stopped", file=sys.stderr)

def main():
    # Можно передать порт как аргament
    port = 8080
    if len(sys.argv) > 1:
        try:
            port = int(sys.argv[1])
        except ValueError:
            print("Usage: python3 telnet_listener.py [port]", file=sys.stderr)
            return

    listener = TelnetListener(port=port)
    listener.run()

if __name__ == "__main__":
    main()
