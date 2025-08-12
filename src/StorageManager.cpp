#include "StorageManager.hpp"
#include "../utils/Utils.hpp"
#include "JSEngine.hpp"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

StorageManager::StorageManager()
	: currentDomain("localhost")
{
#ifndef USE_MUJS
	jsonEngine = JSEngine::create(nullptr);
	loadDomainCookies(currentDomain);
	loadDomainLocalStorage(currentDomain);
#else
	std::cout << "[StorageManager] Skipping cookie/localStorage load under mujs "
				 "(stability)"
			  << std::endl;
#endif
}

// Destructor now declared in header
StorageManager::~StorageManager()
{
	for (const auto& [domain, cookies] : domainCookies)
	{
		if (!cookies.empty())
		{
			saveDomainCookies(domain);
		}
	}
	for (const auto& [domain, storage] : domainLocalStorage)
	{
		if (!storage.empty())
		{
			saveDomainLocalStorage(domain);
		}
	}
}

StorageManager& StorageManager::getInstance()
{
	static StorageManager instance;
	return instance;
}

void StorageManager::setCurrentDomain(const std::string& domain)
{
	if (currentDomain == domain)
		return;
#ifndef USE_MUJS
	saveDomainCookies(currentDomain);
	saveDomainLocalStorage(currentDomain);
#endif
	currentDomain = domain.empty() ? "localhost" : domain;
#ifndef USE_MUJS
	if (domainCookies.find(currentDomain) == domainCookies.end())
	{
		loadDomainCookies(currentDomain);
	}
	if (domainLocalStorage.find(currentDomain) == domainLocalStorage.end())
	{
		loadDomainLocalStorage(currentDomain);
	}
#else
	// Do nothing extra under mujs
#endif
	std::cout << "StorageManager: Switched to domain: " << currentDomain
			  << std::endl;
}

std::string StorageManager::getDomainDataPath(const std::string& domain)
{
	std::string sanitized = sanitizeDomainName(domain);
	return "./data/domains/" + sanitized;
}

void StorageManager::ensureDomainDataDir(const std::string& domain)
{
	std::string domainPath = getDomainDataPath(domain);
	std::filesystem::create_directories(domainPath);
}

