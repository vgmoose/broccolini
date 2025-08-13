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
	
	// Initialize Snabbdom with custom litehtml module and common modules
	std::string snabbdomInit = R"(
		// Get the Snabbdom exports from the AMD module
		var snabbdom = window.snabbdomExports || {};
		
		console.log('[VirtualDOM] Available Snabbdom exports:', Object.keys(snabbdom));
		
		// Create custom litehtml module that bridges to C++
		var liteHTMLModule = {
			create: function(emptyVnode, vnode) {
				console.log('[LiteHTML Module] create:', vnode.sel, 'key:', vnode.key);
				if (vnode.sel && vnode.sel !== '!' && vnode.sel !== 'text') {
					var elementData = {
						tag: vnode.sel,
						key: vnode.key || '',
						props: vnode.data ? vnode.data.props || {} : {},
						attrs: vnode.data ? vnode.data.attrs || {} : {},
						class: vnode.data ? vnode.data.class || {} : {},
						style: vnode.data ? vnode.data.style || {} : {},
						text: vnode.text || ''
					};
					__createLiteHTMLElement(JSON.stringify(elementData));
				}
			},
			
			update: function(oldVnode, vnode) {
				console.log('[LiteHTML Module] update:', vnode.sel, 'key:', vnode.key);
				if (vnode.sel && vnode.sel !== '!' && vnode.sel !== 'text') {
					var elementData = {
						tag: vnode.sel,
						key: vnode.key || vnode.sel,
						props: vnode.data ? vnode.data.props || {} : {},
						attrs: vnode.data ? vnode.data.attrs || {} : {},
						class: vnode.data ? vnode.data.class || {} : {},
						style: vnode.data ? vnode.data.style || {} : {},
						text: vnode.text || '',
						oldText: oldVnode.text || ''
					};
					__updateLiteHTMLElement(JSON.stringify(elementData));
				}
			},
			
			insert: function(vnode) {
				console.log('[LiteHTML Module] insert:', vnode.sel, 'key:', vnode.key);
				// Element has been inserted into virtual DOM tree - tell litehtml about position
				if (vnode.sel && vnode.sel !== '!' && vnode.sel !== 'text') {
					__insertLiteHTMLElement(vnode.key || vnode.sel);
				}
			},
			
			remove: function(vnode, removeCallback) {
				console.log('[LiteHTML Module] remove:', vnode.sel, 'key:', vnode.key);
				if (vnode.sel && vnode.sel !== '!' && vnode.sel !== 'text') {
					__removeLiteHTMLElement(vnode.key || vnode.sel);
				}
				removeCallback(); // Always call the callback to complete removal
			},
			
			destroy: function(vnode) {
				console.log('[LiteHTML Module] destroy:', vnode.sel, 'key:', vnode.key);
				if (vnode.sel && vnode.sel !== '!' && vnode.sel !== 'text') {
					__destroyLiteHTMLElement(vnode.key || vnode.sel);
				}
			}
		};
		
		// Initialize patch function with custom litehtml module first, then standard modules
		if (snabbdom.init && snabbdom.classModule && snabbdom.propsModule && snabbdom.styleModule && snabbdom.eventListenersModule) {
			window.snabbdomPatch = snabbdom.init([
				liteHTMLModule,              // Our custom litehtml bridge module FIRST
				snabbdom.classModule,        // CSS class management
				snabbdom.propsModule,        // DOM properties
				snabbdom.styleModule,        // CSS styles
				snabbdom.eventListenersModule // Event handling
			]);
			
			// Create MuJS-compatible h function wrapper using vnode directly
			function createMuJSCompatibleH() {
				return function h(sel, b, c) {
					// Minimal argument handling to avoid complex logic
					var data = {};
					var children = [];
					var text = undefined;
					
					if (arguments.length === 1) {
						// h('div')
					} else if (arguments.length === 2) {
						if (typeof b === 'string') {
							text = b; // h('div', 'text')
						} else if (b && b.constructor === Array) {
							children = b; // h('div', [children])
						} else if (b) {
							data = b; // h('div', {props})
						}
					} else if (arguments.length === 3) {
						data = b || {};
						if (typeof c === 'string') {
							text = c; // h('div', {props}, 'text')
						} else if (c) {
							children = c; // h('div', {props}, [children])
						}
					}
					
					return snabbdom.vnode(sel, data, children, text, undefined);
				};
			}
			
			// Use our MuJS-compatible h function instead of the broken one
			var compatibleH = createMuJSCompatibleH();
			window.h = compatibleH;
			window.snabbdom = snabbdom;
			
			// IMPORTANT: Also export to global scope for page scripts
			this.h = compatibleH;
			this.snabbdomPatch = window.snabbdomPatch;
			
			// Initialize virtual DOM state tracking
			window.currentVTree = null;
			window.pendingPatches = [];
			
			console.log('[VirtualDOM] Snabbdom initialized successfully with litehtml module');
			console.log('[VirtualDOM] h function available at: window.h and globally as h');
			console.log('[VirtualDOM] snabbdomPatch available at: window.snabbdomPatch and globally as snabbdomPatch');
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
	
	// Snabbdom litehtml bridge callbacks
	engine->registerGlobalFunction("__createLiteHTMLElement", [this]() {
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string elementDataJson = engine->getArgString(0);
			createElementInLiteHTML(elementDataJson);
		}
	});
	
	engine->registerGlobalFunction("__updateLiteHTMLElement", [this]() {
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string elementDataJson = engine->getArgString(0);
			updateElementInLiteHTML(elementDataJson);
		}
	});
	
	engine->registerGlobalFunction("__insertLiteHTMLElement", [this]() {
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string elementKey = engine->getArgString(0);
			insertElementInLiteHTML(elementKey);
		}
	});
	
	engine->registerGlobalFunction("__removeLiteHTMLElement", [this]() {
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string elementKey = engine->getArgString(0);
			removeElementFromLiteHTML(elementKey);
		}
	});
	
	engine->registerGlobalFunction("__destroyLiteHTMLElement", [this]() {
		if (engine->argCount() >= 1 && engine->argIsString(0)) {
			std::string elementKey = engine->getArgString(0);
			destroyElementInLiteHTML(elementKey);
		}
	});
	
	// Event handling through Snabbdom
	engine->registerGlobalFunction("__handleEvent", [this]() {
		if (engine->argCount() >= 3 && engine->argIsString(0) && engine->argIsString(1)) {
			std::string elementKey = engine->getArgString(0);
			std::string eventType = engine->getArgString(1);
			std::string eventDataJson = engine->getArgString(2);
			handleElementEvent(elementKey, eventType, eventDataJson);
		}
	});
}

