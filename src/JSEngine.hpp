#ifndef JSENGINE_HPP
#define JSENGINE_HPP

#include <functional>
#include <string>
#include <map>
#include <vector>

class WebView;

// Forward declaration for callback context
class JSContext
{
public:
	virtual ~JSContext() = default;
};

using JSFunction = std::function<void()>;

// Simplified JavaScript engine interface
// High-level interface that works with JavaScript's natural object model
class JSEngine
{
public:
	using JSFunction = std::function<void()>;
	
	// Factory method - creates the appropriate engine based on compile flags
	static std::unique_ptr<JSEngine> create(WebView* webView);
	
	virtual ~JSEngine() = default;

	// Core JavaScript execution
	virtual bool executeScript(const std::string& script) = 0;

	// Simple function registration for C++ callbacks
	virtual void registerGlobalFunction(const std::string& name, JSFunction func) = 0;

	// High-level value access (no stack manipulation needed)
	virtual void setGlobalString(const std::string& name, const std::string& value) = 0;
	virtual void setGlobalNumber(const std::string& name, double value) = 0;
	virtual std::string getGlobalString(const std::string& name) = 0;
	virtual double getGlobalNumber(const std::string& name) = 0;

	// Function callback context (for when C++ functions are called from JS)
	virtual int argCount() = 0;
	virtual bool argIsString(int index) = 0;
	virtual bool argIsNumber(int index) = 0;
	virtual std::string getArgString(int index) = 0;
	virtual double getArgNumber(int index) = 0;
	virtual void setReturnString(const std::string& value) = 0;
	virtual void setReturnNumber(double value) = 0;
	virtual void setReturnNull() = 0;
	virtual void setReturnUndefined() = 0;

	// JSON parsing utilities (for StorageManager)
	virtual bool parseJSON(const std::string& jsonStr) = 0;

	// Error handling and context
	virtual std::string getLastError() const = 0;
	virtual WebView* getWebView() = 0;

protected:
	JSEngine() = default;
};

#endif // JSENGINE_HPP
