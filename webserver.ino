#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

// Wi-Fi credentials
const char* ssid = "MPMS home";
const char* password = "Malu@2004";
WebServer server(80);

// Flask server IP
const char* flaskServer = "192.168.31.246:5000";

// Store the latest table results
String latestSymbolTable = "";
String latestAnalysis = "";

void handleRoot() {
  String html = 
    "<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>Code Analyzer</title>"
    "<style>"
    "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 0; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: #333; }"
    ".container { max-width: 1200px; margin: 0 auto; padding: 20px; }"
    "header { text-align: center; padding: 20px 0; background: rgba(255, 255, 255, 0.9); border-radius: 10px; margin-bottom: 20px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }"
    "h1 { margin: 0; color: #4a5568; font-size: 2.5em; }"
    ".section { background: rgba(255, 255, 255, 0.95); padding: 20px; margin-bottom: 20px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }"
    "h2 { color: #2d3748; border-bottom: 2px solid #e2e8f0; padding-bottom: 10px; }"
    "textarea { width: 100%; height: 300px; padding: 10px; border: 1px solid #cbd5e0; border-radius: 5px; font-family: monospace; resize: vertical; }"
    ".buttons { display: flex; gap: 10px; flex-wrap: wrap; margin-top: 10px; }"
    "button { background: #3182ce; color: white; border: none; padding: 12px 20px; border-radius: 5px; cursor: pointer; font-size: 1em; transition: background 0.3s; }"
    "button:hover { background: #2c5282; }"
    "button:disabled { background: #a0aec0; cursor: not-allowed; }"
    ".loading { display: none; color: #ffffff; font-weight: bold; position: fixed; top: 20px; right: 20px; background: rgba(0,0,0,0.8); padding: 10px; border-radius: 5px; }"
    ".results { margin-top: 20px; }"
    ".result-section { border: 1px solid #e2e8f0; border-radius: 5px; margin-bottom: 10px; }"
    ".result-header { background: #f7fafc; padding: 10px; font-weight: bold; display: flex; justify-content: space-between; align-items: center; cursor: pointer; }"
    ".result-content { display: none; padding: 15px; background: #ffffff; overflow: auto; max-height: 400px; }"
    ".result-content.show { display: block; }"
    "table { width: 100%; border-collapse: collapse; margin-top: 10px; font-size: 0.9em; }"
    "th, td { border: 1px solid #e2e8f0; padding: 8px; text-align: local; }"
    "th { background: #edf2f7; font-weight: bold; }"
    ".warning { background: #fed7d7; color: #c53030; padding: 10px; border-radius: 5px; margin: 10px 0; }"
    "@media (max-width: 768px) { .buttons { flex-direction: column; } button { width: 100%; } }"
    "</style>"
    "</head>"
    "<body>"
    "<div class='container'>"
    "<header><h1>IoT-Integrated C Code Analyzer</h1><p>Symbol Table + AI Optimization (Free Tier)</p></header>"
    
    "<div class='section'>"
    "<h2>📝 Enter Your Source Code</h2>"
    "<form id='codeForm'>"
    "<textarea name='code' placeholder='Paste your C code here...'>#include &lt;stdio.h&gt;&#10;int main() {&#10;    float pi = 3.14;&#10;    float radius = 5.0;&#10;    float area = pi * radius * radius;&#10;    printf(&quot;Area: %f&quot;, area);&#10;    return 0;&#10;}</textarea>"
    "</form>"
    "<div class='warning'>⚠️ <strong>Free Tier Notice:</strong> LLM Analysis limited. Symbol Table always works!</div>"
    "<div class='buttons'>"
    "<button onclick='generateSymbolTable()'>📊Generate Symbol Table</button>"
    "<button onclick='runLLMAnalysis()'>🧠Run LLM Analysis</button>"
    "</div>"
    "</div>"
    
    "<div class='results'>"
    "<div class='result-section'>"
    "<div class='result-header' onclick='toggleResult(\"symbolResult\", \"symbolToggle\")'>"
    "📊 Symbol Table <span id='symbolToggle'>▶</span>"
    "</div>"
    "<div id='symbolResult' class='result-content'></div>"
    "</div>"
    "<div class='result-section'>"
    "<div class='result-header' onclick='toggleResult(\"analysisResult\", \"analysisToggle\")'>"
    "🧠 LLM Analysis <span id='analysisToggle'>▶</span>"
    "</div>"
    "<div id='analysisResult' class='result-content'></div>"
    "</div>"
    "</div>"
    
    "<div id='loading' class='loading'>🔄 Processing...</div>"
    "</div>"
    
    "<script>"
    "function toggleResult(contentId, toggleId) {"
    "  const content = document.getElementById(contentId);"
    "  const toggle = document.getElementById(toggleId);"
    "  if (content.classList.contains('show')) {"
    "    content.classList.remove('show');"
    "    toggle.textContent = '▶';"
    "  } else {"
    "    content.classList.add('show');"
    "    toggle.textContent = '▼';"
    "  }"
    "}"
    
    "async function generateSymbolTable() {"
    "  const button = document.querySelector('button[onclick*=\"generateSymbolTable\"]');"
    "  button.disabled = true;"
    "  button.textContent = 'Generating...';"
    "  document.getElementById('loading').style.display = 'block';"
    "  const code = document.querySelector('#codeForm textarea').value;"
    "  try {"
    "    const resp = await fetch('/symbol_table', {"
    "      method: 'POST',"
    "      headers: {'Content-Type': 'text/plain'},"
    "      body: code"
    "    });"
    "    if (resp.ok) {"
    "      const text = await resp.text();"
    "      document.getElementById('symbolResult').innerHTML = text;"
    "      document.getElementById('symbolResult').classList.add('show');"
    "      document.getElementById('symbolToggle').textContent = '▼';"
    "    } else {"
    "      document.getElementById('symbolResult').innerHTML = '<p style=\"color:red;\">Error: ' + resp.status + ' - ' + await resp.text() + '</p>';"
    "    }"
    "  } catch (err) {"
    "    document.getElementById('symbolResult').innerHTML = '<p style=\"color:red;\">Network Error: ' + err.message + '</p>';"
    "  }"
    "  button.disabled = false;"
    "  button.textContent = '📊 Generate Symbol Table';"
    "  document.getElementById('loading').style.display = 'none';"
    "}"
    
    "async function runLLMAnalysis() {"
    "  const button = document.querySelector('button[onclick*=\"runLLMAnalysis\"]');"
    "  const symbolResult = document.getElementById('symbolResult');"
    "  if (symbolResult.innerHTML === '' || symbolResult.children.length === 0) {"
    "    alert('⚠️ Please generate Symbol Table first!');"
    "    return;"
    "  }"
    "  button.disabled = true;"
    "  button.textContent = 'Analyzing...';"
    "  document.getElementById('loading').style.display = 'block';"
    "  try {"
    "    const resp = await fetch('/llm_analysis', {method:'POST'});"
    "    if (resp.ok) {"
    "      const text = await resp.text();"
    "      document.getElementById('analysisResult').innerHTML = text;"
    "      document.getElementById('analysisResult').classList.add('show');"
    "      document.getElementById('analysisToggle').textContent = '▼';"
    "    } else {"
    "      const errorText = await resp.text();"
    "      if (errorText.includes('429') || errorText.includes('quota')) {"
    "        document.getElementById('analysisResult').innerHTML = '<div class=\"warning\">🛑 <strong>Free Tier Limit Reached</strong><br>Symbol table works perfectly! For AI analysis:<br>1. Wait ~35 seconds and retry<br>2. Or enable billing (Tier 1) for higher limits</div>';"
    "      } else {"
    "        document.getElementById('analysisResult').innerHTML = '<p style=\"color:red;\">Error: ' + resp.status + ' - ' + errorText + '</p>';"
    "      }"
    "    }"
    "  } catch (err) {"
    "    document.getElementById('analysisResult').innerHTML = '<p style=\"color:red;\">Network Error: ' + err.message + '</p>';"
    "  }"
    "  button.disabled = false;"
    "  button.textContent = '🧠 Run LLM Analysis';"
    "  document.getElementById('loading').style.display = 'none';"
    "}"
    "</script>"
    "</body></html>";
 
  server.send(200, "text/html", html);
}

