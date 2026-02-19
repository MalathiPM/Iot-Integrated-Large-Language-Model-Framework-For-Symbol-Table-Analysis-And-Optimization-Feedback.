from flask import Flask, request
import json, os, subprocess, traceback, re
import markdown
import google.generativeai as genai

app = Flask(__name__)
symbol_table_file = "symbol_table.json"
symbol_table_cache = None
detected_language = None

table_css = """
<style>
    table { border-collapse: collapse; width: 100%; margin-bottom: 16px; }
    th, td { border: 1px solid #ccc; padding: 6px 10px; text-align: left; }
    th { background: #f8f8f8; }
</style>
"""

# Size mapping for different languages
SIZE_MAP = {
    'c': {'int': '4', 'float': '4', 'double': '8', 'char': '1', 'long': '8', 'short': '2', 'function': 'PTR', 'class': 'OBJ'},
    'cpp': {'int': '4', 'float': '4', 'double': '8', 'char': '1', 'long': '8', 'short': '2', 'function': 'PTR', 'class': 'OBJ'},
    'python': {'variable': 'OBJ', 'function': 'FUNC', 'class': 'OBJ'},
    'java': {'variable': 'REF', 'method': 'REF', 'class': 'OBJ'},
    'javascript': {'variable': 'REF', 'function': 'FUNC'}
}

def get_size_estimate(lang, sym_type):
    """Get intelligent size estimate instead of "?" """
    if lang in SIZE_MAP and sym_type in SIZE_MAP[lang]:
        return SIZE_MAP[lang][sym_type]
    return 'VAR' if sym_type == 'variable' else '?'

# Language detection patterns
LANGUAGE_PATTERNS = {
    'cpp': [r'#include\s+', r'int\s+main\s*\(', r'cout\s*<<', r'namespace\s+\w+'],
    'c': [r'#include\s+<stdio\.h>', r'int\s+main\s*\(', r'printf\s*\('],
    'java': [r'public\s+class\s+\w+', r'public\s+static\s+void\s+main', r'System\.out'],
    'python': [r'def\s+\w+\s*\(', r'print\s*\(', r'import\s+\w+'],
    'javascript': [r'function\s+\w+\s*\(', r'console\.log\s*\(', r'let\s+\w+\s*='],
    'rust': [r'fn\s+main\s*\(', r'println!\s*\('],
    'go': [r'func\s+main\s*\(', r'fmt\.Print'],
    'csharp': [r'using\s+System;', r'public\s+class\s+Program']
}

def detect_language(code):
    """Detect programming language from code"""
    code_lower = code.lower()
    scores = {}
    
    for lang, patterns in LANGUAGE_PATTERNS.items():
        score = sum(1 for pattern in patterns if re.search(pattern, code_lower))
        if score > 0:
            scores[lang] = score
    
    return max(scores, key=scores.get) if scores else 'unknown'

def generate_symbol_table_c(code):
    """C/C++ using your binary"""
    temp_c_file = "temp_code.c"
    try:
        with open(temp_c_file, "w") as f:
            f.write(code)
        
        proc = subprocess.run(
            ["./symbol_table_generator", temp_c_file],
            capture_output=True, text=True, timeout=10
        )
        
        if proc.returncode == 0 and os.path.exists(symbol_table_file):
            symbols = json.load(open(symbol_table_file))
            os.remove(temp_c_file)
            if os.path.exists(symbol_table_file):
                os.remove(symbol_table_file)
            return symbols
        if os.path.exists(temp_c_file):
            os.remove(temp_c_file)
        return None
    except:
        if os.path.exists(temp_c_file):
            os.remove(temp_c_file)
        return None

def generate_symbol_table_python(code):
    """Python symbol table"""
    symbols = []
    lines = code.split('\n')
    scope_stack = ['global']
    
    for line in lines:
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        
        # Classes
        class_match = re.match(r'class\s+(\w+)', line)
        if class_match:
            symbols.append({
                'name': class_match.group(1),
                'type': 'class',
                'scope': scope_stack[-1]
            })
            scope_stack.append(class_match.group(1))
            continue
        
        # Functions
        func_match = re.match(r'def\s+(\w+)\s*\(', line)
        if func_match:
            symbols.append({
                'name': func_match.group(1),
                'type': 'function',
                'scope': scope_stack[-1]
            })
            scope_stack.append(func_match.group(1))
            continue
        
        # Variables (simple assignment)
        var_match = re.match(r'(\w+)\s*=', line)
        if var_match and var_match.group(1) not in ['if', 'for', 'while', 'def', 'class']:
            symbols.append({
                'name': var_match.group(1),
                'type': 'variable',
                'scope': scope_stack[-1]
            })
        
        # Scope end (basic)
        if line == 'return' and len(scope_stack) > 1:
            scope_stack.pop()
    
    return symbols

