#ifndef MUJS_ENGINE_HPP
#define MUJS_ENGINE_HPP

#include "../libs/mujs/mujs.h"
#include "JSEngine.hpp"
#include <unordered_map>

// Forward declaration
class MuJSEngine;

// Structure to store function data for MuJS callbacks
struct MuJSFunctionData
{
	MuJSEngine* engine;
	int index;

	MuJSFunctionData(MuJSEngine* eng, int idx)
		: engine(eng)
		, index(idx)
	{
	}
};

class MuJSContext : public JSContext
{
public:
	MuJSContext(js_State* state)
		: state(state)
	{
	}
	js_State* getState() { return state; }

private:
	js_State* state;
};

class MuJSEngine : public JSEngine
{
public:
	MuJSEngine(WebView* webView);
	virtual ~MuJSEngine();

	// Core execution
	bool executeScript(const std::string& script) override;

	// High-level function registration
	void registerGlobalFunction(const std::string& name, JSFunction func) override;

	// High-level global value management
	void setGlobalString(const std::string& name, const std::string& value) override;
	void setGlobalNumber(const std::string& name, double value) override;
	std::string getGlobalString(const std::string& name) override;
	double getGlobalNumber(const std::string& name) override;

	// Simplified callback context
	int argCount() override;
	bool argIsString(int index) override;
	bool argIsNumber(int index) override;
	std::string getArgString(int index) override;
	double getArgNumber(int index) override;
	void setReturnString(const std::string& value) override;
	void setReturnNumber(double value) override;
	void setReturnNull() override;
	void setReturnUndefined() override;

	// JSON support (used by MainDisplay and StorageManager)
	bool parseJSON(const std::string& jsonStr) override;

	// Utility
	std::string getLastError() const override;
	WebView* getWebView() override { return webView; }

	// MuJS specific methods
	js_State* getState() { return J; }
	void setupBasicBindings();

private:
	WebView* webView;
	js_State* J;
	std::string lastError;
	std::vector<JSFunction> functionTable;
	std::unordered_map<std::string, int> functionNameToIndex;
	std::vector<void*> functionDataObjects;
	MuJSContext context;

	// Per-callback transient state
	int currentArgc = 0;
	int callbackBaseTop = 0;
	bool inCallback = false;
	int thisIndex = 0;
	bool hasReturnValue = false;

	// Function wrapper for mujs
	static void mujsFunctionWrapper(js_State* J);

	// Error reporting
	static void reportError(js_State* J, const char* message);

	// Console.log implementation
	static void console_log(js_State* J);

	// Helper to get engine instance from js_State
	static MuJSEngine* getEngineFromState(js_State* J);
};

#endif // MUJS_ENGINE_HPP
