#include "VirtualDOM.hpp"
#include "../utils/BrocContainer.hpp"
#include "JSEngine.hpp"
#include "StorageManager.hpp"
#include "WebView.hpp"
#include <iostream>
#include <fstream>
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
	
	// setup DOM bindings to ensure window/document are available
	setupDOMBindings();
	
	// Load Snabbdom bundle on top
	if (!loadSnabbdomBundle()) {
		std::cerr << "[VirtualDOM] Failed to load Snabbdom bundle" << std::endl;
		return false;
	}
	
	return true;
}

bool VirtualDOM::loadSnabbdomBundle()
{
	// Load the compiled Snabbdom bundle from resources
	std::string snabbdomPath = RAMFS "/res/snabbdom.js";
	
	// Try to read the Snabbdom bundle file
	std::ifstream file(snabbdomPath);
	if (!file.is_open()) {
		std::cerr << "[VirtualDOM] Could not open Snabbdom bundle: " << snabbdomPath << std::endl;
		return false;
	}
	
	std::string snabbdomCode((std::istreambuf_iterator<char>(file)),
							std::istreambuf_iterator<char>());
	file.close();
	
	if (snabbdomCode.empty()) {
		std::cerr << "[VirtualDOM] Snabbdom bundle is empty" << std::endl;
		return false;
	}
	
	std::cout << "[VirtualDOM] Loading Snabbdom bundle (" << snabbdomCode.length() << " chars)" << std::endl;
	
	// First set up AMD module system for the bundle
	std::string amdSetup = R"(
		// Add missing browser APIs for MuJS compatibility
		if (typeof setTimeout === 'undefined') {
			window.setTimeout = function(callback, delay) {
				// Immediate execution for MuJS compatibility
				if (typeof callback === 'function') {
					callback();
				}
				return 1; // Return a dummy timer ID
			};
		}
		
		if (typeof clearTimeout === 'undefined') {
			window.clearTimeout = function(id) {
				// No-op for MuJS compatibility
			};
		}
		
		// Simple AMD loader for Snabbdom
		if (typeof define === 'undefined') {
			var modules = {};
			var exports = {};
			
			var define = function(deps, factory) {
				// Handle rollup-style AMD define
				if (typeof deps === 'function') {
					// Anonymous define with just factory
					factory = deps;
					deps = ['exports'];
				}
				
				var args = [];
				
				// Resolve dependencies
				if (Array.isArray(deps)) {
					for (var i = 0; i < deps.length; i++) {
						var dep = deps[i];
						if (dep === 'require') {
							args.push(function(name) { return modules[name] ? modules[name].exports : {}; });
						} else if (dep === 'exports') {
							args.push(exports);
						} else if (dep === 'module') {
							args.push({ exports: exports });
						} else {
							args.push(modules[dep] ? modules[dep].exports : {});
						}
					}
				}
				
				if (typeof factory === 'function') {
					var result = factory.apply(null, args);
					if (result) {
						exports = result;
					}
				}
				
				// Store the exports globally for access
				window.snabbdomExports = exports;
				modules['snabbdom'] = { exports: exports };
			};
			
			// Make define AMD-compatible
			define.amd = true;
			
			// Make define global
			this.define = define;
		}
	)";
	
	// Execute AMD setup first
	if (!engine->executeScript(amdSetup)) {
		std::cerr << "[VirtualDOM] Failed to set up AMD loader" << std::endl;
		return false;
	}
	
	// Then load the Snabbdom bundle
	if (!engine->executeScript(snabbdomCode)) {
		std::cerr << "[VirtualDOM] Failed to execute Snabbdom bundle" << std::endl;
		return false;
	}
	
	// Initialize Snabbdom with common modules
	std::string snabbdomInit = R"(
		// Get the Snabbdom exports from the AMD module
		var snabbdom = window.snabbdomExports || {};
		
		console.log('[VirtualDOM] Available Snabbdom exports:', Object.keys(snabbdom));
		
		// Initialize patch function with common modules
		if (snabbdom.init && snabbdom.classModule && snabbdom.propsModule && snabbdom.styleModule && snabbdom.eventListenersModule) {
			window.snabbdomPatch = snabbdom.init([
				snabbdom.classModule,
				snabbdom.propsModule, 
				snabbdom.styleModule,
				snabbdom.eventListenersModule
			]);
			
			// Export h function and other utilities
			window.h = snabbdom.h;
			window.snabbdom = snabbdom;
			
			console.log('[VirtualDOM] Snabbdom initialized successfully');
		} else {
			console.log('[VirtualDOM] Failed to find Snabbdom modules, available keys:', Object.keys(snabbdom));
			console.log('[VirtualDOM] init available:', !!snabbdom.init);
			console.log('[VirtualDOM] classModule available:', !!snabbdom.classModule);
			console.log('[VirtualDOM] propsModule available:', !!snabbdom.propsModule);
		}
	)";
	
	if (!engine->executeScript(snabbdomInit)) {
		std::cerr << "[VirtualDOM] Failed to initialize Snabbdom" << std::endl;
		return false;
	}
	
	std::cout << "[VirtualDOM] Snabbdom loaded and initialized successfully" << std::endl;
	return true;
}