void handleSymbolTable() {
  if (server.hasArg("plain")) {
    String code = server.arg("plain");
    HTTPClient http;
    http.begin(String("http://") + flaskServer + "/process_code");
    http.addHeader("Content-Type", "text/plain");
    
    int httpCode = http.POST(code);
    String resp = http.getString();
    http.end();
    
    if (httpCode == 200) {
      latestSymbolTable = resp;
      server.send(200, "text/html", resp);
    } else {
      server.send(500, "text/plain", "Symbol Table Error: " + String(httpCode));
    }
  } else {
    server.send(400, "text/plain", "No C code received");
  }
}

void handleLLMAnalysis() {
  HTTPClient http;
  http.begin(String("http://") + flaskServer + "/run_llm");
  int httpCode = http.POST("");
  String resp = http.getString();
  http.end();
  
  if (httpCode == 200) {
    latestAnalysis = resp;
    server.send(200, "text/html", resp);
  } else {
    server.send(httpCode, "text/plain", resp);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Flask: http://");
  Serial.println(flaskServer);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/symbol_table", HTTP_POST, handleSymbolTable);
  server.on("/llm_analysis", HTTP_POST, handleLLMAnalysis);

  server.begin();
  Serial.println("🚀 ESP32 Web Server ready!");
}

void loop() {
  server.handleClient();
}
