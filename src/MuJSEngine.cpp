#ifdef USE_MUJS
#include "MuJSEngine.hpp"
#include "WebView.hpp"
#include <iostream>
#include <setjmp.h>

MuJSEngine::MuJSEngine(WebView* webView)
	: webView(webView)
	, J(nullptr)
	, lastError("")
	, context(nullptr)
{
	// Create mujs state with non-strict mode
	J = js_newstate(nullptr, nullptr, 0);

	if (!J)
	{
		lastError = "Failed to create JavaScript state";
		return;
	}

	// Update context with actual state
	context = MuJSContext(J);

	// Set error reporting callback
	js_setreport(J, reportError);

	// Store reference to this engine instance in the JavaScript context
	js_setcontext(J, this);

	// Set up basic bindings
	setupBasicBindings();
}

MuJSEngine::~MuJSEngine()
{
	// Clean up function data objects
	for (auto* funcData : functionDataObjects)
	{
		delete funcData;
	}
	functionDataObjects.clear();

	if (J)
	{
		js_freestate(J);
		J = nullptr;
	}
}

bool MuJSEngine::executeScript(const std::string& script)
{
	if (!J)
	{
		lastError = "JavaScript state not initialized";
		return false;
	}

	lastError.clear();

	std::cout << "[MuJSEngine] Executing script (length: " << script.length()
			  << ")" << std::endl;

	if (js_ploadstring(J, "[script]", script.c_str()))
	{
		lastError = std::string("Script loading error: ") + js_trystring(J, -1, "Error");
		std::cout << "[MuJSEngine] Load failed. First 120 chars: ===\n"
				  << script.substr(0, 120) << "\n===" << std::endl;

		// Count quotes for debugging
		size_t singleQuotes = 0, doubleQuotes = 0;
		for (char c : script)
		{
			if (c == '\'') singleQuotes++;
			if (c == '"') doubleQuotes++;
		}
		std::cout << "[MuJSEngine] Quote counts single=" << singleQuotes
				  << " double=" << doubleQuotes << std::endl;

		std::cout << "[MuJSEngine] Script loading failed: " << lastError
				  << std::endl;
		js_pop(J, 1);
		return false;
	}

	// IMPORTANT: replicate legacy behavior: push undefined as 'this' before pcall
	js_pushundefined(J);
	if (js_pcall(J, 0))
	{
		std::string base = js_trystring(J, -1, "Error");
		std::string name, message;
		if (js_isobject(J, -1))
		{
			js_getproperty(J, -1, "name");
			if (js_isstring(J, -1))
				name = js_trystring(J, -1, "");
			js_pop(J, 1);
			js_getproperty(J, -1, "message");
			if (js_isstring(J, -1))
				message = js_trystring(J, -1, "");
			js_pop(J, 1);
		}

		if (!name.empty() && !message.empty())
		{
			lastError = name + ": " + message;
		}
		else
		{
			lastError = "JavaScript Error: " + base;
		}

		std::cout << "[MuJSEngine] Execution failed: " << lastError << std::endl;
		js_pop(J, 1);
		return false;
	}

	std::cout << "[MuJSEngine] Script executed successfully" << std::endl;
	return true;
}

void MuJSEngine::registerGlobalFunction(const std::string& name, JSFunction func)
{
	if (!J)
		return;

	int index = (int)functionTable.size();
	functionTable.push_back(func);

	std::cout << "[MuJSEngine] Registering function '" << name << "' at index "
			  << index << std::endl;

	js_newcfunction(J, mujsFunctionWrapper, name.c_str(), 0);
	auto* funcData = new MuJSFunctionData(this, index);
	functionDataObjects.push_back(funcData);
	js_setcontext(J, funcData);
	js_setglobal(J, name.c_str());
}

// High-level global value management
void MuJSEngine::setGlobalString(const std::string& name, const std::string& value)
{
	if (!J)
		return;
	js_newstring(J, value.c_str());
	js_setglobal(J, name.c_str());
}

void MuJSEngine::setGlobalNumber(const std::string& name, double value)
{
	if (!J)
		return;
	js_newnumber(J, value);
	js_setglobal(J, name.c_str());
}

std::string MuJSEngine::getGlobalString(const std::string& name)
{
	if (!J)
		return "";
	js_getglobal(J, name.c_str());
	std::string result = js_trystring(J, -1, "");
	js_pop(J, 1);
	return result;
}

double MuJSEngine::getGlobalNumber(const std::string& name)
{
	if (!J)
		return 0.0;
	js_getglobal(J, name.c_str());
	double result = js_trynumber(J, -1, 0.0);
	js_pop(J, 1);
	return result;
}

// Simplified callback context
int MuJSEngine::argCount() 
{ 
	return inCallback ? currentArgc : 0; 
}

bool MuJSEngine::argIsString(int index)
{
	if (!inCallback || index < 0 || index >= currentArgc)
		return false;
	return js_isstring(J, 1 + index);
}

