#include "JSEngine.hpp"
#include "WebView.hpp"
#include <iostream>
#include <setjmp.h>

JSEngine::JSEngine(WebView* webView) : webView(webView), lastError("") {
    // Create new JavaScript state
    J = js_newstate(nullptr, nullptr, JS_STRICT);
    
    if (!J) {
        lastError = "Failed to create JavaScript state";
        return;
    }
    
    // Set error reporting callback
    js_setreport(J, reportError);
    
    // Store reference to this JSEngine instance in the JavaScript context
    js_setcontext(J, this);
    
    // Set up basic bindings
    setupBasicBindings();
}

JSEngine::~JSEngine() {
    if (J) {
        js_freestate(J);
    }
}

bool JSEngine::executeScript(const std::string& script) {
    if (!J) {
        lastError = "JavaScript state not initialized";
        return false;
    }
    
    lastError = "";
    
    // Use safer execution without try/catch for now
    int result = js_dostring(J, script.c_str());
    
    if (result != 0) {
        lastError = "Script execution returned error code: " + std::to_string(result);
        return false;
    }
    
    return true;
}

void JSEngine::setupBasicBindings() {
    if (!J) return;
    
    // Create console object with log function
    js_newobject(J);
    {
        js_newcfunction(J, console_log, "log", 1);
        js_setproperty(J, -2, "log");
    }
    js_setglobal(J, "console");
    
    // Add basic Math object (mujs should provide this, but let's ensure it's there)
    // Note: mujs actually provides Math by default, but we can extend it if needed
    
    // TODO: We'll add DOM-related functions here later
    // For now, let's keep it simple and just provide console.log
}

void JSEngine::registerFunction(const std::string& name, js_CFunction func) {
    if (!J) return;
    
    js_newcfunction(J, func, name.c_str(), 0);
    js_setglobal(J, name.c_str());
}

void JSEngine::setGlobalString(const std::string& name, const std::string& value) {
    if (!J) return;
    
    js_pushstring(J, value.c_str());
    js_setglobal(J, name.c_str());
}

void JSEngine::setGlobalNumber(const std::string& name, double value) {
    if (!J) return;
    
    js_pushnumber(J, value);
    js_setglobal(J, name.c_str());
}

std::string JSEngine::getGlobalString(const std::string& name) {
    if (!J) return "";
    
    js_getglobal(J, name.c_str());
    if (js_isstring(J, -1)) {
        std::string result = js_tostring(J, -1);
        js_pop(J, 1);
        return result;
    }
    js_pop(J, 1);
    return "";
}

double JSEngine::getGlobalNumber(const std::string& name) {
    if (!J) return 0.0;
    
    js_getglobal(J, name.c_str());
    if (js_isnumber(J, -1)) {
        double result = js_tonumber(J, -1);
        js_pop(J, 1);
        return result;
    }
    js_pop(J, 1);
    return 0.0;
}

// Static callback functions
void JSEngine::reportError(js_State* J, const char* message) {
    JSEngine* engine = static_cast<JSEngine*>(js_getcontext(J));
    if (engine) {
        engine->lastError = std::string(message);
    }
    std::cerr << "JavaScript Error: " << message << std::endl;
}

void JSEngine::console_log(js_State* J) {
    // Get the number of arguments
    int argc = js_gettop(J);
    
    std::cout << "[JS Console] ";
    
    // Print each argument
    for (int i = 1; i <= argc; i++) {
        if (i > 1) std::cout << " ";
        
        if (js_isstring(J, i)) {
            std::cout << js_tostring(J, i);
        } else if (js_isnumber(J, i)) {
            std::cout << js_tonumber(J, i);
        } else if (js_isboolean(J, i)) {
            std::cout << (js_toboolean(J, i) ? "true" : "false");
        } else if (js_isnull(J, i)) {
            std::cout << "null";
        } else if (js_isundefined(J, i)) {
            std::cout << "undefined";
        } else {
            std::cout << "[object]";
        }
    }
    
    std::cout << std::endl;
}
