#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer_Generic.h>


// Configura tus credenciales de WiFi (AP)
const char* ssid = "ESP32_AP";  // Nombre de la red WiFi creada por el ESP32
const char* password = "12345678";  // Contraseña de la red WiFi

// Pin del LED
const int ledPin = 2;  // GPIO 2

// Crea un servidor web en el puerto 80
WebServer server(80);

// Crea un servidor WebSocket en el puerto 81
WebSocketsServer webSocket = WebSocketsServer(81);

// Página HTML que se servirá al cliente
const char* htmlPage = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>Control de LED</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
        button { padding: 10px 20px; font-size: 16px; margin: 10px; }
        .on { background-color: green; color: white; }
        .off { background-color: red; color: white; }
    </style>
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
                updateLEDState(event.data);
            };

            socket.onclose = function(event) {
                console.log("Conexión cerrada");
                document.getElementById("status").innerHTML = "Desconectado";
            };
        }

        function updateLEDState(state) {
            var ledState = document.getElementById("ledState");
            if (state === "ON") {
                ledState.innerHTML = "LED: ENCENDIDO";
                ledState.className = "on";
            } else if (state === "OFF") {
                ledState.innerHTML = "LED: APAGADO";
                ledState.className = "off";
            }
        }

        function sendCommand(command) {
            if (socket && socket.readyState === WebSocket.OPEN) {
                socket.send(command);
            } else {
                alert("No estás conectado al servidor WebSocket.");
            }
        }
    </script>
</head>
<body>
    <h1>Control de LED con ESP32</h1>
    <p>Estado: <span id="status">Desconectado</span></p>
    <input type="text" id="ip" placeholder="Dirección IP del ESP32" value="192.168.4.1" />
    <button onclick="connect()">Conectar</button>
    <hr>
    <p id="ledState" class="off">LED: APAGADO</p>
    <button onclick="sendCommand('ON')">Encender LED</button>
    <button onclick="sendCommand('OFF')">Apagar LED</button>
</body>
</html>
)=====";

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
        // Envía el estado actual del LED al cliente
        String ledState = digitalRead(ledPin) ? "ON" : "OFF";
        webSocket.sendTXT(num, ledState);
      }
      break;
    case WStype_TEXT:
      {
        String message = String((char *)payload);
        Serial.printf("[%u] Mensaje recibido: %s\n", num, message.c_str());

        // Controla el LED según el mensaje recibido
        if (message == "ON") {
          digitalWrite(ledPin, HIGH);
          webSocket.sendTXT(num, "ON");  // Envía el estado actualizado
        } else if (message == "OFF") {
          digitalWrite(ledPin, LOW);
          webSocket.sendTXT(num, "OFF");  // Envía el estado actualizado
        }
      }
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

void setup() {
  // Inicia la comunicación serial
  Serial.begin(115200);

  // Configura el pin del LED como salida
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  // Inicia con el LED apagado

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
