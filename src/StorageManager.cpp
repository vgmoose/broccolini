#include "StorageManager.hpp"
#include "JSEngine.hpp"
#include "../utils/Utils.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>

StorageManager::StorageManager() : currentDomain("localhost") {
    // Create a JSEngine instance for JSON operations (no WebView)
    jsonEngine = std::make_unique<JSEngine>(nullptr);
    
    // Load initial domain data
    loadDomainCookies(currentDomain);
    loadDomainLocalStorage(currentDomain);
}

StorageManager::~StorageManager() {
    // Save all domains before destruction
    // TODO: periodically flush these to prevent data loss, alongside the session data
    for (const auto& [domain, cookies] : domainCookies) {
        if (!cookies.empty()) {
            saveDomainCookies(domain);
        }
    }
    for (const auto& [domain, storage] : domainLocalStorage) {
        if (!storage.empty()) {
            saveDomainLocalStorage(domain);
        }
    }
}

StorageManager& StorageManager::getInstance() {
    static StorageManager instance;
    return instance;
}

void StorageManager::setCurrentDomain(const std::string& domain) {
    if (currentDomain == domain) return;
    
    // Save current domain data
    saveDomainCookies(currentDomain);
    saveDomainLocalStorage(currentDomain);
    
    // Switch to new domain
    currentDomain = domain.empty() ? "localhost" : domain;
    
    // Load new domain data if not already loaded
    if (domainCookies.find(currentDomain) == domainCookies.end()) {
        loadDomainCookies(currentDomain);
    }
    if (domainLocalStorage.find(currentDomain) == domainLocalStorage.end()) {
        loadDomainLocalStorage(currentDomain);
    }
    
    std::cout << "StorageManager: Switched to domain: " << currentDomain << std::endl;
}

std::string StorageManager::getDomainDataPath(const std::string& domain) {
    std::string sanitized = sanitizeDomainName(domain);
    return "./data/domains/" + sanitized;
}

void StorageManager::ensureDomainDataDir(const std::string& domain) {
    std::string domainPath = getDomainDataPath(domain);
    std::filesystem::create_directories(domainPath);
}

std::string StorageManager::sanitizeDomainName(const std::string& domain) {
    std::string sanitized = domain;
    // Replace invalid characters for file/directory names
    std::replace(sanitized.begin(), sanitized.end(), '/', '_');
    std::replace(sanitized.begin(), sanitized.end(), '\\', '_');
    std::replace(sanitized.begin(), sanitized.end(), ':', '_');
    std::replace(sanitized.begin(), sanitized.end(), '*', '_');
    std::replace(sanitized.begin(), sanitized.end(), '?', '_');
    std::replace(sanitized.begin(), sanitized.end(), '"', '_');
    std::replace(sanitized.begin(), sanitized.end(), '<', '_');
    std::replace(sanitized.begin(), sanitized.end(), '>', '_');
    std::replace(sanitized.begin(), sanitized.end(), '|', '_');
    return sanitized;
}

