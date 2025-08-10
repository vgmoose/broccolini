#include "JSEngine.hpp"
#include "WebView.hpp"
#include <iostream>
#include <setjmp.h>

JSEngine::JSEngine(WebView* webView) : webView(webView), lastError("") {
    // uses non-strict mode to allow undeclared variable assignments
    J = js_newstate(nullptr, nullptr, 0);
    
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

bool JSEngine::parseJSON(const std::string& jsonStr) {
    if (!J) {
        lastError = "JavaScript state not initialized";
        return false;
    }
    
    try {
        // Create a properly escaped JSON string for safe script execution
        std::string escapedJson = jsonStr;
        
        // Escape backslashes first (must be done before escaping quotes)
        size_t pos = 0;
        while ((pos = escapedJson.find("\\", pos)) != std::string::npos) {
            escapedJson.replace(pos, 1, "\\\\");
            pos += 2;
        }
        
        // Escape single quotes
        pos = 0;
        while ((pos = escapedJson.find("'", pos)) != std::string::npos) {
            escapedJson.replace(pos, 1, "\\'");
            pos += 2;
        }
        
        // Escape newlines and tabs
        pos = 0;
        while ((pos = escapedJson.find("\n", pos)) != std::string::npos) {
            escapedJson.replace(pos, 1, "\\n");
            pos += 2;
        }
        
        pos = 0;
        while ((pos = escapedJson.find("\t", pos)) != std::string::npos) {
            escapedJson.replace(pos, 1, "\\t");
            pos += 2;
        }
        
        pos = 0;
        while ((pos = escapedJson.find("\r", pos)) != std::string::npos) {
            escapedJson.replace(pos, 1, "\\r");
            pos += 2;
        }
        
        std::string script = "var parsedJSON = JSON.parse('" + escapedJson + "');";
        
        int result = js_dostring(J, script.c_str());
        if (result != 0) {
            lastError = "JSON.parse script execution failed with code: " + std::to_string(result);
            return false;
        }
        
        std::cout << "JSON.parse executed successfully!" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        lastError = std::string("Exception in parseJSON: ") + e.what();
        return false;
    } catch (...) {
        lastError = "Unknown exception in parseJSON";
        return false;
    }
}

std::vector<std::string> JSEngine::getArrayFromGlobal(const std::string& globalName, const std::string& arrayPath) {
    std::vector<std::string> result;
    if (!J) {
        lastError = "JavaScript state not initialized";
        return result;
    }
    
    try {
        // Get the global object
        js_getglobal(J, globalName.c_str());
        if (js_isundefined(J, -1)) {
            lastError = "Global object '" + globalName + "' is undefined";
            js_pop(J, 1);
            return result;
        }
        
        // Navigate to the array property
        js_getproperty(J, -1, arrayPath.c_str());
        if (!js_isarray(J, -1)) {
            lastError = "Property '" + arrayPath + "' is not an array";
            js_pop(J, 2);
            return result;
        }
        
        // Get array length
        int length = js_getlength(J, -1);
        
        // Extract each array element
        for (int i = 0; i < length; i++) {
            js_getindex(J, -1, i);
            if (js_isstring(J, -1)) {
                result.push_back(js_tostring(J, -1));
            }
            js_pop(J, 1);
        }
        
        js_pop(J, 2); // pop array and global object
        return result;
    } catch (const std::exception& e) {
        lastError = std::string("Exception in getArrayFromGlobal: ") + e.what();
        return result;
    } catch (...) {
        lastError = "Unknown exception in getArrayFromGlobal";
        return result;
    }
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

std::string JSEngine::stringifyObject(const std::map<std::string, std::string>& stringMap) {
    if (!J) return "{}";
    
    // Clear the stack
    while (js_gettop(J) > 0) js_pop(J, 1);
    
    // Create object
    js_newobject(J);
    
    for (const auto& [key, value] : stringMap) {
        js_pushstring(J, value.c_str());
        js_setproperty(J, -2, key.c_str());
    }
    
    // Stringify the object
    js_getglobal(J, "JSON");
    js_getproperty(J, -1, "stringify");
    js_copy(J, -3); // copy object
    js_call(J, 1);
    
    std::string result = js_tostring(J, -1);
    
    // Clean up
    while (js_gettop(J) > 0) js_pop(J, 1);
    
    return result;
}

std::string JSEngine::stringifyArray(const std::vector<std::string>& stringArray) {
    if (!J) return "[]";
    
    // Clear the stack
    while (js_gettop(J) > 0) js_pop(J, 1);
    
    // Create array
    js_newarray(J);
    
    for (size_t i = 0; i < stringArray.size(); i++) {
        js_pushstring(J, stringArray[i].c_str());
        js_setindex(J, -2, i);
    }
    
    // Stringify the array
    js_getglobal(J, "JSON");
    js_getproperty(J, -1, "stringify");
    js_copy(J, -3); // copy array
    js_call(J, 1);
    
    std::string result = js_tostring(J, -1);
    
    // Clean up
    while (js_gettop(J) > 0) js_pop(J, 1);
    
    return result;
}