bool MuJSEngine::argIsNumber(int index)
{
	if (!inCallback || index < 0 || index >= currentArgc)
		return false;
	return js_isnumber(J, 1 + index);
}

std::string MuJSEngine::getArgString(int index)
{
	if (!inCallback || index < 0 || index >= currentArgc)
		return "";
	return js_trystring(J, 1 + index, "");
}

double MuJSEngine::getArgNumber(int index)
{
	if (!inCallback || index < 0 || index >= currentArgc)
		return 0.0;
	return js_trynumber(J, 1 + index, 0.0);
}

void MuJSEngine::setReturnString(const std::string& value)
{
	if (!inCallback)
		return;
	js_pushstring(J, value.c_str());
	hasReturnValue = true;
}

void MuJSEngine::setReturnNumber(double value)
{
	if (!inCallback)
		return;
	js_pushnumber(J, value);
	hasReturnValue = true;
}

void MuJSEngine::setReturnNull()
{
	if (!inCallback)
		return;
	js_pushnull(J);
	hasReturnValue = true;
}

void MuJSEngine::setReturnUndefined()
{
	if (!inCallback)
		return;
	js_pushundefined(J);
	hasReturnValue = true;
}

// JSON support (used by MainDisplay and StorageManager)
bool MuJSEngine::parseJSON(const std::string& jsonStr)
{
	if (!J)
		return false;

	lastError.clear();

	// Properly escape the JSON string by wrapping it in single quotes
	// and escaping any single quotes inside
	std::string escapedJson = jsonStr;
	size_t pos = 0;
	while ((pos = escapedJson.find("'", pos)) != std::string::npos) {
		escapedJson.replace(pos, 1, "\\'");
		pos += 2;
	}

	std::string script = "JSON.parse('" + escapedJson + "')";
	
	if (js_ploadstring(J, "[json]", script.c_str()))
	{
		lastError = std::string("JSON parse script loading error: ") + js_trystring(J, -1, "Error");
		js_pop(J, 1);
		return false;
	}

	js_pushundefined(J);
	if (js_pcall(J, 0))
	{
		lastError = std::string("JSON parse execution error: ") + js_trystring(J, -1, "Error");
		js_pop(J, 1);
		return false;
	}

	js_pop(J, 1); // Pop the result
	return true;
}

// Utility
std::string MuJSEngine::getLastError() const 
{ 
	return lastError; 
}

// MuJS specific methods
void MuJSEngine::setupBasicBindings()
{
	if (!J)
		return;

	// Register console.log
	js_newobject(J);
	js_newcfunction(J, console_log, "log", 1);
	js_setproperty(J, -2, "log");
	js_setglobal(J, "console");
}

// Static methods
void MuJSEngine::mujsFunctionWrapper(js_State* J)
{
	auto* funcData = static_cast<MuJSFunctionData*>(js_getcontext(J));
	if (!funcData || !funcData->engine)
		return;

	MuJSEngine* engine = funcData->engine;
	
	if (funcData->index < 0 || funcData->index >= (int)engine->functionTable.size())
	{
		std::cerr << "[MuJSEngine] Invalid function index: " << funcData->index << std::endl;
		return;
	}

	// Set up callback context
	engine->inCallback = true;
	engine->currentArgc = js_gettop(J) - 1; // subtract 1 for 'this'
	engine->callbackBaseTop = js_gettop(J);
	engine->thisIndex = 0; // 'this' is at index 0
	engine->hasReturnValue = false;

	// Call the function
	JSFunction func = engine->functionTable[funcData->index];
	func();

	// If no return value was set, return undefined
	if (!engine->hasReturnValue)
	{
		js_pushundefined(J);
	}

	engine->inCallback = false;
}

void MuJSEngine::reportError(js_State* J, const char* message)
{
	MuJSEngine* engine = getEngineFromState(J);
	if (engine)
	{
		engine->lastError = std::string("MuJS Error: ") + message;
		std::cout << "[MuJSEngine] Error: " << engine->lastError << std::endl;
	}
}

void MuJSEngine::console_log(js_State* J)
{
	std::cout << "[JS Console] ";
	int n = js_gettop(J);
	for (int i = 1; i < n; ++i)
	{
		const char* str = js_trystring(J, i, "[object]");
		std::cout << str;
		if (i < n - 1)
			std::cout << " ";
	}
	std::cout << std::endl;
}

MuJSEngine* MuJSEngine::getEngineFromState(js_State* J)
{
	void* context = js_getcontext(J);
	if (!context)
		return nullptr;
	
	// Check if this is a function data context
	auto* funcData = static_cast<MuJSFunctionData*>(context);
	if (funcData && funcData->engine)
		return funcData->engine;
	
	// Otherwise assume it's the engine directly
	return static_cast<MuJSEngine*>(context);
}

#endif // USE_MUJS
