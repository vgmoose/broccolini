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
    
    // Check if a global function exists
    bool globalFunctionExists(const std::string& name);
    
    // JSON parsing utilities
    bool parseJSON(const std::string& jsonStr);
    std::vector<std::string> getArrayFromGlobal(const std::string& globalName, const std::string& arrayPath);
    
    // JSON stringification utilities
    std::string stringifyObject(const std::map<std::string, std::string>& stringMap);
    std::string stringifyArray(const std::vector<std::string>& stringArray);
    
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
