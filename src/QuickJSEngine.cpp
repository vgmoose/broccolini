#ifdef USE_QUICKJS
#include "QuickJSEngine.hpp"
#include "WebView.hpp"
#include <iostream>

QuickJSEngine::QuickJSEngine(WebView* webView)
	: webView(webView)
	, rt(nullptr)
	, ctx(nullptr)
	, context(nullptr)
{
	rt = JS_NewRuntime();
	if (!rt)
	{
		lastError = "Failed to create QuickJS runtime";
		return;
	}

	ctx = JS_NewContext(rt);
	if (!ctx)
	{
		lastError = "Failed to create QuickJS context";
		JS_FreeRuntime(rt);
		rt = nullptr;
		return;
	}

	// Update context wrapper
	context = QuickJSContext(reinterpret_cast<JSContext*>(ctx));

	// Store reference to this engine
	JS_SetContextOpaque(ctx, this);

	// Set up basic bindings
	setupBasicBindings();
}

QuickJSEngine::~QuickJSEngine()
{
	// Clean up callback args
	for (auto& v : callbackArgs)
		JS_FreeValue(ctx, v);
	callbackArgs.clear();
	
	if (!JS_IsUndefined(callbackThis))
		JS_FreeValue(ctx, callbackThis);
	
	if (!JS_IsUndefined(returnValue))
		JS_FreeValue(ctx, returnValue);

	if (ctx)
	{
		JS_FreeContext(ctx);
		ctx = nullptr;
	}
	if (rt)
	{
		JS_FreeRuntime(rt);
		rt = nullptr;
	}
}

bool QuickJSEngine::executeScript(const std::string& script)
{
	if (!ctx)
	{
		lastError = "QuickJS context not initialized";
		return false;
	}

	lastError.clear();

	std::cout << "[QuickJSEngine] Executing script (length: " << script.length() << ")" << std::endl;

	// Start execution timer for timeout detection
	startExecutionTimer();

	JSValue result = JS_Eval(ctx, script.c_str(), script.length(), "[script]", JS_EVAL_TYPE_GLOBAL);

	if (JS_IsException(result))
	{
		JSValue exception = JS_GetException(ctx);
		const char* str = JS_ToCString(ctx, exception);
		lastError = std::string("QuickJS Error: ") + (str ? str : "Unknown error");
		JS_FreeCString(ctx, str);
		JS_FreeValue(ctx, exception);
		JS_FreeValue(ctx, result);
		std::cout << "[QuickJSEngine] Execution failed: " << lastError << std::endl;
		return false;
	}

	JS_FreeValue(ctx, result);
	std::cout << "[QuickJSEngine] Script executed successfully" << std::endl;
	return true;
}

void QuickJSEngine::enableInterruptHandler()
{
	if (rt) {
		std::cout << "[QuickJSEngine] Enabling interrupt handler for execution control" << std::endl;
		JS_SetInterruptHandler(rt, interruptHandler, this);
	}
}

void QuickJSEngine::disableInterruptHandler()
{
	if (rt) {
		std::cout << "[QuickJSEngine] Disabling interrupt handler" << std::endl;
		JS_SetInterruptHandler(rt, nullptr, nullptr);
	}
}

int QuickJSEngine::interruptHandler(JSRuntime* rt, void* opaque)
{
	QuickJSEngine* engine = static_cast<QuickJSEngine*>(opaque);
	if (engine) {
		// Check for explicit interruption (e.g., from AlertManager)
		if (engine->isExecutionInterrupted()) {
			std::cout << "[QuickJSEngine] Interrupt handler triggered - stopping execution due to explicit interrupt" << std::endl;
			return 1; // Return 1 to interrupt execution
		}
		
		// Check for time-based interruption (prevent infinite loops)
		if (engine->shouldInterruptForTime(100.0)) { // 100ms max execution time
			std::cout << "[QuickJSEngine] Interrupt handler triggered - stopping execution due to timeout (" 
					  << engine->getExecutionTimeMs() << "ms)" << std::endl;
			engine->setExecutionInterrupted(true); // Mark as interrupted for future checks
			return 1; // Return 1 to interrupt execution
		}
	}
	return 0; // Return 0 to continue execution
}

