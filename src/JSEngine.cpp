#include "JSEngine.hpp"

#ifdef USE_MUJS
#include "MuJSEngine.hpp"
#endif

#ifdef USE_QUICKJS
#include "QuickJSEngine.hpp"
#endif

std::unique_ptr<JSEngine> JSEngine::create(WebView* webView)
{
#ifdef USE_QUICKJS
	return std::make_unique<QuickJSEngine>(webView);
#elif defined(USE_MUJS)
	return std::make_unique<MuJSEngine>(webView);
#else
	#error "No JavaScript engine configured. Use -DUSE_MUJS or -DUSE_QUICKJS"
	return nullptr;
#endif
}
