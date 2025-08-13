#ifndef QUICKJS_ENGINE_HPP
#define QUICKJS_ENGINE_HPP

#include "../libs/quickjs/quickjs.h"
#include "JSEngine.hpp"

class QuickJSContext : public JSContext
{
public:
	QuickJSContext(JSContext* ctx)
		: ctx(ctx)
	{
	}
	JSContext* getContext() { return ctx; }

private:
	JSContext* ctx;
};

class QuickJSEngine : public JSEngine
{
public:
	QuickJSEngine(WebView* webView);
	virtual ~QuickJSEngine();

	// Core execution
	bool executeScript(const std::string& script) override;
	
	// Execution control - interruption support
	void enableInterruptHandler() override;
	void disableInterruptHandler() override;

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
	void throwError(const std::string& message) override;

	// QuickJS specific methods
	JSContext* getContext() { return ctx; }
	void setupBasicBindings();

private:
	WebView* webView;
	JSRuntime* rt;
	JSContext* ctx;
	std::string lastError;
	QuickJSContext context;
	std::vector<JSFunction> functionTable;
	void* userData;

	// Per-callback transient state
	bool inCallback = false;
	int currentArgc = 0;
	std::vector<JSValue> callbackArgs;
	JSValue callbackThis = JS_UNDEFINED;
	JSValue returnValue = JS_UNDEFINED;
	bool hasReturnValue = false;

	// Function dispatcher
	static JSValue quickjsFunctionDispatcher(JSContext* ctx,
		JSValueConst this_val, int argc,
		JSValueConst* argv, int magic);

	// Helper to get engine instance from JSContext
	static QuickJSEngine* getEngineFromContext(JSContext* ctx);

	// Console.log implementation
	static JSValue console_log(JSContext* ctx, JSValueConst this_val, int argc,
		JSValueConst* argv);
	
	// Interrupt handler for execution control
	static int interruptHandler(JSRuntime* rt, void* opaque);
};

#endif // QUICKJS_ENGINE_HPP