void QuickJSEngine::registerGlobalFunction(const std::string& name, JSFunction func)
{
	if (!ctx)
		return;

	int magic = (int)functionTable.size();
	functionTable.push_back(func);

	std::cout << "[QuickJSEngine] Registering function '" << name << "' with magic " << magic << std::endl;

	JSValue funcVal = JS_NewCFunctionMagic(ctx, quickjsFunctionDispatcher,
		name.c_str(), 0, JS_CFUNC_generic_magic, magic);
	
	JSValue global = JS_GetGlobalObject(ctx);
	JS_SetPropertyStr(ctx, global, name.c_str(), funcVal);
	JS_FreeValue(ctx, global);
}

// High-level global value management
void QuickJSEngine::setGlobalString(const std::string& name, const std::string& value)
{
	if (!ctx)
		return;
	JSValue global = JS_GetGlobalObject(ctx);
	JS_SetPropertyStr(ctx, global, name.c_str(), JS_NewString(ctx, value.c_str()));
	JS_FreeValue(ctx, global);
}

void QuickJSEngine::setGlobalNumber(const std::string& name, double value)
{
	if (!ctx)
		return;
	JSValue global = JS_GetGlobalObject(ctx);
	JS_SetPropertyStr(ctx, global, name.c_str(), JS_NewFloat64(ctx, value));
	JS_FreeValue(ctx, global);
}

std::string QuickJSEngine::getGlobalString(const std::string& name)
{
	if (!ctx)
		return "";
	JSValue global = JS_GetGlobalObject(ctx);
	JSValue val = JS_GetPropertyStr(ctx, global, name.c_str());
	const char* str = JS_ToCString(ctx, val);
	std::string result = str ? str : "";
	JS_FreeCString(ctx, str);
	JS_FreeValue(ctx, val);
	JS_FreeValue(ctx, global);
	return result;
}

double QuickJSEngine::getGlobalNumber(const std::string& name)
{
	if (!ctx)
		return 0.0;
	JSValue global = JS_GetGlobalObject(ctx);
	JSValue val = JS_GetPropertyStr(ctx, global, name.c_str());
	double result = 0.0;
	JS_ToFloat64(ctx, &result, val);
	JS_FreeValue(ctx, val);
	JS_FreeValue(ctx, global);
	return result;
}

// Simplified callback context
int QuickJSEngine::argCount() 
{ 
	return inCallback ? currentArgc : 0; 
}

bool QuickJSEngine::argIsString(int index)
{
	if (!inCallback || index < 0 || index >= currentArgc)
		return false;
	return JS_IsString(callbackArgs[index]);
}

bool QuickJSEngine::argIsNumber(int index)
{
	if (!inCallback || index < 0 || index >= currentArgc)
		return false;
	return JS_IsNumber(callbackArgs[index]);
}

std::string QuickJSEngine::getArgString(int index)
{
	if (!inCallback || index < 0 || index >= currentArgc)
		return "";
	const char* str = JS_ToCString(ctx, callbackArgs[index]);
	std::string result = str ? str : "";
	JS_FreeCString(ctx, str);
	return result;
}

double QuickJSEngine::getArgNumber(int index)
{
	if (!inCallback || index < 0 || index >= currentArgc)
		return 0.0;
	double result = 0.0;
	JS_ToFloat64(ctx, &result, callbackArgs[index]);
	return result;
}

void QuickJSEngine::setReturnString(const std::string& value)
{
	if (!inCallback)
		return;
	if (hasReturnValue)
		JS_FreeValue(ctx, returnValue);
	returnValue = JS_NewString(ctx, value.c_str());
	hasReturnValue = true;
}

void QuickJSEngine::setReturnNumber(double value)
{
	if (!inCallback)
		return;
	if (hasReturnValue)
		JS_FreeValue(ctx, returnValue);
	returnValue = JS_NewFloat64(ctx, value);
	hasReturnValue = true;
}