void StorageManager::loadDomainCookies(const std::string& domain) {
    std::string filePath = getDomainDataPath(domain) + "/cookies.json";
    std::string content = readFile(filePath);
    
    // Initialize domain if it doesn't exist
    if (domainCookies.find(domain) == domainCookies.end()) {
        domainCookies[domain] = std::map<std::string, CookieData>();
    }
    
    if (content.empty()) {
        std::cout << "No cookies file found for domain: " << domain << std::endl;
        return;
    }
    
    // Parse JSON using JSEngine
    if (!jsonEngine->parseJSON(content)) {
        std::cout << "Failed to parse cookies JSON for domain: " << domain 
                  << " - " << jsonEngine->getLastError() << std::endl;
        return;
    }
    
    // Extract cookies from parsedJSON global
    js_State* J = jsonEngine->getState();
    js_getglobal(J, "parsedJSON");
    
    if (!js_isobject(J, -1)) {
        js_pop(J, 1);
        return;
    }
    
    // Iterate through cookie properties
    js_pushiterator(J, -1, 1);
    const char* key;
    while ((key = js_nextiterator(J, -1)) != NULL) {
        std::string cookieName = key;
        
        js_getproperty(J, -2, key); // Get value from the parsedJSON object
        if (js_isstring(J, -1)) {  // value (JSON string)
            std::string cookieJsonData = js_tostring(J, -1);
                
            // Parse individual cookie data
            JSEngine cookieParser(nullptr);
            if (cookieParser.parseJSON(cookieJsonData)) {
                js_State* CJ = cookieParser.getState();
                js_getglobal(CJ, "parsedJSON");
                
                CookieData cookie;
                
                // Extract cookie properties
                js_getproperty(CJ, -1, "value");
                if (js_isstring(CJ, -1)) {
                    cookie.value = js_tostring(CJ, -1);
                }
                js_pop(CJ, 1);
                
                js_getproperty(CJ, -1, "domain");
                if (js_isstring(CJ, -1)) {
                    cookie.domain = js_tostring(CJ, -1);
                }
                js_pop(CJ, 1);
                
                js_getproperty(CJ, -1, "path");
                if (js_isstring(CJ, -1)) {
                    cookie.path = js_tostring(CJ, -1);
                }
                js_pop(CJ, 1);
                
                js_getproperty(CJ, -1, "httpOnly");
                if (js_isstring(CJ, -1)) {
                    cookie.httpOnly = (std::string(js_tostring(CJ, -1)) == "true");
                }
                js_pop(CJ, 1);
                
                js_getproperty(CJ, -1, "secure");
                if (js_isstring(CJ, -1)) {
                    cookie.secure = (std::string(js_tostring(CJ, -1)) == "true");
                }
                js_pop(CJ, 1);
                
                js_getproperty(CJ, -1, "expires");
                if (js_isstring(CJ, -1)) {
                    std::string expiresStr = js_tostring(CJ, -1);
                    if (!expiresStr.empty()) {
                        auto timeT = static_cast<std::time_t>(std::stoll(expiresStr));
                        cookie.expires = std::chrono::system_clock::from_time_t(timeT);
                    }
                }
                js_pop(CJ, 1);
                
                domainCookies[domain][cookieName] = cookie;
                js_pop(CJ, 1); // pop parsedJSON
            }
        }
        js_pop(J, 1); // pop value
    }
    js_pop(J, 2); // pop iterator and parsedJSON
    
    std::cout << "Loaded " << domainCookies[domain].size() << " cookies for domain: " << domain << std::endl;
}

void StorageManager::saveDomainCookies(const std::string& domain) {
    auto cookiesIt = domainCookies.find(domain);
    if (cookiesIt == domainCookies.end() || cookiesIt->second.empty()) {
        return; // No cookies to save
    }
    
    cleanupExpiredCookies(domain);
    ensureDomainDataDir(domain);
    
    // Build JSON object using JSEngine
    js_State* J = jsonEngine->getState();
    
    // Clear the stack
    while (js_gettop(J) > 0) js_pop(J, 1);
    
    // Create main object
    js_newobject(J);
    
    for (const auto& [name, cookie] : cookiesIt->second) {
        // Create cookie details object
        js_newobject(J);
        
        js_pushstring(J, cookie.value.c_str());
        js_setproperty(J, -2, "value");
        
        js_pushstring(J, cookie.domain.c_str());
        js_setproperty(J, -2, "domain");
        
        js_pushstring(J, cookie.path.c_str());
        js_setproperty(J, -2, "path");
        
        js_pushstring(J, cookie.httpOnly ? "true" : "false");
        js_setproperty(J, -2, "httpOnly");
        
        js_pushstring(J, cookie.secure ? "true" : "false");
        js_setproperty(J, -2, "secure");
        
        if (cookie.expires != std::chrono::system_clock::time_point::max()) {
            auto timeT = std::chrono::system_clock::to_time_t(cookie.expires);
            js_pushstring(J, std::to_string(static_cast<int64_t>(timeT)).c_str());
        } else {
            js_pushstring(J, "");
        }
        js_setproperty(J, -2, "expires");
        
        // Stringify this cookie object
        js_getglobal(J, "JSON");
        js_getproperty(J, -1, "stringify");
        js_copy(J, -3); // copy cookie object
        js_call(J, 1);
        
        // Set the stringified cookie data
        js_setproperty(J, -3, name.c_str()); // Set in main object
        js_pop(J, 1); // pop JSON object
    }
    
    // Stringify the main object
    js_getglobal(J, "JSON");
    js_getproperty(J, -1, "stringify");
    js_copy(J, -3); // copy main object
    js_call(J, 1);
    
    std::string jsonContent = js_tostring(J, -1);
    js_pop(J, 3); // cleanup
    
    std::string filePath = getDomainDataPath(domain) + "/cookies.json";
    if (writeFile(filePath, jsonContent)) {
        std::cout << "Saved " << cookiesIt->second.size() << " cookies for domain: " << domain << std::endl;
    } else {
        std::cerr << "Failed to save cookies for domain: " << domain << std::endl;
    }
}

