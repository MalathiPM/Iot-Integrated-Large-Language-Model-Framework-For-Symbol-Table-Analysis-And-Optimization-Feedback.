# Iot-Integrated-Large-Language-Model-Framework-For-Symbol-Table-Analysis-And-Optimization-Feedback.
An AI-based symbol table optimization system that analyses variables and generates actionable memory efficiency recommendations using Google Gemini LLM API.
1. **ESP32**: Minimal web server. Serves UI, relays POST /extract and /analyze.
2. **Backend**: Python/Flask orchestrates symbol extraction, prompt building, LLM calls.
3. **Extractor**: Parses C translation unit → JSON (name, type, scope, size, inferred issues).
4. **LLM Engine**: Prompt-bounded GPT-4o-mini or equivalent → validated Markdown table → HTML.
5. **Fallback**: Deterministic rules emit identical schema when LLM unavailable.

### Prerequisites
- ESP32 with Arduino IDE (WiFi library)
- Python 3.10+, Flask, requests
- OpenAI API key (or local LLM)

### Backend
- pip install -r requirements.txt
- python app.py
- run the ESP 32 Web server
- Backend runs on http://localhost:5000
