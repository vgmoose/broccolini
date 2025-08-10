#pragma once

#include <string>
#include <map>
#include <chrono>
#include <memory>

class JSEngine;

// Centralized storage manager that handles cookie and localStorage for all tabs
class StorageManager {
private:
    // Cookie data structure
    struct CookieData {
        std::string value;
        std::chrono::system_clock::time_point expires;
        std::string domain;
        std::string path;
        bool httpOnly;
        bool secure;
        
        CookieData() : expires(std::chrono::system_clock::time_point::max()), 
                      domain(""), path("/"), httpOnly(false), secure(false) {}
    };
    
    // Storage maps keyed by domain
    std::map<std::string, std::map<std::string, CookieData>> domainCookies;
    std::map<std::string, std::map<std::string, std::string>> domainLocalStorage;
    
    // Current domain for operations
    std::string currentDomain;
    
    // JSEngine instance for JSON operations
    std::unique_ptr<JSEngine> jsonEngine;
    
    // Helper methods
    void loadDomainCookies(const std::string& domain);
    void saveDomainCookies(const std::string& domain);
    void loadDomainLocalStorage(const std::string& domain);
    void saveDomainLocalStorage(const std::string& domain);
    
    bool isExpiredCookie(const CookieData& cookie);
    void cleanupExpiredCookies(const std::string& domain);
    
    std::string getDomainDataPath(const std::string& domain);
    void ensureDomainDataDir(const std::string& domain);
    std::string sanitizeDomainName(const std::string& domain);
    
public:
    StorageManager();
    ~StorageManager();
    
    // Domain management
    void setCurrentDomain(const std::string& domain);
    std::string getCurrentDomain() const { return currentDomain; }
    
    // Cookie operations
    void setCookie(const std::string& name, const std::string& value);
    void setCookie(const std::string& name, const std::string& value, 
                   const std::string& domain, const std::string& path,
                   int maxAge = -1, bool httpOnly = false, bool secure = false);
    std::string getCookie(const std::string& name);
    void removeCookie(const std::string& name);
    void clearAllCookies();
    std::string getAllCookiesAsString();
    void setCookieFromString(const std::string& cookieString);
    
    // localStorage operations
    void setLocalStorageItem(const std::string& key, const std::string& value);
    std::string getLocalStorageItem(const std::string& key);
    void removeLocalStorageItem(const std::string& key);
    void clearLocalStorage();
    int getLocalStorageLength();
    std::string getLocalStorageKey(int index);
    bool hasLocalStorageItem(const std::string& key);
    
    // Get the singleton instance
    static StorageManager& getInstance();
};
