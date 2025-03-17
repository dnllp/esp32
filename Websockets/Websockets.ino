#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// Configura tus credenciales de WiFi
const char* ssid = "ESP32_AP";  // Nombre de la red WiFi creada por el ESP32
const char* password = "12345678";  // Contraseña de la red WiFi

// Crea un servidor web en el puerto 80
WebServer server(80);

// Crea un servidor WebSocket en el puerto 81
WebSocketsServer webSocket = WebSocketsServer(81);

// Página HTML que se servirá al cliente
const char* htmlPage = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 WebSocket Server</title>
    <script>
        var socket;
        function connect() {
            var ip = document.getElementById("ip").value;
            socket = new WebSocket("ws://" + ip + ":81");

            socket.onopen = function(event) {
                console.log("Conexión establecida");
                document.getElementById("status").innerHTML = "Conectado";
            };

            socket.onmessage = function(event) {
                console.log("Mensaje recibido: " + event.data);
                document.getElementById("messages").innerHTML += "<p>" + event.data + "</p>";
            };

            socket.onclose = function(event) {
                console.log("Conexión cerrada");
                document.getElementById("status").innerHTML = "Desconectado";
            };
        }

        function sendMessage() {
            var message = document.getElementById("message").value;
            if (socket && socket.readyState === WebSocket.OPEN) {
                socket.send(message);
                document.getElementById("message").value = "";
            } else {
                alert("No estás conectado al servidor WebSocket.");
            }
        }
    </script>
</head>
<body>
    <h1>ESP32 WebSocket Server</h1>
    <p>Estado: <span id="status">Desconectado</span></p>
    <input type="text" id="ip" placeholder="Dirección IP del ESP32" value="192.168.4.1" />
    <button onclick="connect()">Conectar</button>
    <hr>
    <input type="text" id="message" placeholder="Escribe un mensaje" />
    <button onclick="sendMessage()">Enviar</button>
    <hr>
    <div id="messages"></div>
</body>
</html>
)=====";

void setup() {
  // Inicia la comunicación serial
  Serial.begin(115200);

  // Configura el ESP32 como un punto de acceso (AP)
  WiFi.softAP(ssid, password);
  Serial.println("Punto de acceso iniciado");
  Serial.print("IP del AP: ");
  Serial.println(WiFi.softAPIP());

  // Inicia el servidor web
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Servidor web iniciado");

  // Inicia el servidor WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Servidor WebSocket iniciado");
}

void loop() {
  // Maneja las solicitudes del servidor web
  server.handleClient();

  // Maneja las conexiones WebSocket
  webSocket.loop();
}

// Función para manejar la solicitud de la página principal
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Función que maneja los eventos del WebSocket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Desconectado!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Conectado desde %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] Mensaje recibido: %s\n", num, payload);

      // Envía un mensaje de vuelta al cliente
      webSocket.sendTXT(num, "Mensaje recibido: " + String((char *)payload));
      break;
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}