void StorageManager::loadDomainLocalStorage(const std::string& domain) {
    std::string filePath = getDomainDataPath(domain) + "/localstorage.json";
    std::string content = readFile(filePath);
    
    // Initialize domain if it doesn't exist
    if (domainLocalStorage.find(domain) == domainLocalStorage.end()) {
        domainLocalStorage[domain] = std::map<std::string, std::string>();
    }
    
    if (content.empty()) {
        std::cout << "No localStorage file found for domain: " << domain << std::endl;
        return;
    }
    
    // Parse JSON using JSEngine
    if (!jsonEngine->parseJSON(content)) {
        std::cout << "Failed to parse localStorage JSON for domain: " << domain 
                  << " - " << jsonEngine->getLastError() << std::endl;
        return;
    }
    
    // Extract storage items from parsedJSON global
    js_State* J = jsonEngine->getState();
    js_getglobal(J, "parsedJSON");
    
    if (!js_isobject(J, -1)) {
        js_pop(J, 1);
        return;
    }
    
    // Iterate through storage properties
    js_pushiterator(J, -1, 1);
    const char* key;
    while ((key = js_nextiterator(J, -1)) != NULL) {
        js_getproperty(J, -2, key); // Get value from the parsedJSON object
        if (js_isstring(J, -1)) {
            std::string keyStr = key;
            std::string value = js_tostring(J, -1);
            domainLocalStorage[domain][keyStr] = value;
        }
        js_pop(J, 1); // pop value
    }
    js_pop(J, 2); // pop iterator and parsedJSON
    
    std::cout << "Loaded " << domainLocalStorage[domain].size() << " localStorage items for domain: " << domain << std::endl;
}

void StorageManager::saveDomainLocalStorage(const std::string& domain) {
    auto storageIt = domainLocalStorage.find(domain);
    if (storageIt == domainLocalStorage.end() || storageIt->second.empty()) {
        return; // No storage to save
    }
    
    ensureDomainDataDir(domain);
    
    // Build JSON object using JSEngine
    js_State* J = jsonEngine->getState();
    
    // Clear the stack
    while (js_gettop(J) > 0) js_pop(J, 1);
    
    // Create object
    js_newobject(J);
    
    for (const auto& [key, value] : storageIt->second) {
        js_pushstring(J, value.c_str());
        js_setproperty(J, -2, key.c_str());
    }
    
    // Stringify the object
    js_getglobal(J, "JSON");
    js_getproperty(J, -1, "stringify");
    js_copy(J, -3); // copy object
    js_call(J, 1);
    
    std::string jsonContent = js_tostring(J, -1);
    js_pop(J, 3); // cleanup
    
    std::string filePath = getDomainDataPath(domain) + "/localstorage.json";
    if (writeFile(filePath, jsonContent)) {
        std::cout << "Saved " << storageIt->second.size() << " localStorage items for domain: " << domain << std::endl;
    } else {
        std::cerr << "Failed to save localStorage for domain: " << domain << std::endl;
    }
}

bool StorageManager::isExpiredCookie(const CookieData& cookie) {
    return cookie.expires != std::chrono::system_clock::time_point::max() && 
           std::chrono::system_clock::now() > cookie.expires;
}

void StorageManager::cleanupExpiredCookies(const std::string& domain) {
    auto cookiesIt = domainCookies.find(domain);
    if (cookiesIt == domainCookies.end()) return;
    
    auto& cookies = cookiesIt->second;
    auto it = cookies.begin();
    while (it != cookies.end()) {
        if (isExpiredCookie(it->second)) {
            std::cout << "Removing expired cookie: " << it->first << " for domain: " << domain << std::endl;
            it = cookies.erase(it);
        } else {
            ++it;
        }
    }
}

// Cookie operations for current domain
void StorageManager::setCookie(const std::string& name, const std::string& value) {
    CookieData cookie;
    cookie.value = value;
    cookie.domain = currentDomain;
    domainCookies[currentDomain][name] = cookie;
    std::cout << "Set cookie: " << name << " = " << value << " for domain: " << currentDomain << std::endl;
    saveDomainCookies(currentDomain);
}

void StorageManager::setCookie(const std::string& name, const std::string& value, 
                              const std::string& domain, const std::string& path,
                              int maxAge, bool httpOnly, bool secure) {
    CookieData cookie;
    cookie.value = value;
    cookie.domain = domain;
    cookie.path = path;
    cookie.httpOnly = httpOnly;
    cookie.secure = secure;
    
    if (maxAge > 0) {
        cookie.expires = std::chrono::system_clock::now() + std::chrono::seconds(maxAge);
    }
    
    domainCookies[currentDomain][name] = cookie;
    std::cout << "Set cookie with options: " << name << " = " << value << " for domain: " << currentDomain << std::endl;
    saveDomainCookies(currentDomain);
}