void QuickJSEngine::setReturnNull()
{
	if (!inCallback)
		return;
	if (hasReturnValue)
		JS_FreeValue(ctx, returnValue);
	returnValue = JS_NULL;
	hasReturnValue = true;
}

void QuickJSEngine::setReturnUndefined()
{
	if (!inCallback)
		return;
	if (hasReturnValue)
		JS_FreeValue(ctx, returnValue);
	returnValue = JS_UNDEFINED;
	hasReturnValue = true;
}

// JSON support (used by MainDisplay and StorageManager)
bool QuickJSEngine::parseJSON(const std::string& jsonStr)
{
	if (!ctx)
		return false;

	std::string script = "JSON.parse(`" + jsonStr + "`)";
	JSValue result = JS_Eval(ctx, script.c_str(), script.length(), "[json]", JS_EVAL_TYPE_GLOBAL);

	if (JS_IsException(result))
	{
		JS_FreeValue(ctx, result);
		return false;
	}

	JS_FreeValue(ctx, result);
	return true;
}

// Utility
std::string QuickJSEngine::getLastError() const 
{ 
	return lastError; 
}

void QuickJSEngine::throwError(const std::string& message)
{
	if (ctx) {
		// Throw an error in the current JavaScript execution context
		JS_ThrowInternalError(ctx, "%s", message.c_str());
	}
}

// QuickJS specific methods
void QuickJSEngine::setupBasicBindings()
{
	if (!ctx)
		return;

	// Register console.log
	JSValue console = JS_NewObject(ctx);
	JS_SetPropertyStr(ctx, console, "log", JS_NewCFunction(ctx, console_log, "log", 1));
	
	JSValue global = JS_GetGlobalObject(ctx);
	JS_SetPropertyStr(ctx, global, "console", console);
	JS_FreeValue(ctx, global);
}

// Static methods
JSValue QuickJSEngine::quickjsFunctionDispatcher(JSContext* ctx,
	JSValueConst this_val, int argc, JSValueConst* argv, int magic)
{
	QuickJSEngine* engine = getEngineFromContext(ctx);
	if (!engine)
		return JS_ThrowInternalError(ctx, "Engine not found");
	
	if (magic < 0 || magic >= (int)engine->functionTable.size())
		return JS_ThrowInternalError(ctx, "Invalid magic index");

	// Set up callback context
	engine->inCallback = true;
	engine->currentArgc = argc;
	engine->callbackArgs.clear();
	for (int i = 0; i < argc; i++)
		engine->callbackArgs.push_back(JS_DupValue(ctx, argv[i]));
	engine->callbackThis = JS_DupValue(ctx, this_val);
	
	if (engine->hasReturnValue)
	{
		JS_FreeValue(ctx, engine->returnValue);
		engine->hasReturnValue = false;
	}
	engine->returnValue = JS_UNDEFINED;

	// Call the function
	JSFunction func = engine->functionTable[magic];
	func();

	// Get return value
	JSValue ret = engine->hasReturnValue ? engine->returnValue : JS_UNDEFINED;
	
	// Clean up
	for (auto& v : engine->callbackArgs)
		JS_FreeValue(ctx, v);
	engine->callbackArgs.clear();
	JS_FreeValue(ctx, engine->callbackThis);
	engine->callbackThis = JS_UNDEFINED;
	engine->inCallback = false;
	engine->hasReturnValue = false;
	
	return ret;
}

QuickJSEngine* QuickJSEngine::getEngineFromContext(JSContext* ctx)
{
	return static_cast<QuickJSEngine*>(JS_GetContextOpaque(ctx));
}

JSValue QuickJSEngine::console_log(JSContext* ctx, JSValueConst this_val, 
	int argc, JSValueConst* argv)
{
	std::cout << "[JS Console] ";
	for (int i = 0; i < argc; ++i)
	{
		const char* str = JS_ToCString(ctx, argv[i]);
		std::cout << (str ? str : "[object]");
		JS_FreeCString(ctx, str);
		if (i < argc - 1)
			std::cout << " ";
	}
	std::cout << std::endl;
	return JS_UNDEFINED;
}

#endif // USE_QUICKJS