void VirtualDOM::createDOMWithJavaScript()
{
	// Create a Snabbdom-based DOM API that routes everything through virtual DOM
	std::string domScript = R"(
		console.log("[VDOM] Starting Snabbdom-based DOM creation");
		
		// Initialize virtual DOM state
		var virtualDOMContainer = null;
		var elementVNodes = {}; // Track vnodes by element ID/key (MuJS compatible object)
		
		// MuJS-compatible helper function to convert Map-like object to plain object
		function mapToObject(mapLikeObj) {
			var result = {};
			if (mapLikeObj && mapLikeObj._keys && mapLikeObj._values) {
				for (var i = 0; i < mapLikeObj._keys.length; i++) {
					result[mapLikeObj._keys[i]] = mapLikeObj._values[i];
				}
			}
			return result;
		}
		
		// MuJS-compatible Map-like object
		function createMapLike() {
			return {
				_keys: [],
				_values: [],
				set: function(key, value) {
					var index = this._keys.indexOf(key);
					if (index !== -1) {
						this._values[index] = value;
					} else {
						this._keys.push(key);
						this._values.push(value);
					}
				},
				get: function(key) {
					var index = this._keys.indexOf(key);
					return index !== -1 ? this._values[index] : undefined;
				},
				delete: function(key) {
					var index = this._keys.indexOf(key);
					if (index !== -1) {
						this._keys.splice(index, 1);
						this._values.splice(index, 1);
						return true;
					}
					return false;
				},
				get size() {
					return this._keys.length;
				}
			};
		}
		
		// Enhanced createElement that creates virtual nodes
		function createElement(tagName) {
			try {
				// Generate a unique key for this element
				var elementKey = 'elem_' + Math.random().toString(36).slice(2, 11);
				
				// Create the virtual node using window.h for MuJS compatibility
				var vnode;
				try {
					vnode = window.h(tagName.toLowerCase(), { key: elementKey });
				} catch (e) {
					console.log('[createElement] Error creating vnode:', e.message);
					return null;
				}
				
				// Create a proxy object that looks like a DOM element but routes through Snabbdom
				var element = {
					tagName: tagName.toUpperCase(),
					key: elementKey,
					id: "",
					_textContent: "",
					_innerHTML: "",
					_vnode: vnode,
					_eventListeners: createMapLike()
				};
				
				// Store in our tracking object
				elementVNodes[elementKey] = vnode;
			
			// textContent property with getter/setter that patches through Snabbdom
			Object.defineProperty(element, 'textContent', {
				get: function() {
					return this._textContent || "";
				},
				set: function(value) {
					console.log('[textContent setter] Setting text:', value, 'on element:', this.key);
					this._textContent = value;
					
					// Create new vnode with updated text and patch it
					var newVnode = window.h(this.tagName.toLowerCase(), { 
						key: this.key,
						props: this._props || {},
						attrs: this._attrs || {},
						on: this._eventListeners.size > 0 ? mapToObject(this._eventListeners) : {}
					}, value);
					
					// Update our tracking
					var oldVnode = elementVNodes[this.key] || this._vnode;
					elementVNodes[this.key] = newVnode;
					
					// Patch through Snabbdom (this will trigger our litehtml module)
					if (window.snabbdomPatch) {
						if (window.currentVTree) {
							window.currentVTree = window.snabbdomPatch(oldVnode, newVnode);
						} else {
							// Initialize the virtual tree on first patch
							window.currentVTree = newVnode;
							// Still patch to trigger our litehtml module
							window.currentVTree = window.snabbdomPatch(oldVnode, newVnode);
						}
					}
				},
				configurable: true,
				enumerable: true
			});
			
			// innerHTML property with getter/setter that patches through Snabbdom
			Object.defineProperty(element, 'innerHTML', {
				get: function() {
					return this._innerHTML || "";
				},
				set: function(value) {
					console.log('[innerHTML setter] Setting HTML:', value, 'on element:', this.key);
					console.log('[innerHTML setter] typeof window:', typeof window);
					console.log('[innerHTML setter] typeof window.__updateElementHTML:', typeof window.__updateElementHTML);
					this._innerHTML = value;
					
					// For innerHTML updates, we bypass Snabbdom and directly call our C++ bridge
					// because innerHTML contains complex HTML that Snabbdom virtual nodes can't easily represent
					console.log('[innerHTML setter] Calling __updateElementHTML directly');
					
					try {
						// CRITICAL: For MuJS, we need to call via window. since MuJS doesn't have global function access
						if (typeof window !== 'undefined' && window.__updateElementHTML) {
							window.__updateElementHTML(this.key, value);
						} else {
							// Fallback for other engines
							__updateElementHTML(this.key, value);
						}
					} catch (e) {
						console.log('[innerHTML setter] Error calling __updateElementHTML:', e.message);
						throw e;
					}
				},
				configurable: true,
				enumerable: true
			});
			
			// addEventListener that goes through Snabbdom's event system
			element.addEventListener = function(eventType, handler, options) {
				console.log('[addEventListener] Adding', eventType, 'listener to element:', this.key);
				
				// Store the event listener
				this._eventListeners.set(eventType, handler);
				
				// Create new vnode with event listener and patch it
				var newVnode = window.h(this.tagName.toLowerCase(), { 
					key: this.key,
					props: this._props || {},
					attrs: this._attrs || {},
					on: mapToObject(this._eventListeners)
				}, this._textContent || "");
				
				// Update our tracking
				var oldVnode = elementVNodes[this.key] || this._vnode;
				elementVNodes[this.key] = newVnode;
				
				// Patch through Snabbdom (eventListenersModule will handle the actual event binding)
				if (window.snabbdomPatch && window.currentVTree) {
					window.currentVTree = window.snabbdomPatch(oldVnode, newVnode);
				}
			};
			
			// removeEventListener
			element.removeEventListener = function(eventType, handler) {
				console.log('[removeEventListener] Removing', eventType, 'listener from element:', this.key);
				this._eventListeners.delete(eventType);
				
				// Re-patch without the event listener
				var newVnode = window.h(this.tagName.toLowerCase(), { 
					key: this.key,
					props: this._props || {},
					attrs: this._attrs || {},
					on: mapToObject(this._eventListeners)
				}, this._textContent || "");
				
				var oldVnode = elementVNodes[this.key] || this._vnode;
				elementVNodes[this.key] = newVnode;
				
				if (window.snabbdomPatch && window.currentVTree) {
					window.currentVTree = window.snabbdomPatch(oldVnode, newVnode);
				}
			};
			
			return element;
			} catch (e) {
				console.log('[createElement] ERROR in createElement:', e.message);
				console.log('[createElement] Error stack:', e.stack || 'no stack');
				throw e;
			}
		}
		
		// Enhanced getElementById that works with our virtual DOM system
		function getElementById(id) {
			console.log('[getElementById] Looking for element with id:', id);
			
			// First check if we have this element in our virtual DOM (MuJS compatible iteration)
			for (var key in elementVNodes) {
				if (elementVNodes.hasOwnProperty(key)) {
					var vnode = elementVNodes[key];
					if (vnode.data && vnode.data.attrs && vnode.data.attrs.id === id) {
						console.log('[getElementById] Found element in virtual DOM:', id);
						// Return the element wrapper from our tracking
						return document._elementWrappers[key];
					}
				}
			}
			
			// Fall back to checking litehtml
			var tagName = __getElementById(id);
			if (tagName) {
				console.log('[getElementById] Found element in litehtml:', id, 'tag:', tagName);
				var element = createElement(tagName);
				
				// IMPORTANT: Use the actual ID as the key instead of random generated key
				element.key = id;
				element.id = id;
				
				// Set the id attribute in the vnode using the actual ID as key
				var newVnode = window.h(tagName.toLowerCase(), { 
					key: id,  // Use actual ID as key
					attrs: { id: id }
				});
				
				elementVNodes[id] = newVnode;  // Use ID as object key
				
				// Store wrapper for future lookups
				if (!document._elementWrappers) {
					document._elementWrappers = {};
				}
				document._elementWrappers[id] = element;  // Use ID as object key
				
				return element;
			}
			
			console.log('[getElementById] Element not found:', id);
			return null;
		}
		
		// Create document object with Snabbdom integration
		var document = {
			createElement: createElement,
			getElementById: getElementById,
			_elementWrappers: {} // Track element wrappers by key (MuJS compatible object)
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
		
		// Create window object with proper parent/location hierarchy for MuJS compatibility
		var window = {
			document: document,
			location: location,
			alert: function(message) {
				__alert(message);
			},
			// CRITICAL: Add bridge functions to window object for innerHTML setters
			__updateElementHTML: __updateElementHTML,
			__getElementById: __getElementById,
			__updateTextContent: __updateTextContent,
			__updateLiteHTMLElement: __updateLiteHTMLElement,
			__createLiteHTMLElement: __createLiteHTMLElement
		};
		
		// CRITICAL: Set up window hierarchy with proper location access for MuJS
		// MuJS needs explicit object references, can't use forward references
		window.parent = {
			location: location,
			document: document
		};
		window.top = window;
		window.self = window;
		
		// IMPORTANT: Make sure parent.location has all required methods
		if (!window.parent.location.replace) {
			window.parent.location.replace = function(url) {
				__setHref(url);
			};
		}
		if (!window.parent.location.assign) {
			window.parent.location.assign = function(url) {
				__setHref(url);
			};
		}
		if (!window.parent.location.reload) {
			window.parent.location.reload = function() {
				__reloadPage();
			};
		}
		
		// Expose globally - CRITICAL: MuJS requires explicit global binding
		if (typeof globalThis !== 'undefined') {
			globalThis.window = window;
			globalThis.document = document;
		}
		
		// IMPORTANT: For MuJS compatibility, bind directly to global scope
		// MuJS doesn't have globalThis, so we need to use 'this' in global context
		try {
			// In MuJS, 'this' at global level refers to the global object
			if (typeof this === 'object' && this !== null) {
				this.window = window;
				this.document = document;
				this.location = location; // Make location directly accessible too
				
				// CRITICAL: Also bind commonly used onclick patterns directly
				this.alert = window.alert;
				
				// CRITICAL: Bind bridge functions that are called from innerHTML setter and other DOM operations
				this.__updateElementHTML = __updateElementHTML;
				this.__getElementById = __getElementById;
				this.__updateTextContent = __updateTextContent;
				this.__updateLiteHTMLElement = __updateLiteHTMLElement;
				this.__createLiteHTMLElement = __createLiteHTMLElement;
				
				// Verify the bindings work
				console.log('[VDOM] MuJS global bindings created:');
				console.log('[VDOM] - this.window:', typeof this.window);
				console.log('[VDOM] - this.document:', typeof this.document);
				console.log('[VDOM] - this.window.parent:', typeof this.window.parent);
				console.log('[VDOM] - this.window.parent.location:', typeof this.window.parent.location);
				console.log('[VDOM] - this.window.parent.location.replace:', typeof this.window.parent.location.replace);
			}
		} catch (e) {
			// Fallback for engines that don't support 'this' at global level
			console.log('[VDOM] Could not bind to global this:', e.message);
		}
		
		console.log("[VDOM] Snabbdom-based DOM creation completed");
	)";
	
	if (!engine->executeScript(domScript)) {
		std::cout << "[VirtualDOM] Failed to execute Snabbdom-based DOM creation script" << std::endl;
	}
}