void VirtualDOM::setupDOMBindings()
{
	if (!engine)
		return;
	
	// 1st, register C++ callbacks that JS can call
	registerCppCallbacks();
	
	// 2nd, create DOM objects using pure JavaScript _befoore_ loading Snabbdom
	createDOMWithJavaScript();
	
	// 3rd, add window/document bootstrapping
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
			"  if(!window.parent) window.parent = window;\n"
			"  if(!window.parent.location) window.parent.location = window.location;\n"
			"})();");
	}

	
	// Verify that DOM objects are available
	std::string verifyScript = R"(
		if (typeof window === 'undefined') {
			console.log('[VirtualDOM] ERROR: window object not available after setup');
		} else {
			console.log('[VirtualDOM] ✓ window object available');
		}
		
		if (typeof document === 'undefined') {
			console.log('[VirtualDOM] ERROR: document object not available after setup');
		} else {
			console.log('[VirtualDOM] ✓ document object available');
		}
		
		if (!window.parent || !window.parent.location) {
			console.log('[VirtualDOM] ERROR: window.parent.location not available');
		} else {
			console.log('[VirtualDOM] ✓ window.parent.location available');
		}
	)";
	
	engine->executeScript(verifyScript);
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
	
	engine->registerGlobalFunction("__updateElementHTML", [this]() {
		std::cout << "[VirtualDOM] __updateElementHTML called with " << engine->argCount() << " arguments" << std::endl;
		if (engine->argCount() >= 2 && engine->argIsString(0) && engine->argIsString(1)) {
			std::string elementId = engine->getArgString(0);
			std::string newHTML = engine->getArgString(1);
			std::cout << "[VirtualDOM] __updateElementHTML: " << elementId << " -> " << newHTML << std::endl;
			updateElementInnerHTML(elementId, newHTML);
			if (webView) webView->needsRender = true;
		} else {
			std::cout << "[VirtualDOM] __updateElementHTML: Invalid arguments" << std::endl;
		}
	});
	
	engine->registerGlobalFunction("__getElementById", [this]() {
		std::cout << "[VirtualDOM] __getElementById called with " << engine->argCount() << " arguments" << std::endl;
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string id = engine->getArgString(0);
			std::cout << "[VirtualDOM] __getElementById: looking for '" << id << "'" << std::endl;
			auto element = findElementByIdInLiteHTML(id);
			if (element) {
				std::string tagName = element->get_tagName();
				std::cout << "[VirtualDOM] __getElementById: found element '" << id << "' with tag '" << tagName << "'" << std::endl;
				engine->setReturnString(tagName);
			} else {
				std::cout << "[VirtualDOM] __getElementById: element '" << id << "' not found" << std::endl;
				engine->setReturnNull();
			}
		} else {
			std::cout << "[VirtualDOM] __getElementById: Invalid arguments" << std::endl;
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
	
	// Register batch update function for Snabbdom integration
	engine->registerGlobalFunction("__processBatchUpdates", [this]() {
		std::cout << "[VirtualDOM] __processBatchUpdates called with " << engine->argCount() << " arguments" << std::endl;
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string updatesJson = engine->getArgString(0);
			std::cout << "[VirtualDOM] __processBatchUpdates argument: '" << updatesJson << "'" << std::endl;
			processBatchUpdates(updatesJson);
			if (webView) webView->needsRender = true;
		} else {
			std::cout << "[VirtualDOM] __processBatchUpdates: Invalid arguments" << std::endl;
		}
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
	
	// Virtual DOM specific callbacks
	engine->registerGlobalFunction("__batchUpdateElements", [this]() {
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			// Expect JSON string with array of updates
			std::string updatesJson = engine->getArgString(0);
			processBatchUpdates(updatesJson);
			if (webView) webView->needsRender = true;
		}
	});
}

void VirtualDOM::createDOMWithJavaScript()
{
	// Create a basic DOM API with virtual DOM foundation
	std::string domScript = R"(
		console.log("[VDOM] Starting DOM creation");
		
		// Simple createElement function
		function createElement(tagName) {
			console.log('[createElement] Creating element with tag:', tagName);
			var element = {
				tagName: tagName,
				id: "",
				_textContent: "",
				_innerHTML: ""
			};
			
			// textContent property with getter/setter
			Object.defineProperty(element, 'textContent', {
				get: function() {
					return this._textContent || "";
				},
				set: function(value) {
					this._textContent = value;
					if (this.id) {
						__updateTextContent(this.id, value);
					}
				},
				configurable: true,
				enumerable: true
			});
			
			// innerHTML property with getter/setter
			Object.defineProperty(element, 'innerHTML', {
				get: function() {
					console.log('[createElement innerHTML getter] Called for element:', this.tagName, 'id:', this.id);
					return this._innerHTML || "";
				},
				set: function(value) {
					console.log('[createElement innerHTML setter] Called for element:', this.tagName, 'id:', this.id, 'with value:', value);
					this._innerHTML = value;
					if (this.id) {
						console.log('[createElement innerHTML setter] About to call __updateElementHTML with:', this.id, value);
						__updateElementHTML(this.id, value);
					}
				},
				configurable: true,
				enumerable: true
			});
			
			return element;
		}
		
		// Simple getElementById function
		function getElementById(id) {
			console.log('[getElementById] Looking for element with id:', id);
			console.log('[getElementById] About to call __getElementById');
			var tagName = __getElementById(id);
			console.log('[getElementById] __getElementById returned:', tagName);
			if (tagName) {
				console.log('[getElementById] Found element with tag:', tagName);
				var element = createElement(tagName);
				element.id = id;
				
				// Only add innerHTML if it doesn't already exist
				if (!element.hasOwnProperty('innerHTML')) {
					// Add innerHTML property that calls back to C++
					Object.defineProperty(element, 'innerHTML', {
						get: function() {
							console.log('[innerHTML getter] Called for element:', this.id);
							return this._innerHTML || "";
						},
						set: function(value) {
							console.log('[innerHTML setter] Called for element:', this.id, 'with value:', value);
							this._innerHTML = value;
							// Update the actual DOM through VirtualDOM
							console.log('[innerHTML setter] About to call __updateElementHTML with:', this.id, value);
							__updateElementHTML(this.id, value);
						},
						configurable: true,
						enumerable: true
					});
				}
				
				return element;
			}
			return null;
		}
		
		// Create document object
		var document = {
			createElement: createElement,
			getElementById: getElementById
		};
		
		// Create location object for navigation
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
		
		// Create window object
		var window = {
			document: document,
			location: location,
			alert: function(message) {
				__alert(message);
			}
		};
		
		// Set up window hierarchy with proper location access
		window.parent = {
			location: location,
			document: document
		};
		window.top = window;
		window.self = window;
		
		// Expose globally
		if (typeof globalThis !== 'undefined') {
			globalThis.window = window;
			globalThis.document = document;
		}
		
		console.log("[VDOM] DOM creation completed");
	)";
	
	if (!engine->executeScript(domScript)) {
		std::cout << "[VirtualDOM] Failed to execute DOM creation script" << std::endl;
	}
}