def generate_symbol_table_java(code):
    """Java symbol table"""
    symbols = []
    lines = code.split('\n')
    
    for line in lines:
        line = line.strip()
        if not line or line.startswith('//'):
            continue
        
        # Classes
        class_match = re.search(r'class\s+(\w+)', line)
        if class_match:
            symbols.append({'name': class_match.group(1), 'type': 'class', 'scope': 'global'})
        
        # Methods
        method_match = re.search(r'(\w+)\s+(\w+)\s*\(', line)
        if method_match:
            symbols.append({
                'name': method_match.group(2),
                'type': 'method',
                'scope': 'class',
                'additionalInfo': method_match.group(1)
            })
    
    return symbols

def generate_symbol_table_js(code):
    """JavaScript symbol table"""
    symbols = []
    lines = code.split('\n')
    
    for line in lines:
        line = line.strip()
        if not line or line.startswith('//'):
            continue
        
        # Functions
        func_match = re.match(r'(?:function\s+)?(\w+)\s*\(', line)
        if func_match and func_match.group(1) != 'function':
            symbols.append({'name': func_match.group(1), 'type': 'function', 'scope': 'global'})
        
        # Variables
        var_match = re.match(r'(?:let|const|var)\s+(\w+)', line)
        if var_match:
            symbols.append({'name': var_match.group(1), 'type': 'variable', 'scope': 'global'})
    
    return symbols

def generate_prompt(symbol_table):
    return (
        f"Multi-language code optimizer. Language: {detected_language}\n"
        "Analyze symbol table. Focus on memory, scope, best practices.\n"
        "Markdown table ONLY: Name | Type | Scope | Recommendation\n\n"
        "| Name | Type | Scope | Additional Info | Size |\n|------|------|-------|----------------|------|\n" +
        "".join(f"| {s['name']} | {s['type']} | {s['scope']} | {s.get('additionalInfo', '')} | {get_size_estimate(detected_language, s['type'])} |\n" for s in symbol_table)
    )

def analyze_symbol_table(symbol_table):
    api_key = os.getenv("GEMINI_API_KEY")
    if not api_key:
        raise RuntimeError("GEMINI_API_KEY not set")
    
    genai.configure(api_key=api_key)
    model = genai.GenerativeModel('gemini-2.0-flash')
    response = model.generate_content(generate_prompt(symbol_table))
    return response.text

def wrap_html_table(table_html, title):
    return f"{table_css}<div><h3>{title}</h3>{table_html}</div>"

@app.route('/process_code', methods=['POST'])
def process_code():
    global symbol_table_cache, detected_language
    
    code = request.get_data(as_text=True)
    detected_language = detect_language(code)
    
    # Generate symbols by language
    if detected_language in ['c', 'cpp']:
        symbols = generate_symbol_table_c(code)
    elif detected_language == 'python':
        symbols = generate_symbol_table_python(code)
    elif detected_language == 'java':
        symbols = generate_symbol_table_java(code)
    elif detected_language == 'javascript':
        symbols = generate_symbol_table_js(code)
    else:
        symbols = []
    
    if not symbols:
        return f"""
        <div style='padding:20px; background:#fef5e7; border-left:4px solid #f59e0b; border-radius:10px;'>
        <h3>🔍 Detected: {detected_language}</h3>
        <p>Limited support. C/C++ fully supported.</p>
        </div>
        """, 200
    
    symbol_table_cache = symbols
    
    # HTML table with SMART sizes
    html = "<table>"
    html += "<tr><th>Name</th><th>Type</th><th>Scope</th><th>Info</th><th>Size</th></tr>"
    for sym in symbols:
        size = get_size_estimate(detected_language, sym['type'])
        html += f"<tr><td>{sym['name']}</td><td>{sym['type']}</td><td>{sym['scope']}</td>"
        html += f"<td>{sym.get('additionalInfo', '')}</td><td><strong>{size}</strong></td></tr>"
    html += "</table>"
    
    return wrap_html_table(html, f"✅ {detected_language.upper()} ({len(symbols)} symbols)"), 200

@app.route('/run_llm', methods=['POST'])
def run_llm():
    global symbol_table_cache
    
    try:
        if not symbol_table_cache:
            return """
            <div style='padding:20px; background:#fed7d7; border-radius:10px; color:#c53030;'>
            <h3>⚠️ Generate Symbol Table First!</h3>
            </div>
            """, 400
        
        analysis_html = markdown.markdown(analyze_symbol_table(symbol_table_cache), extensions=['tables'])
        return wrap_html_table(analysis_html, f"🧠 {detected_language.upper()} Analysis"), 200
        
    except Exception as e:
        if any(x in str(e).lower() for x in ['429', 'quota']):
            return """
            <div style='padding:20px; background:#fed7d7; border-radius:10px; color:#c53030;'>
            <h3>🛑 Free Tier Limit</h3><p>Wait 35s or enable billing</p>
            </div>
            """, 429
        return f"<p>Error: {str(e)}</p>", 500

@app.route('/', methods=['GET'])
def index():
    return """
    <h1>🌍 Multi-Language Symbol Table Analyzer</h1>
    <ul>
    <li>✅ C/C++: 4/8/PTR bytes</li>
    <li>✅ Python: OBJ/FUNC</li>
    <li>✅ Java: REF/OBJ</li>
    <li>✅ JavaScript: REF/FUNC</li>
    </ul>
    <p>Endpoints: /process_code → /run_llm</p>
    """

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