// These methods are now deprecated but kept for backward compatibility
// All new code should go through Snabbdom
void VirtualDOM::updateElementTextContent(const std::string& elementId, const std::string& newText)
{
	std::cout << "[VirtualDOM] updateElementTextContent (legacy): " << elementId << " -> " << newText << std::endl;
	std::cout << "[VirtualDOM] WARNING: Direct updateElementTextContent called - should use Snabbdom instead" << std::endl;
	
	// Route through Snabbdom if possible
	if (engine) {
		std::string snabbdomUpdate = R"(
			try {
				var element = document.getElementById(')" + elementId + R"(');
				if (element) {
					element.textContent = ')" + newText + R"(';
					console.log('[VirtualDOM] Routed legacy textContent update through Snabbdom');
				} else {
					console.log('[VirtualDOM] Element not found for legacy update:', ')" + elementId + R"(');
				}
			} catch (e) {
				console.log('[VirtualDOM] Error in legacy textContent update:', e.message);
			}
		)";
		
		if (engine->executeScript(snabbdomUpdate)) {
			return; // Successfully routed through Snabbdom
		}
	}
	
	// Fallback to direct litehtml manipulation if Snabbdom routing fails
	auto element = findElementByIdInLiteHTML(elementId);
	if (element && webView) {
		std::string& htmlContent = webView->contents;
		std::string searchPattern = "id=\"" + elementId + "\"";
		size_t elementPos = htmlContent.find(searchPattern);
		
		if (elementPos != std::string::npos) {
			size_t tagStart = htmlContent.rfind('<', elementPos);
			size_t contentStart = htmlContent.find('>', elementPos) + 1;
			size_t contentEnd = htmlContent.find('<', contentStart);
			
			if (tagStart != std::string::npos && contentStart != std::string::npos && contentEnd != std::string::npos) {
				std::string before = htmlContent.substr(0, contentStart);
				std::string after = htmlContent.substr(contentEnd);
				htmlContent = before + newText + after;
				
				std::cout << "[VirtualDOM] Updated HTML content via fallback, triggering re-render" << std::endl;
				webView->recreateDocument();
				return;
			}
		}
	}
	
	std::cout << "[VirtualDOM] Legacy textContent update failed: " << elementId << std::endl;
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