// Missing method implementations
void VirtualDOM::updateElementTextContent(const std::string& elementId, const std::string& newText)
{
	std::cout << "[VirtualDOM] updateElementTextContent: " << elementId << " -> " << newText << std::endl;
	
	// Find the element in the litehtml DOM
	auto element = findElementByIdInLiteHTML(elementId);
	if (element)
	{
		std::cout << "[VirtualDOM] Found element, updating text content" << std::endl;
		
		// Alternative approach: Modify the HTML content directly and recreate
		if (webView) {
			// Find the element in the original HTML content and replace it
			std::string& htmlContent = webView->contents;
			
			// Simple find-and-replace approach for the specific element
			// This is a bit crude but should work for basic text updates
			std::string searchPattern = "id=\"" + elementId + "\"";
			size_t elementPos = htmlContent.find(searchPattern);
			
			if (elementPos != std::string::npos) {
				// Find the opening and closing tags
				size_t tagStart = htmlContent.rfind('<', elementPos);
				size_t contentStart = htmlContent.find('>', elementPos) + 1;
				size_t contentEnd = htmlContent.find('<', contentStart);
				
				if (tagStart != std::string::npos && contentStart != std::string::npos && contentEnd != std::string::npos) {
					// Replace the content between the tags
					std::string before = htmlContent.substr(0, contentStart);
					std::string after = htmlContent.substr(contentEnd);
					htmlContent = before + newText + after;
					
					std::cout << "[VirtualDOM] Updated HTML content, triggering re-render" << std::endl;
					
					// Force document recreation to reflect the changes
					webView->recreateDocument();
					return;
				}
			}
			
			std::cout << "[VirtualDOM] Could not find element in HTML content for replacement" << std::endl;
		}
	}
	else
	{
		std::cout << "[VirtualDOM] Element not found: " << elementId << std::endl;
	}
}

