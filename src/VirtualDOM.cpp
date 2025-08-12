#include "VirtualDOM.hpp"
#include "../utils/BrocContainer.hpp"
#include "JSEngine.hpp"
#include "StorageManager.hpp"
#include "WebView.hpp"
#include <iostream>
#include <litehtml.h>

VirtualDOM::VirtualDOM(WebView* webView)
	: webView(webView)
	, jsEngine(nullptr)
	, engine(nullptr)
{
	rootNode = std::make_shared<VNode>("html");
}

VirtualDOM::~VirtualDOM() { }

bool VirtualDOM::initializeSnabbdom()
{
	if (!webView || !webView->jsEngine)
	{
		std::cerr << "VirtualDOM: Missing jsEngine" << std::endl;
		return false;
	}
	jsEngine = webView->jsEngine.get();
	engine = jsEngine; // Direct reference now
	std::cout << "[VirtualDOM] Got engine instance: " << engine << std::endl;
	if (!engine)
		return false;
	setupDOMBindings();
	return true;
}

void VirtualDOM::setupDOMBindings()
{
	if (!engine)
		return;
	
	// Register C++ callbacks that JS can call
	registerCppCallbacks();
	
	// Create DOM objects using pure JavaScript
	createDOMWithJavaScript();
	
	// Previously injected a bootstrap IIFE; no longer needed for mujs (was
	// causing abort)
#if defined(USE_QUICKJS)
	if (jsEngine)
	{
		jsEngine->executeScript(
			"(function(){\n"
			"  if(typeof window==='undefined'){window={};}\n"
			"  if(typeof document!=='undefined'){ if(typeof "
			"window.document==='undefined') window.document=document; }\n"
			"  if(!window.parent) window.parent=window;\n"
			"  if(!window.top) window.top=window;\n"
			"  if(!window.self) window.self=window;\n"
			"  if(typeof window.alert!=='function'){ window.alert=function(msg){ "
			"console.log('[ALERT]', msg); }; if(typeof alert==='undefined') "
			"alert=window.alert; }\n"
			"  if(typeof window.setTimeout!=='function'){ "
			"window.setTimeout=function(cb,ms){ if(typeof cb==='function'){ cb(); "
			"} }; if(typeof setTimeout==='undefined') "
			"setTimeout=window.setTimeout; }\n"
			"})();");
	}
#endif
}

void VirtualDOM::registerCppCallbacks()
{
	// Register simple C++ functions that JavaScript can call
	engine->registerGlobalFunction("__updateTextContent", [this]() {
		if (engine->argCount() >= 2 && engine->argIsString(0) && engine->argIsString(1)) {
			std::string elementId = engine->getArgString(0);
			std::string newText = engine->getArgString(1);
			updateElementTextContent(elementId, newText);
			if (webView) webView->needsRender = true;
		}
	});
	
	engine->registerGlobalFunction("__getElementById", [this]() {
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string id = engine->getArgString(0);
			auto element = findElementByIdInLiteHTML(id);
			if (element) {
				engine->setReturnString(element->get_tagName());
			} else {
				engine->setReturnNull();
			}
		} else {
			engine->setReturnNull();
		}
	});
	
	engine->registerGlobalFunction("__setTitle", [this]() {
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string title = engine->getArgString(0);
			if (webView) webView->setTitle(title);
		}
	});
	
	engine->registerGlobalFunction("__getTitle", [this]() {
		engine->setReturnString(webView ? webView->windowTitle : "");
	});
	
	engine->registerGlobalFunction("__getHref", [this]() {
		engine->setReturnString(webView ? webView->url : "");
	});
	
	engine->registerGlobalFunction("__setHref", [this]() {
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string url = engine->getArgString(0);
			if (webView) webView->goTo(url);
		}
	});
	
	engine->registerGlobalFunction("__reloadPage", [this]() {
		if (webView) webView->reloadPage();
	});
	
	engine->registerGlobalFunction("__alert", [this]() {
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string message = engine->getArgString(0);
			std::cout << "[ALERT] " << message << std::endl;
		}
	});
	
	// Storage callbacks
	engine->registerGlobalFunction("__getLocalStorageItem", [this]() {
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string key = engine->getArgString(0);
			std::string value = StorageManager::getInstance().getLocalStorageItem(key);
			if (value.empty()) {
				engine->setReturnNull();
			} else {
				engine->setReturnString(value);
			}
		} else {
			engine->setReturnNull();
		}
	});
	
	engine->registerGlobalFunction("__setLocalStorageItem", [this]() {
		if (engine->argCount() >= 2 && engine->argIsString(0) && engine->argIsString(1)) {
			std::string key = engine->getArgString(0);
			std::string value = engine->getArgString(1);
			StorageManager::getInstance().setLocalStorageItem(key, value);
		}
	});
	
	engine->registerGlobalFunction("__removeLocalStorageItem", [this]() {
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string key = engine->getArgString(0);
			StorageManager::getInstance().removeLocalStorageItem(key);
		}
	});
	
	engine->registerGlobalFunction("__clearLocalStorage", [this]() {
		StorageManager::getInstance().clearLocalStorage();
	});
	
	engine->registerGlobalFunction("__getCookies", [this]() {
		engine->setReturnString(StorageManager::getInstance().getAllCookiesAsString());
	});
	
	engine->registerGlobalFunction("__setCookie", [this]() {
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string cookie = engine->getArgString(0);
			StorageManager::getInstance().setCookieFromString(cookie);
		}
	});
}