bool VirtualDOM::updateElementTextDirectly(litehtml::element::ptr element, const std::string& newText)
{
	std::cout << "[VirtualDOM] updateElementTextDirectly: " << newText << std::endl;
	
	if (!element || !webView) {
		std::cout << "[VirtualDOM] No element or webView provided for direct update" << std::endl;
		return false;
	}
	
	try {
		// Alternative approach: Update the HTML string but avoid full document recreation
		// by using a more targeted update strategy
		
		std::string elementId = element->get_attr("id");
		if (elementId.empty()) {
			std::cout << "[VirtualDOM] Element has no ID, cannot update directly" << std::endl;
			return false;
		}
		
		std::cout << "[VirtualDOM] Attempting targeted HTML update for element: " << elementId << std::endl;
		
		// Update the HTML content string (same as before)
		std::string& htmlContent = webView->contents;
		std::string searchPattern = "id=\"" + elementId + "\"";
		size_t elementPos = htmlContent.find(searchPattern);
		
		if (elementPos != std::string::npos) {
			// Find the opening and closing tags
			size_t tagStart = htmlContent.rfind('<', elementPos);
			size_t contentStart = htmlContent.find('>', elementPos) + 1;
			
			if (tagStart != std::string::npos && contentStart != std::string::npos) {
				// Extract tag name
				size_t tagNameStart = tagStart + 1;
				size_t tagNameEnd = htmlContent.find_first_of(" >", tagNameStart);
				std::string tagName = htmlContent.substr(tagNameStart, tagNameEnd - tagNameStart);
				
				// Find the closing tag
				std::string closingTag = "</" + tagName + ">";
				size_t contentEnd = htmlContent.find(closingTag, contentStart);
				
				if (contentEnd != std::string::npos) {
					// Replace the content between opening and closing tags
					std::string before = htmlContent.substr(0, contentStart);
					std::string after = htmlContent.substr(contentEnd);
					htmlContent = before + newText + after;
					
					std::cout << "[VirtualDOM] Successfully updated HTML string for element: " << elementId << std::endl;
					
					// CRITICAL: Recreate litehtml document from updated HTML string while preserving JS context
					std::cout << "[VirtualDOM] Recreating litehtml document from updated HTML (preserving JS context)" << std::endl;
					recreateLiteHTMLDocumentOnly();
					
					return true; // Indicate that we successfully updated the HTML string
				}
			}
		}
		
		std::cout << "[VirtualDOM] Could not find element in HTML string: " << elementId << std::endl;
		return false;
		
	} catch (const std::exception& e) {
		std::cout << "[VirtualDOM] Error in direct element update: " << e.what() << std::endl;
		return false;
	}
}