litehtml::element::ptr VirtualDOM::findElementByIdInLiteHTML(const std::string& id)
{
	std::cout << "[VirtualDOM] findElementByIdInLiteHTML: " << id << std::endl;
	
	if (!webView || !webView->m_doc) {
		std::cout << "[VirtualDOM] No document available" << std::endl;
		return nullptr;
	}
	
	auto root = webView->m_doc->root();
	if (!root) {
		std::cout << "[VirtualDOM] No document root" << std::endl;
		return nullptr;
	}
	
	// Use litehtml's CSS selector to find element by ID
	std::string selector = "#" + id;
	auto elements = root->select_all(selector);
	
	if (!elements.empty()) {
		std::cout << "[VirtualDOM] Found element with ID: " << id << std::endl;
		return elements.front(); // Return the first (should be only) match
	}
	
	std::cout << "[VirtualDOM] Element not found: " << id << std::endl;
	return nullptr;
}

void VirtualDOM::updateElementInnerHTML(const std::string& elementId, const std::string& newHTML)
{
	std::cout << "[VirtualDOM] updateElementInnerHTML: " << elementId << " -> " << newHTML << std::endl;
	
	// Find the element in the litehtml DOM
	auto element = findElementByIdInLiteHTML(elementId);
	if (element)
	{
		std::cout << "[VirtualDOM] Found element, updating innerHTML" << std::endl;
		
		if (webView) {
			// Find the element in the original HTML content and replace its innerHTML
			std::string& htmlContent = webView->contents;
			
			// Find the element by its ID attribute
			std::string searchPattern = "id=\"" + elementId + "\"";
			size_t elementPos = htmlContent.find(searchPattern);
			
			if (elementPos != std::string::npos) {
				// Find the opening tag end and closing tag start
				size_t tagStart = htmlContent.rfind('<', elementPos);
				size_t contentStart = htmlContent.find('>', elementPos) + 1;
				
				// Find the matching closing tag
				if (tagStart != std::string::npos && contentStart != std::string::npos) {
					// Extract the tag name to find the matching closing tag
					size_t tagNameStart = tagStart + 1;
					size_t tagNameEnd = htmlContent.find_first_of(" >", tagNameStart);
					std::string tagName = htmlContent.substr(tagNameStart, tagNameEnd - tagNameStart);
					
					std::string closingTag = "</" + tagName + ">";
					size_t contentEnd = htmlContent.find(closingTag, contentStart);
					
					if (contentEnd != std::string::npos) {
						// Replace the content between the tags
						std::string before = htmlContent.substr(0, contentStart);
						std::string after = htmlContent.substr(contentEnd);
						htmlContent = before + newHTML + after;
						
						std::cout << "[VirtualDOM] Updated HTML content with innerHTML, triggering re-render" << std::endl;
						
						// Force document recreation to reflect the changes
						webView->recreateDocument();
						return;
					}
				}
			}
			
			std::cout << "[VirtualDOM] Could not find element in HTML content for innerHTML replacement" << std::endl;
		}
	}
	else
	{
		std::cout << "[VirtualDOM] Element not found for innerHTML update: " << elementId << std::endl;
	}
}