std::string StorageManager::getCookie(const std::string& name) {
    auto cookiesIt = domainCookies.find(currentDomain);
    if (cookiesIt == domainCookies.end()) return "";
    
    auto it = cookiesIt->second.find(name);
    if (it != cookiesIt->second.end() && !isExpiredCookie(it->second)) {
        return it->second.value;
    }
    return "";
}

void StorageManager::removeCookie(const std::string& name) {
    auto cookiesIt = domainCookies.find(currentDomain);
    if (cookiesIt == domainCookies.end()) return;
    
    auto it = cookiesIt->second.find(name);
    if (it != cookiesIt->second.end()) {
        cookiesIt->second.erase(it);
        std::cout << "Removed cookie: " << name << " for domain: " << currentDomain << std::endl;
        saveDomainCookies(currentDomain);
    }
}

void StorageManager::clearAllCookies() {
    auto cookiesIt = domainCookies.find(currentDomain);
    if (cookiesIt != domainCookies.end()) {
        cookiesIt->second.clear();
        std::cout << "Cleared all cookies for domain: " << currentDomain << std::endl;
        saveDomainCookies(currentDomain);
    }
}

std::string StorageManager::getAllCookiesAsString() {
    auto cookiesIt = domainCookies.find(currentDomain);
    if (cookiesIt == domainCookies.end()) return "";
    
    std::ostringstream oss;
    bool first = true;
    
    for (const auto& [name, cookie] : cookiesIt->second) {
        if (!isExpiredCookie(cookie)) {
            if (!first) oss << "; ";
            oss << name << "=" << cookie.value;
            first = false;
        }
    }
    
    return oss.str();
}

void StorageManager::setCookieFromString(const std::string& cookieString) {
    // Simple cookie parser - parse "name=value; options"
    if (cookieString.empty()) return;
    
    std::istringstream iss(cookieString);
    std::string segment;
    
    if (std::getline(iss, segment, ';')) {
        auto eq = segment.find('=');
        if (eq != std::string::npos) {
            std::string name = segment.substr(0, eq);
            std::string value = segment.substr(eq + 1);
            
            // Trim whitespace
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            setCookie(name, value);
        }
    }
}

// localStorage operations for current domain
void StorageManager::setLocalStorageItem(const std::string& key, const std::string& value) {
    domainLocalStorage[currentDomain][key] = value;
    std::cout << "localStorage.setItem: " << key << " = " << value << " for domain: " << currentDomain << std::endl;
    saveDomainLocalStorage(currentDomain);
}

std::string StorageManager::getLocalStorageItem(const std::string& key) {
    auto storageIt = domainLocalStorage.find(currentDomain);
    if (storageIt == domainLocalStorage.end()) return "";
    
    auto it = storageIt->second.find(key);
    if (it != storageIt->second.end()) {
        return it->second;
    }
    return "";
}

void StorageManager::removeLocalStorageItem(const std::string& key) {
    auto storageIt = domainLocalStorage.find(currentDomain);
    if (storageIt == domainLocalStorage.end()) return;
    
    auto it = storageIt->second.find(key);
    if (it != storageIt->second.end()) {
        storageIt->second.erase(it);
        std::cout << "localStorage.removeItem: " << key << " for domain: " << currentDomain << std::endl;
        saveDomainLocalStorage(currentDomain);
    }
}

void StorageManager::clearLocalStorage() {
    auto storageIt = domainLocalStorage.find(currentDomain);
    if (storageIt != domainLocalStorage.end()) {
        storageIt->second.clear();
        std::cout << "localStorage.clear for domain: " << currentDomain << std::endl;
        saveDomainLocalStorage(currentDomain);
    }
}

int StorageManager::getLocalStorageLength() {
    auto storageIt = domainLocalStorage.find(currentDomain);
    if (storageIt != domainLocalStorage.end()) {
        return static_cast<int>(storageIt->second.size());
    }
    return 0;
}

std::string StorageManager::getLocalStorageKey(int index) {
    auto storageIt = domainLocalStorage.find(currentDomain);
    if (storageIt == domainLocalStorage.end() || index < 0) return "";
    
    if (index >= static_cast<int>(storageIt->second.size())) return "";
    
    // Convert map to vector for indexed access
    std::vector<std::string> keys;
    for (const auto& [key, value] : storageIt->second) {
        keys.push_back(key);
    }
    
    return keys[index];
}

bool StorageManager::hasLocalStorageItem(const std::string& key) {
    auto storageIt = domainLocalStorage.find(currentDomain);
    if (storageIt != domainLocalStorage.end()) {
        return storageIt->second.find(key) != storageIt->second.end();
    }
    return false;
}