void VirtualDOM::createDOMWithJavaScript()
{
	// Create the entire DOM API using pure JavaScript with C++ callbacks
	std::string domScript = R"(
		// Element constructor that creates proper DOM-like objects
		function createElement(tagName) {
			var element = {
				tagName: tagName,
				_textContent: "",
				_innerHTML: "",
				id: "",
				_elementId: Math.random().toString(36).substr(2, 9) // unique ID for tracking
			};
			
			// appendChild method
			element.appendChild = function(child) {
				console.log("appendChild called on", this.tagName, "for child", child.tagName);
				// In a real implementation, this would manipulate the DOM tree
				return child;
			};
			
			// textContent property with getter/setter
			Object.defineProperty(element, 'textContent', {
				get: function() {
					return this._textContent;
				},
				set: function(value) {
					this._textContent = value;
					// Notify C++ to update the actual HTML
					if (this.id) {
						__updateTextContent(this.id, value);
					}
				}
			});
			
			// innerHTML property with getter/setter  
			Object.defineProperty(element, 'innerHTML', {
				get: function() {
					return this._innerHTML;
				},
				set: function(value) {
					this._innerHTML = value;
					// Notify C++ to update the actual HTML
					if (this.id) {
						__updateTextContent(this.id, value);
					}
				}
			});
			
			// innerText alias
			Object.defineProperty(element, 'innerText', {
				get: function() { return this._textContent; },
				set: function(value) { this.textContent = value; }
			});
			
			// Event methods (simplified)
			element.addEventListener = function(event, callback) {
				console.log("addEventListener:", event);
			};
			
			element.removeEventListener = function(event, callback) {
				console.log("removeEventListener:", event);
			};
			
			return element;
		}
		
		// Text node constructor
		function createTextNode(text) {
			return {
				tagName: "#text",
				textContent: text,
				nodeValue: text
			};
		}
		
		// getElementById implementation
		function getElementById(id) {
			var tagName = __getElementById(id);
			if (tagName) {
				var element = createElement(tagName);
				element.id = id;
				return element;
			}
			return null;
		}
		
		// querySelector (simplified - only supports #id)
		function querySelector(selector) {
			if (selector.startsWith('#')) {
				return getElementById(selector.substring(1));
			}
			return null;
		}
		
		// Create document object
		var document = {
			createElement: createElement,
			createTextNode: createTextNode,
			getElementById: getElementById,
			querySelector: querySelector
		};
		
		// Document title property
		Object.defineProperty(document, 'title', {
			get: function() {
				return __getTitle();
			},
			set: function(value) {
				__setTitle(value);
			}
		});
		
		// Document cookie property
		Object.defineProperty(document, 'cookie', {
			get: function() {
				return __getCookies();
			},
			set: function(value) {
				__setCookie(value);
			}
		});
		
		// Create location object
		var location = {
			reload: function() {
				__reloadPage();
			},
			replace: function(url) {
				__setHref(url);
			},
			assign: function(url) {
				__setHref(url);
			}
		};
		
		Object.defineProperty(location, 'href', {
			get: function() {
				return __getHref();
			},
			set: function(value) {
				__setHref(value);
			}
		});
		
		// Create localStorage object
		var localStorage = {
			getItem: function(key) {
				return __getLocalStorageItem(key);
			},
			setItem: function(key, value) {
				__setLocalStorageItem(key, value);
			},
			removeItem: function(key) {
				__removeLocalStorageItem(key);
			},
			clear: function() {
				__clearLocalStorage();
			},
			key: function(index) {
				// Simplified implementation
				return null;
			}
		};
		
		// Create window object
		var window = {
			document: document,
			location: location,
			localStorage: localStorage,
			alert: function(message) {
				__alert(message);
			},
			setTimeout: function(callback, delay) {
				// Immediate execution for simplicity
				if (typeof callback === 'function') {
					callback();
				}
			}
		};
		
		// Set up window hierarchy
		window.parent = window;
		window.top = window;
		window.self = window;
		
		// Expose as globals
		if (typeof globalThis !== 'undefined') {
			globalThis.window = window;
			globalThis.document = document;
			globalThis.location = location;
			globalThis.localStorage = localStorage;
			globalThis.alert = window.alert;
			globalThis.setTimeout = window.setTimeout;
		}
	)";
	
	engine->executeScript(domScript);
}

// Missing method implementations
void VirtualDOM::updateElementTextContent(const std::string& elementId, const std::string& newText)
{
	std::cout << "[VirtualDOM] updateElementTextContent: " << elementId << " -> " << newText << std::endl;
	
	// Find the element in the litehtml DOM
	auto element = findElementByIdInLiteHTML(elementId);
	if (element)
	{
		// Update the text content in litehtml
		// Note: This is a simplified implementation
		// TODO: properly update the DOM tree (snabbdom?)
		std::cout << "[VirtualDOM] Found element, updating text content" << std::endl;
	}
	else
	{
		std::cout << "[VirtualDOM] Element not found: " << elementId << std::endl;
	}
}

litehtml::element::ptr VirtualDOM::findElementByIdInLiteHTML(const std::string& id)
{
	std::cout << "[VirtualDOM] findElementByIdInLiteHTML: " << id << std::endl;
	
	// This is a placeholder implementation TODO: Traverse the litehtml document (snabbdom?)
	// and find the element with the matching ID attribute
	
	return nullptr;
}
