/*
 * INA219 modülü vasıtasıyla gerilim, akım ve güç ölçümü
 * */


#include <ESP8266WiFi.h>      // 
#include <ESPAsyncTCP.h>      // 
#include <ESPAsyncWebServer.h>
#include <Adafruit_INA219.h>
#include <Wire.h>
#include "hesap.h"            // Wi-fi hesap ve IP ayarlarının olduğu dosya

// --- PIN TANIMLARI (NodeMCU Standart I2C) ---
// SDA -> D2 (GPIO 4), SCL -> D1 (GPIO 5)
#define I2C_SDA 4
#define I2C_SCL 5

// --- OBJELER ---
Adafruit_INA219 ina219;
AsyncWebServer server(80);

// --- DEĞİŞKENLER ---
float currentV = 0, currentMA = 0;
float totalEnergyWh = 0;
unsigned long startTime = 0;
unsigned long lastSampleTime = 0;
bool isMeasuring = false;
int measureMode = 0; // 0: Sürekli, 1: 2 Dakika, 2: 10 Dakika
int remainingTime = 0;
String lastResult = "Hazir";

// --- WEB ARAYÜZÜ (HTML + CSS + JS) ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>
<title>ESP8266 Power Analyzer</title>
<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>
<style>
  body { font-family: 'Segoe UI', sans-serif; text-align: center; background: #121212; color: #e0e0e0; margin: 0; padding: 20px; }
  .container { max-width: 800px; margin: auto; background: #1e1e1e; padding: 25px; border-radius: 15px; box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
  h2 { color: #4caf50; }
  .btn-group { display: flex; justify-content: center; gap: 10px; margin-bottom: 20px; flex-wrap: wrap; }
  button { padding: 12px 24px; border: none; border-radius: 8px; cursor: pointer; font-weight: bold; background: #333; color: white; transition: 0.3s; }
  button:hover { background: #555; }
  .btn-stop { background: #c62828 !important; }
  .spinner { display: none; margin: 15px auto; width: 40px; height: 40px; border: 5px solid #333; border-top: 5px solid #4caf50; border-radius: 50%; animation: spin 1s linear infinite; }
  @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
  #timer-display { font-size: 1.2em; color: #ff9800; font-weight: bold; margin: 10px 0; display: none; }
  .stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 15px; margin: 25px 0; }
  .stat-box { background: #252525; padding: 15px; border-radius: 10px; border: 1px solid #333; }
  .stat-val { font-size: 1.5em; color: #4caf50; display: block; font-weight: bold; }
  #result-box { margin-top: 20px; padding: 15px; background: #1b5e20; border-radius: 10px; display: none; border: 2px solid #4caf50; }
</style></head><body>
<div class='container'>
  <h2> TA2EI ESP8266 Güç Çözümleyici</h2>
  <div class='btn-group'>
    <button onclick="start(0)">Sürekli</button>
    <button onclick="start(1)">2 Dakika</button>
    <button onclick="start(2)">10 Dakika</button>
    <button class='btn-stop' onclick="stop()">Durdur</button>
  </div>
  <div id='spinner' class='spinner'></div>
  <div id='timer-display'>Kalan Süre: <span id='timer'>0</span>s</div>
  <div class='stats'>
    <div class='stat-box'><span>Gerilim</span><span id='v' class='stat-val'>0.00</span>V</div>
    <div class='stat-box'><span>Akım</span><span id='ma' class='stat-val'>0.0</span>mA</div>
    <div class='stat-box'><span>Güç</span><span id='w' class='stat-val'>0.00</span>W</div>
  </div>
  <canvas id='chart' style='width:100%; height:300px;'></canvas>
  <div id='result-box'><h3 id='result-text'>Ölçüm Tamamlandı</h3></div>
</div>
<script>
var ctx = document.getElementById('chart').getContext('2d');
var chart = new Chart(ctx, {
  type: 'line',
  data: { labels: [], datasets: [
    { label: 'Volt', borderColor: '#ffcd56', data: [], yAxisID: 'yV', pointRadius: 0, tension: 0.2 },
    { label: 'mA', borderColor: '#4bc0c0', data: [], yAxisID: 'yMA', pointRadius: 0, tension: 0.2 }
  ]},
  options: { animation: false, scales: { yV: { min: 0, max: 6 }, yMA: { min: 0, max: 3000, position: 'right' } } }
});
function start(m) { fetch('/start?m='+m); document.getElementById('result-box').style.display='none'; chart.data.labels=[]; chart.data.datasets[0].data=[]; chart.data.datasets[1].data=[]; }
function stop() { fetch('/stop'); }
setInterval(function() {
  fetch('/data').then(r => r.json()).then(d => {
    document.getElementById('v').innerText = d.v.toFixed(2);
    document.getElementById('ma').innerText = d.ma.toFixed(1);
    document.getElementById('w').innerText = (d.v * d.ma / 1000).toFixed(2);
    if(d.run) {
      document.getElementById('spinner').style.display = 'block';
      if(d.mode > 0) { document.getElementById('timer-display').style.display='block'; document.getElementById('timer').innerText=d.rem; }
      if(chart.data.labels.length > 40) { chart.data.labels.shift(); chart.data.datasets[0].data.shift(); chart.data.datasets[1].data.shift(); }
      chart.data.labels.push(new Date().toLocaleTimeString());
      chart.data.datasets[0].data.push(d.v); chart.data.datasets[1].data.push(d.ma);
      chart.update('none');
    } else {
      document.getElementById('spinner').style.display='none'; document.getElementById('timer-display').style.display='none';
      if (d.res !== "Hazir") { document.getElementById('result-box').style.display='block'; document.getElementById('result-text').innerText=d.res; }
    }
  });
}, 1000);
</script></body></html>)rawliteral";

void setup() {
  Serial.begin(115200);
  delay(500);

  // --- ESP8266 STATIK IP KONFIGURASYONU ---
  WiFi.mode(WIFI_STA);
  // ESP8266'da config parametre sırası: IP, Gateway, Subnet, DNS1, DNS2
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Statik IP Hatasi!");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nESP8266 Baglandi!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  // --- SENSOR VE I2C ---
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!ina219.begin()) {
    Serial.println("INA219 bulunamadi!");
  }

  // --- WEB ROUTER ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("m")) {
      measureMode = request->getParam("m")->value().toInt();
      totalEnergyWh = 0; startTime = millis(); lastSampleTime = millis(); isMeasuring = true;
      request->send(200, "text/plain", "OK");
    }
  });

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request){
    isMeasuring = false; lastResult = "Durduruldu.";
    request->send(200, "text/plain", "OK");
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"v\":"+String(currentV)+",\"ma\":"+String(currentMA)+",\"run\":"+(isMeasuring?"true":"false")+",\"mode\":"+String(measureMode)+",\"rem\":"+String(remainingTime)+",\"res\":\""+lastResult+"\"}";
    request->send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  currentV = ina219.getBusVoltage_V();
  currentMA = ina219.getCurrent_mA();

  if (isMeasuring) {
    unsigned long now = millis();
    unsigned long elapsed = (now - startTime) / 1000;
    
    if (measureMode == 1) remainingTime = 120 - elapsed;
    else if (measureMode == 2) remainingTime = 600 - elapsed;
    else remainingTime = 0;

    if (now - lastSampleTime >= 1000) {
      totalEnergyWh += (currentV * currentMA / 1000.0) * (1.0 / 3600.0);
      lastSampleTime = now;
    }

    if ((measureMode == 1 && elapsed >= 120) || (measureMode == 2 && elapsed >= 600)) {
      isMeasuring = false;
      float forecast = totalEnergyWh * (measureMode == 1 ? 30.0 : 6.0);
      lastResult = "Saatlik Tuketim Ongorusu: " + String(forecast, 4) + " Wh";
    }
  }
}