void VirtualDOM::processBatchUpdates(const std::string& updatesJson)
{
	std::cout << "[VirtualDOM] Processing batch updates with Snabbdom: " << updatesJson << std::endl;
	
	if (!engine) {
		std::cerr << "[VirtualDOM] No engine available for batch updates" << std::endl;
		return;
	}
	
	// Use Snabbdom to process virtual DOM updates
	std::string escapedJson = updatesJson;
	// Replace single quotes with escaped single quotes
	size_t pos = 0;
	while ((pos = escapedJson.find("'", pos)) != std::string::npos) {
		escapedJson.replace(pos, 1, "\\'");
		pos += 2;
	}
	
	std::string snabbdomUpdate = R"(
		try {
			// Parse the updates JSON
			var updates = JSON.parse(')" + escapedJson + R"(');
			
			if (!Array.isArray(updates)) {
				console.log('[VirtualDOM] Updates should be an array');
				return;
			}
			
			// Process each update using Snabbdom
			updates.forEach(function(update) {
				if (update.type === 'textContent') {
					// Find the target element
					var element = document.getElementById(update.elementId);
					if (element) {
						// Create a virtual node representing the new state
						var vnode = h(element.tagName.toLowerCase(), {}, update.value);
						
						// Apply the update directly to the DOM
						element.textContent = update.value;
						
						console.log('[VirtualDOM] Updated element', update.elementId, 'to:', update.value);
					} else {
						console.log('[VirtualDOM] Element not found:', update.elementId);
					}
				} else if (update.type === 'setAttribute') {
					var element = document.getElementById(update.elementId);
					if (element) {
						element.setAttribute(update.attribute, update.value);
						console.log('[VirtualDOM] Set attribute', update.attribute, 'on', update.elementId, 'to:', update.value);
					}
				} else if (update.type === 'addClass') {
					var element = document.getElementById(update.elementId);
					if (element) {
						element.classList.add(update.className);
						console.log('[VirtualDOM] Added class', update.className, 'to', update.elementId);
					}
				} else if (update.type === 'removeClass') {
					var element = document.getElementById(update.elementId);
					if (element) {
						element.classList.remove(update.className);
						console.log('[VirtualDOM] Removed class', update.className, 'from', update.elementId);
					}
				}
			});
			
		} catch (e) {
			console.log('[VirtualDOM] Error processing batch updates:', e.message);
		}
	)";
	
	if (!engine->executeScript(snabbdomUpdate)) {
		std::cerr << "[VirtualDOM] Failed to execute Snabbdom batch update" << std::endl;
	}
}