std::string StorageManager::sanitizeDomainName(const std::string& domain)
{
	std::string sanitized = domain;
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

void StorageManager::loadDomainCookies(const std::string& domain)
{
	std::string filePath = getDomainDataPath(domain) + "/cookies.json";
	std::string content = readFile(filePath);
	if (domainCookies.find(domain) == domainCookies.end())
	{
		domainCookies[domain] = std::map<std::string, CookieData>();
	}
	if (content.empty())
	{
		std::cout << "No cookies file found for domain: " << domain << std::endl;
		return;
	}
	if (!jsonEngine->parseJSON(content))
	{
		std::cout << "Failed to parse cookies JSON for domain: " << domain << " - "
				  << jsonEngine->getLastError() << std::endl;
		return;
	}
	// Expect parsedJSON to be an object whose keys are cookie names and values
	// are JSON strings of cookie data. Use script to flatten into compact lines:
	// name|value|domain|path|httpOnly|secure|expiresEpoch
	// Use \\n escape so JS receives the two-character sequence \n, not a raw
	// newline inside single quotes
	std::string script = "var __cookiesFlatten='';try{var o=parsedJSON;for(var k in "
						 "o){if(!Object.prototype.hasOwnProperty.call(o,k))continue;var "
						 "raw=o[k];var "
						 "c={};try{c=JSON.parse(raw);}catch(e){};__cookiesFlatten+=(__"
						 "cookiesFlatten?'\\n':'')+ "
						 "[k,c.value||'',c.domain||'',c.path||'/"
						 "',c.httpOnly?'1':'0',c.secure?'1':'0',c.expires||''].join('|');}}catch("
						 "e){__cookiesFlatten='';}";
	jsonEngine->executeScript(script);
	
	// Get the result using the simplified interface
	std::string packed = jsonEngine->getGlobalString("__cookiesFlatten");
	std::istringstream iss(packed);
	std::string line;
	while (std::getline(iss, line))
	{
		if (line.empty())
			continue;
		std::vector<std::string> parts;
		size_t start = 0;
		while (start <= line.size())
		{
			size_t sep = line.find('|', start);
			if (sep == std::string::npos)
			{
				parts.push_back(line.substr(start));
				break;
			}
			parts.push_back(line.substr(start, sep - start));
			start = sep + 1;
		}
		if (parts.size() < 7)
			continue;
		CookieData cd;
		cd.value = parts[1];
		cd.domain = parts[2];
		cd.path = parts[3];
		cd.httpOnly = (parts[4] == "1");
		cd.secure = (parts[5] == "1");
		if (!parts[6].empty())
		{
			try
			{
				auto t = static_cast<std::time_t>(std::stoll(parts[6]));
				cd.expires = std::chrono::system_clock::from_time_t(t);
			}
			catch (...)
			{
			}
		}
		domainCookies[domain][parts[0]] = cd;
	}
	std::cout << "Loaded " << domainCookies[domain].size()
			  << " cookies for domain: " << domain << std::endl;
}

void StorageManager::saveDomainCookies(const std::string& domain)
{
	auto it = domainCookies.find(domain);
	if (it == domainCookies.end() || it->second.empty())
		return;
	cleanupExpiredCookies(domain);
	ensureDomainDataDir(domain);
	// Build JSON using abstraction (simple manual stringify)
	std::ostringstream oss;
	oss << "{";
	bool first = true;
	for (auto& kv : it->second)
	{
		if (isExpiredCookie(kv.second))
			continue;
		if (!first)
			oss << ",";
		first = false;
		const auto& c = kv.second;
		oss << "\"" << kv.first << "\":\"";
		std::ostringstream cjson;
		cjson << "{\\\"value\\\":\\\"" << c.value << "\\\",\\\"domain\\\":\\\""
			  << c.domain << "\\\",\\\"path\\\":\\\"" << c.path
			  << "\\\",\\\"httpOnly\\\":\\\"" << (c.httpOnly ? "true" : "false")
			  << "\\\",\\\"secure\\\":\\\"" << (c.secure ? "true" : "false")
			  << "\\\",\\\"expires\\\":\\\"";
		if (c.expires != std::chrono::system_clock::time_point::max())
		{
			auto t = std::chrono::system_clock::to_time_t(c.expires);
			cjson << (long long)t;
		}
		cjson << "\\\"}";
		oss << cjson.str() << "\"";
	}
	oss << "}";
	std::string filePath = getDomainDataPath(domain) + "/cookies.json";
	if (writeFile(filePath, oss.str()))
		std::cout << "Saved " << it->second.size()
				  << " cookies for domain: " << domain << std::endl;
	else
		std::cerr << "Failed to save cookies for domain: " << domain << std::endl;
}

void StorageManager::loadDomainLocalStorage(const std::string& domain)
{
	std::string filePath = getDomainDataPath(domain) + "/localstorage.json";
	std::string content = readFile(filePath);
	if (domainLocalStorage.find(domain) == domainLocalStorage.end())
		domainLocalStorage[domain] = {};
	if (content.empty())
	{
		std::cout << "No localStorage file found for domain: " << domain
				  << std::endl;
		return;
	}
	if (!jsonEngine->parseJSON(content))
	{
		std::cout << "Failed to parse localStorage JSON for domain: " << domain
				  << " - " << jsonEngine->getLastError() << std::endl;
		return;
	}
	std::string script = "var __lsFlatten='';try{var o=parsedJSON;for(var k in "
						 "o){if(!Object.prototype.hasOwnProperty.call(o,k))continue;__lsFlatten+=("
						 "__lsFlatten?'\\n':'')+k+'|'+o[k];}}catch(e){__lsFlatten='';}";
	jsonEngine->executeScript(script);
	std::string packed = jsonEngine->getGlobalString("__lsFlatten");
	std::istringstream iss(packed);
	std::string line;
	while (std::getline(iss, line))
	{
		if (line.empty())
			continue;
		size_t sep = line.find('|');
		if (sep == std::string::npos)
			continue;
		std::string k = line.substr(0, sep);
		std::string v = line.substr(sep + 1);
		domainLocalStorage[domain][k] = v;
	}
	std::cout << "Loaded " << domainLocalStorage[domain].size()
			  << " localStorage items for domain: " << domain << std::endl;
}

void StorageManager::saveDomainLocalStorage(const std::string& domain)
{
	auto it = domainLocalStorage.find(domain);
	if (it == domainLocalStorage.end() || it->second.empty())
		return;
	ensureDomainDataDir(domain);
	std::ostringstream oss;
	oss << "{";
	bool first = true;
	for (auto& kv : it->second)
	{
		if (!first)
			oss << ",";
		first = false;
		oss << "\"" << kv.first << "\":\"" << kv.second << "\"";
	}
	oss << "}";
	std::string filePath = getDomainDataPath(domain) + "/localstorage.json";
	if (writeFile(filePath, oss.str()))
		std::cout << "Saved " << it->second.size()
				  << " localStorage items for domain: " << domain << std::endl;
	else
		std::cerr << "Failed to save localStorage for domain: " << domain
				  << std::endl;
}

bool StorageManager::isExpiredCookie(const CookieData& cookie)
{
	return cookie.expires != std::chrono::system_clock::time_point::max() && std::chrono::system_clock::now() > cookie.expires;
}

void StorageManager::cleanupExpiredCookies(const std::string& domain)
{
	auto cookiesIt = domainCookies.find(domain);
	if (cookiesIt == domainCookies.end())
		return;

	auto& cookies = cookiesIt->second;
	auto it = cookies.begin();
	while (it != cookies.end())
	{
		if (isExpiredCookie(it->second))
		{
			std::cout << "Removing expired cookie: " << it->first
					  << " for domain: " << domain << std::endl;
			it = cookies.erase(it);
		}
		else
		{
			++it;
		}
	}
}

// Cookie operations for current domain
void StorageManager::setCookie(const std::string& name,
	const std::string& value)
{
	CookieData cookie;
	cookie.value = value;
	cookie.domain = currentDomain;
	domainCookies[currentDomain][name] = cookie;
	std::cout << "Set cookie: " << name << " = " << value
			  << " for domain: " << currentDomain << std::endl;
	saveDomainCookies(currentDomain);
}

void StorageManager::setCookie(const std::string& name,
	const std::string& value,
	const std::string& domain,
	const std::string& path, int maxAge,
	bool httpOnly, bool secure)
{
	CookieData cookie;
	cookie.value = value;
	cookie.domain = domain;
	cookie.path = path;
	cookie.httpOnly = httpOnly;
	cookie.secure = secure;

	if (maxAge > 0)
	{
		cookie.expires = std::chrono::system_clock::now() + std::chrono::seconds(maxAge);
	}

	domainCookies[currentDomain][name] = cookie;
	std::cout << "Set cookie with options: " << name << " = " << value
			  << " for domain: " << currentDomain << std::endl;
	saveDomainCookies(currentDomain);
}

std::string StorageManager::getCookie(const std::string& name)
{
	auto cookiesIt = domainCookies.find(currentDomain);
	if (cookiesIt == domainCookies.end())
		return "";

	auto it = cookiesIt->second.find(name);
	if (it != cookiesIt->second.end() && !isExpiredCookie(it->second))
	{
		return it->second.value;
	}
	return "";
}

void StorageManager::removeCookie(const std::string& name)
{
	auto cookiesIt = domainCookies.find(currentDomain);
	if (cookiesIt == domainCookies.end())
		return;

	auto it = cookiesIt->second.find(name);
	if (it != cookiesIt->second.end())
	{
		cookiesIt->second.erase(it);
		std::cout << "Removed cookie: " << name << " for domain: " << currentDomain
				  << std::endl;
		saveDomainCookies(currentDomain);
	}
}

void StorageManager::clearAllCookies()
{
	auto cookiesIt = domainCookies.find(currentDomain);
	if (cookiesIt != domainCookies.end())
	{
		cookiesIt->second.clear();
		std::cout << "Cleared all cookies for domain: " << currentDomain
				  << std::endl;
		saveDomainCookies(currentDomain);
	}
}

std::string StorageManager::getAllCookiesAsString()
{
	auto cookiesIt = domainCookies.find(currentDomain);
	if (cookiesIt == domainCookies.end())
		return "";

	std::ostringstream oss;
	bool first = true;

	for (const auto& [name, cookie] : cookiesIt->second)
	{
		if (!isExpiredCookie(cookie))
		{
			if (!first)
				oss << "; ";
			oss << name << "=" << cookie.value;
			first = false;
		}
	}

	return oss.str();
}

void StorageManager::setCookieFromString(const std::string& cookieString)
{
	// Simple cookie parser - parse "name=value; options"
	if (cookieString.empty())
		return;

	std::istringstream iss(cookieString);
	std::string segment;

	if (std::getline(iss, segment, ';'))
	{
		auto eq = segment.find('=');
		if (eq != std::string::npos)
		{
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
void StorageManager::setLocalStorageItem(const std::string& key,
	const std::string& value)
{
	domainLocalStorage[currentDomain][key] = value;
	std::cout << "localStorage.setItem: " << key << " = " << value
			  << " for domain: " << currentDomain << std::endl;
	saveDomainLocalStorage(currentDomain);
}

std::string StorageManager::getLocalStorageItem(const std::string& key)
{
	auto storageIt = domainLocalStorage.find(currentDomain);
	if (storageIt == domainLocalStorage.end())
		return "";

	auto it = storageIt->second.find(key);
	if (it != storageIt->second.end())
	{
		return it->second;
	}
	return "";
}

void StorageManager::removeLocalStorageItem(const std::string& key)
{
	auto storageIt = domainLocalStorage.find(currentDomain);
	if (storageIt == domainLocalStorage.end())
		return;

	auto it = storageIt->second.find(key);
	if (it != storageIt->second.end())
	{
		storageIt->second.erase(it);
		std::cout << "localStorage.removeItem: " << key
				  << " for domain: " << currentDomain << std::endl;
		saveDomainLocalStorage(currentDomain);
	}
}

void StorageManager::clearLocalStorage()
{
	auto storageIt = domainLocalStorage.find(currentDomain);
	if (storageIt != domainLocalStorage.end())
	{
		storageIt->second.clear();
		std::cout << "localStorage.clear for domain: " << currentDomain
				  << std::endl;
		saveDomainLocalStorage(currentDomain);
	}
}

int StorageManager::getLocalStorageLength()
{
	auto storageIt = domainLocalStorage.find(currentDomain);
	if (storageIt != domainLocalStorage.end())
	{
		return static_cast<int>(storageIt->second.size());
	}
	return 0;
}

std::string StorageManager::getLocalStorageKey(int index)
{
	auto storageIt = domainLocalStorage.find(currentDomain);
	if (storageIt == domainLocalStorage.end() || index < 0)
		return "";

	if (index >= static_cast<int>(storageIt->second.size()))
		return "";

	// Convert map to vector for indexed access
	std::vector<std::string> keys;
	for (const auto& [key, value] : storageIt->second)
	{
		keys.push_back(key);
	}

	return keys[index];
}

bool StorageManager::hasLocalStorageItem(const std::string& key)
{
	auto storageIt = domainLocalStorage.find(currentDomain);
	if (storageIt != domainLocalStorage.end())
	{
		return storageIt->second.find(key) != storageIt->second.end();
	}
	return false;
}

std::string StorageManager::readFile(const std::string& path) const
{
	std::ifstream ifs(path, std::ios::in | std::ios::binary);
	if (!ifs)
		return "";
	std::ostringstream ss;
	ss << ifs.rdbuf();
	return ss.str();
}

bool StorageManager::writeFile(const std::string& path,
	const std::string& content) const
{
	std::ofstream ofs(path, std::ios::out | std::ios::binary | std::ios::trunc);
	if (!ofs)
		return false;
	ofs << content;
	return true;
}