void VirtualDOM::updateElementInnerHTML(const std::string& elementId, const std::string& newHTML)
{
	std::cout << "[VirtualDOM] updateElementInnerHTML: " << elementId << " -> " << newHTML << std::endl;
	
	// For innerHTML updates, we should directly update the HTML content and recreate the document
	// Don't route through Snabbdom since innerHTML is complex HTML content
	
	auto element = findElementByIdInLiteHTML(elementId);
	if (element && webView) {
		std::string& htmlContent = webView->contents;
		std::string searchPattern = "id=\"" + elementId + "\"";
		size_t elementPos = htmlContent.find(searchPattern);
		
		if (elementPos != std::string::npos) {
			size_t tagStart = htmlContent.rfind('<', elementPos);
			size_t contentStart = htmlContent.find('>', elementPos) + 1;
			
			if (tagStart != std::string::npos && contentStart != std::string::npos) {
				size_t tagNameStart = tagStart + 1;
				size_t tagNameEnd = htmlContent.find_first_of(" >", tagNameStart);
				std::string tagName = htmlContent.substr(tagNameStart, tagNameEnd - tagNameStart);
				
				std::string closingTag = "</" + tagName + ">";
				size_t contentEnd = htmlContent.find(closingTag, contentStart);
				
				if (contentEnd != std::string::npos) {
					std::string before = htmlContent.substr(0, contentStart);
					std::string after = htmlContent.substr(contentEnd);
					htmlContent = before + newHTML + after;
					
					std::cout << "[VirtualDOM] Successfully updated HTML content for innerHTML" << std::endl;
					std::cout << "[VirtualDOM] New HTML content: " << newHTML << std::endl;
					
					// Use our targeted document recreation method
					std::cout << "[VirtualDOM] Calling recreateLiteHTMLDocumentOnly() for innerHTML update" << std::endl;
					recreateLiteHTMLDocumentOnly();
					return;
				}
			}
		}
		
		std::cout << "[VirtualDOM] Could not find element in HTML for innerHTML update: " << elementId << std::endl;
	}
	
	std::cout << "[VirtualDOM] innerHTML update failed: " << elementId << std::endl;
}

