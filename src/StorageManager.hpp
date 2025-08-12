#pragma once

#include "JSEngine.hpp"
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct CookieData
{
	std::string value;
	std::string domain;
	std::string path;
	bool httpOnly { false };
	bool secure { false };
	std::chrono::system_clock::time_point expires = std::chrono::system_clock::time_point::max();
};

class StorageManager
{
public:
	StorageManager();
	~StorageManager();

	static StorageManager& getInstance();

	void setCurrentDomain(const std::string& domain);
	std::string getCurrentDomain() const { return currentDomain; }

	// Cookie operations (current domain)
	void setCookie(const std::string& name, const std::string& value);
	void setCookie(const std::string& name, const std::string& value,
		const std::string& domain, const std::string& path, int maxAge,
		bool httpOnly, bool secure);
	std::string getCookie(const std::string& name);
	void removeCookie(const std::string& name);
	void clearAllCookies();
	std::string getAllCookiesAsString();
	void setCookieFromString(const std::string& cookieString);

	// localStorage operations (current domain)
	void setLocalStorageItem(const std::string& key, const std::string& value);
	std::string getLocalStorageItem(const std::string& key);
	void removeLocalStorageItem(const std::string& key);
	void clearLocalStorage();
	int getLocalStorageLength();
	std::string getLocalStorageKey(int index);
	bool hasLocalStorageItem(const std::string& key);

private:
	std::string currentDomain;
	std::map<std::string, std::map<std::string, CookieData>> domainCookies;
	std::map<std::string, std::map<std::string, std::string>> domainLocalStorage;
	std::unique_ptr<JSEngine> jsonEngine;

	// helpers
	std::string getDomainDataPath(const std::string& domain);
	void ensureDomainDataDir(const std::string& domain);
	std::string sanitizeDomainName(const std::string& domain);
	std::string readFile(const std::string& path) const;
	bool writeFile(const std::string& path, const std::string& content) const;

	void loadDomainCookies(const std::string& domain);
	void saveDomainCookies(const std::string& domain);
	void loadDomainLocalStorage(const std::string& domain);
	void saveDomainLocalStorage(const std::string& domain);
	void cleanupExpiredCookies(const std::string& domain);
	bool isExpiredCookie(const CookieData& cookie);
};
