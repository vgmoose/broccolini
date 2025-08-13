#ifndef JSENGINE_HPP
#define JSENGINE_HPP

#include <functional>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <chrono>

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
	
	// Core execution
	virtual bool executeScript(const std::string& script) = 0;
	
	// Execution control - interruption support
	virtual void setExecutionInterrupted(bool interrupted) { executionInterrupted = interrupted; }
	virtual bool isExecutionInterrupted() const { return executionInterrupted; }
	virtual void enableInterruptHandler() {} // Override in engines that support it
	virtual void disableInterruptHandler() {} // Override in engines that support it
	
	// Execution time management
	virtual void startExecutionTimer() { executionStartTime = std::chrono::steady_clock::now(); }
	virtual void resetExecutionTimer() { executionStartTime = std::chrono::steady_clock::now(); }
	virtual double getExecutionTimeMs() const {
		auto now = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - executionStartTime);
		return duration.count();
	}
	virtual bool shouldInterruptForTime(double maxMs = 100.0) const {
		return getExecutionTimeMs() > maxMs;
	}	// Simple function registration for C++ callbacks
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
	virtual void throwError(const std::string& message) = 0; // Throw error from C++ to stop JS execution

protected:
	JSEngine() = default;
	bool executionInterrupted = false; // Flag for execution interruption
	std::chrono::steady_clock::time_point executionStartTime = std::chrono::steady_clock::now();
};

#endif // JSENGINE_HPP