void VirtualDOM::processBatchUpdates(const std::string& updatesJson)
{
	std::cout << "[VirtualDOM] Processing batch updates through Snabbdom: " << updatesJson << std::endl;
	
	if (!engine) {
		std::cerr << "[VirtualDOM] No engine available for batch updates" << std::endl;
		return;
	}
	
	// Route through Snabbdom instead of direct DOM manipulation
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
			
			// Process each update by creating appropriate virtual nodes and patching
			updates.forEach(function(update) {
				if (update.type === 'textContent') {
					// Create a vnode with the new text content and patch it
					var element = document.getElementById(update.elementId);
					if (element) {
						element.textContent = update.value; // This will go through our Snabbdom system
						console.log('[VirtualDOM] Updated element', update.elementId, 'text via Snabbdom');
					}
				} else if (update.type === 'innerHTML') {
					var element = document.getElementById(update.elementId);
					if (element) {
						element.innerHTML = update.value; // This will go through our Snabbdom system
						console.log('[VirtualDOM] Updated element', update.elementId, 'innerHTML via Snabbdom');
					}
				} else if (update.type === 'setAttribute') {
					// Create vnode with updated attributes
					console.log('[VirtualDOM] Attribute updates via Snabbdom not yet implemented');
				} else if (update.type === 'addClass' || update.type === 'removeClass') {
					// Create vnode with updated classes
					console.log('[VirtualDOM] Class updates via Snabbdom not yet implemented');
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

void VirtualDOM::createElementInLiteHTML(const std::string& elementDataJson)
{
	std::cout << "[VirtualDOM] createElementInLiteHTML: " << elementDataJson << std::endl;
	
	// TODO: modify the HTML content and recreate the document
	// For now, just trigger a re-render since creation often happens via innerHTML
	if (webView) {
		std::cout << "[VirtualDOM] Triggering document recreation for element creation" << std::endl;
		webView->recreateDocument();
	}
}

void VirtualDOM::updateElementInLiteHTML(const std::string& elementDataJson)
{
	std::cout << "[VirtualDOM] updateElementInLiteHTML: " << elementDataJson << std::endl;
	
	if (!webView) {
		std::cout << "[VirtualDOM] No webView available for update" << std::endl;
		return;
	}
	
	try {
		// Parse the JSON element data from Snabbdom
		// Expected format: { "tag": "div", "key": "elementId", "text": "new text", "attrs": {...}, ... }
		
		// Simple JSON parsing for the key fields we need
		std::string elementKey, newText, tag;
		bool hasText = false;
		
		// Extract key/id
		size_t keyPos = elementDataJson.find("\"key\":");
		if (keyPos != std::string::npos) {
			size_t keyStart = elementDataJson.find("\"", keyPos + 6) + 1;
			size_t keyEnd = elementDataJson.find("\"", keyStart);
			if (keyEnd != std::string::npos) {
				elementKey = elementDataJson.substr(keyStart, keyEnd - keyStart);
			}
		}
		
		// Extract tag
		size_t tagPos = elementDataJson.find("\"tag\":");
		if (tagPos != std::string::npos) {
			size_t tagStart = elementDataJson.find("\"", tagPos + 6) + 1;
			size_t tagEnd = elementDataJson.find("\"", tagStart);
			if (tagEnd != std::string::npos) {
				tag = elementDataJson.substr(tagStart, tagEnd - tagStart);
			}
		}
		
		// Extract text content, the text might be empty but still valid
		// Checks if the text field exists, not just whether or not it has content
		size_t textPos = elementDataJson.find("\"text\":");
		if (textPos != std::string::npos) {
			size_t textStart = elementDataJson.find("\"", textPos + 7) + 1;
			size_t textEnd = elementDataJson.find("\"", textStart);
			if (textEnd != std::string::npos) {
				newText = elementDataJson.substr(textStart, textEnd - textStart);
				hasText = true;
				std::cout << "[VirtualDOM] Found text field with content: '" << newText << "'" << std::endl;
			}
		}

		std::cout << "[VirtualDOM] Parsed update - key: " << elementKey << ", tag: " << tag << ", text: '" << newText << "'" << std::endl;
		
		// Try direct litehtml DOM manipulation first (preserves JavaScript context)
		if (hasText && !elementKey.empty()) {
			auto element = findElementByIdInLiteHTML(elementKey);
			if (element && updateElementTextDirectly(element, newText)) {
				std::cout << "[VirtualDOM] Successfully updated element directly without document recreation" << std::endl;
				// Just trigger a render, don't recreate the document
				if (webView) webView->needsRender = true;
				return;
			}
		}
		
		// Old/Fallback: Update HTML content string and recreate (preserves visual changes but loses JS context)
		// TODO: remove this, and make sure it doesn't break anything
		if (hasText && !elementKey.empty()) {
			std::string& htmlContent = webView->contents;
			
			// Find the element in the HTML by id attribute
			std::string searchPattern = "id=\"" + elementKey + "\"";
			size_t elementPos = htmlContent.find(searchPattern);
			
			if (elementPos != std::string::npos) {
				// Find the opening and closing tags
				size_t tagStart = htmlContent.rfind('<', elementPos);
				size_t contentStart = htmlContent.find('>', elementPos) + 1;
				
				if (tagStart != std::string::npos && contentStart != std::string::npos) {
					// Extract tag name
					size_t tagNameStart = tagStart + 1;
					size_t tagNameEnd = htmlContent.find_first_of(" >", tagNameStart);
					std::string tagName = htmlContent.substr(tagNameStart, tagNameEnd - tagNameStart);
					
					// Find the closing tag
					std::string closingTag = "</" + tagName + ">";
					size_t contentEnd = htmlContent.find(closingTag, contentStart);
					
					if (contentEnd != std::string::npos) {
						// Replace the content between opening and closing tags
						std::string before = htmlContent.substr(0, contentStart);
						std::string after = htmlContent.substr(contentEnd);
						htmlContent = before + newText + after;
						
						std::cout << "[VirtualDOM] Updated HTML content string for element: " << elementKey << std::endl;
						std::cout << "[VirtualDOM] New content: " << newText << std::endl;
						
						std::cout << "[VirtualDOM] WARNING: Falling back to document recreation - this will break JS context" << std::endl;
						recreateDocumentWithStatePreservation();
						return;
					}
				}
			}
			
			std::cout << "[VirtualDOM] Could not find element in HTML: " << elementKey << std::endl;
		}
		
	} catch (const std::exception& e) {
		std::cout << "[VirtualDOM] Error updating element: " << e.what() << std::endl;
	}
	
	// Final fallback: just trigger a re-render
	if (webView) {
		webView->needsRender = true;
	}
}

void VirtualDOM::insertElementInLiteHTML(const std::string& elementKey)
{
	std::cout << "[VirtualDOM] insertElementInLiteHTML: " << elementKey << std::endl;
	
	// Handle element insertion in litehtml DOM, probably using litehtml's appendChild
	
	if (webView) {
		webView->needsRender = true;
	}
}

void VirtualDOM::removeElementFromLiteHTML(const std::string& elementKey)
{
	std::cout << "[VirtualDOM] removeElementFromLiteHTML: " << elementKey << std::endl;
	
	// Handle element removal from litehtml DOM (removeChild?)
	
	if (webView) {
		webView->needsRender = true;
	}
}

void VirtualDOM::destroyElementInLiteHTML(const std::string& elementKey)
{
	std::cout << "[VirtualDOM] destroyElementInLiteHTML: " << elementKey << std::endl;
	
	// Handle element cleanup in litehtml DOM
	
	if (webView) {
		webView->needsRender = true;
	}
}

void VirtualDOM::handleElementEvent(const std::string& elementKey, const std::string& eventType, const std::string& eventDataJson)
{
	std::cout << "[VirtualDOM] handleElementEvent: " << elementKey << " event: " << eventType << " data: " << eventDataJson << std::endl;
	
	// TODO: handle events triggered in litehtml and forward them to the JavaScript event handlers
	// The Snabbdom eventListenersModule will handle the JavaScript side
	
	// For a click event:
	// 1. Find the element by key
	// 2. Create an event object
	// 3. Trigger the JavaScript event handler
	
	if (!engine) return;
	
	std::string eventScript = R"(
		try {
			// Find the element and trigger its event handler
			var elementKey = ')" + elementKey + R"(';
			var eventType = ')" + eventType + R"(';
			
			// TODO: properly construct event objects and handle propagation
			console.log('[VirtualDOM] Dispatching event', eventType, 'on element', elementKey);
			
		} catch (e) {
			console.log('[VirtualDOM] Error handling event:', e.message);
		}
	)";
	
	engine->executeScript(eventScript);
}

void VirtualDOM::recreateDocumentWithStatePreservation()
{
	std::cout << "[VirtualDOM] recreateDocumentWithStatePreservation() called" << std::endl;
	
	if (!webView || !engine) {
		std::cout << "[VirtualDOM] Missing webView or engine for state preservation" << std::endl;
		if (webView) webView->recreateDocument();
		return;
	}
	
	// Simpler approach: Just recreate the document and reinitialize Snabbdom
	// The page scripts will be re-executed anyway, so we don't need complex state preservation
	
	std::cout << "[VirtualDOM] Calling webView->recreateDocument()..." << std::endl;
	webView->recreateDocument();
	
	// Re-initialize Snabbdom after document recreation
	// This ensures the bridge functions are available again
	std::cout << "[VirtualDOM] Re-initializing Snabbdom after document recreation..." << std::endl;
	
	if (!loadSnabbdomBundle()) {
		std::cout << "[VirtualDOM] Failed to re-load Snabbdom after recreation" << std::endl;
		return;
	}
	
	std::cout << "[VirtualDOM] Document recreation completed - Snabbdom reinitialized" << std::endl;
}

void VirtualDOM::recreateLiteHTMLDocumentOnly()
{
	std::cout << "[VirtualDOM] recreateLiteHTMLDocumentOnly() - preserving JavaScript context" << std::endl;
	
	if (!webView || !webView->container) {
		std::cout << "[VirtualDOM] No webView or container available" << std::endl;
		return;
	}
	
	try {
		// Recreate only the litehtml document part, preserving JavaScript engine state
		// This is based on WebView::recreateDocument() but without touching scripts
		
		std::string currentUrl = webView->url;
		
		// Clear the old litehtml document
		webView->m_doc = nullptr;
		
		// Recreate the container (this handles rendering/layout)
		delete webView->container;
		webView->container = new BrocContainer(webView);
		webView->container->set_base_url(currentUrl.c_str());
		
		// Recreate litehtml document from the updated HTML string
		webView->m_doc = litehtml::document::createFromString(webView->contents.c_str(), webView->container);
		
		// Trigger a visual re-render
		webView->needsRender = true;
		
		std::cout << "[VirtualDOM] Successfully recreated litehtml document while preserving JS context" << std::endl;
		
	} catch (const std::exception& e) {
		std::cout << "[VirtualDOM] Error in recreateLiteHTMLDocumentOnly: " << e.what() << std::endl;
		
		// Fallback to full recreation if targeted approach fails
		std::cout << "[VirtualDOM] Falling back to full document recreation" << std::endl;
		recreateDocumentWithStatePreservation();
	}
}
