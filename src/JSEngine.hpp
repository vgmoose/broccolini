#ifndef JSENGINE_HPP
#define JSENGINE_HPP

#include "../libs/mujs/mujs.h"
#include <string>
#include <map>
#include <functional>

class WebView;

class JSEngine {
public:
    JSEngine(WebView* webView);
    ~JSEngine();
    
    // Execute JavaScript code
    bool executeScript(const std::string& script);
    
    // Set up basic DOM bindings (without document/window initially)
    void setupBasicBindings();
    
    // Register a C++ function to be callable from JavaScript
    void registerFunction(const std::string& name, js_CFunction func);
    
    // Get/Set JavaScript variables
    void setGlobalString(const std::string& name, const std::string& value);
    void setGlobalNumber(const std::string& name, double value);
    std::string getGlobalString(const std::string& name);
    double getGlobalNumber(const std::string& name);
    
    // Error handling
    std::string getLastError() const { return lastError; }
    
    js_State* getState() { return J; }
    WebView* getWebView() { return webView; }
    
private:
    js_State* J;
    WebView* webView;
    std::string lastError;
    
    // Error reporting callback
    static void reportError(js_State* J, const char* message);
    
    // Basic console.log implementation
    static void console_log(js_State* J);
};

#endif // JSENGINE_HPP
