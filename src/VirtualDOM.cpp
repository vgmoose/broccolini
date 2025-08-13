#include "VirtualDOM.hpp"
#include "../utils/BrocContainer.hpp"
#include "../utils/Utils.hpp"
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
	
	// Load JavaScript files into memory once at initialization
	if (!loadJavaScriptFiles()) {
		std::cerr << "[VirtualDOM] VirtualDOM functionality will be disabled due to missing JavaScript files" << std::endl;
	} else {
		std::cout << "[VirtualDOM] All JavaScript files loaded successfully - VirtualDOM ready" << std::endl;
	}
}

VirtualDOM::~VirtualDOM() { }

bool VirtualDOM::loadJavaScriptFiles()
{
	std::cout << "[VirtualDOM] Loading JavaScript files from disk..." << std::endl;
	
	bool success = true;
	
	// Load AMD setup script
	try {
		amdSetupScript = readFile(RAMFS "/res/js/amd-setup.js");
		if (amdSetupScript.empty()) {
			std::cerr << "[VirtualDOM] Failed to load amd-setup.js - file empty or not found" << std::endl;
			success = false;
		} else {
			std::cout << "[VirtualDOM] Loaded amd-setup.js (" << amdSetupScript.length() << " chars)" << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "[VirtualDOM] Error loading amd-setup.js: " << e.what() << std::endl;
		success = false;
	}
	
	// Load Snabbdom initialization script
	try {
		snabbdomInitScript = readFile(RAMFS "/res/js/snabbdom-init.js");
		if (snabbdomInitScript.empty()) {
			std::cerr << "[VirtualDOM] Failed to load snabbdom-init.js - file empty or not found" << std::endl;
			success = false;
		} else {
			std::cout << "[VirtualDOM] Loaded snabbdom-init.js (" << snabbdomInitScript.length() << " chars)" << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "[VirtualDOM] Error loading snabbdom-init.js: " << e.what() << std::endl;
		success = false;
	}
	
	// Load DOM creation script
	try {
		domCreationScript = readFile(RAMFS "/res/js/dom-creation.js");
		if (domCreationScript.empty()) {
			std::cerr << "[VirtualDOM] Failed to load dom-creation.js - file empty or not found" << std::endl;
			success = false;
		} else {
			std::cout << "[VirtualDOM] Loaded dom-creation.js (" << domCreationScript.length() << " chars)" << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "[VirtualDOM] Error loading dom-creation.js: " << e.what() << std::endl;
		success = false;
	}
	
	// Load window bootstrap script
	try {
		windowBootstrapScript = readFile(RAMFS "/res/js/window-bootstrap.js");
		if (windowBootstrapScript.empty()) {
			std::cerr << "[VirtualDOM] Failed to load window-bootstrap.js - file empty or not found" << std::endl;
			success = false;
		} else {
			std::cout << "[VirtualDOM] Loaded window-bootstrap.js (" << windowBootstrapScript.length() << " chars)" << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "[VirtualDOM] Error loading window-bootstrap.js: " << e.what() << std::endl;
		success = false;
	}
	
	if (!success) {
		std::cerr << "[VirtualDOM] Failed to load required JavaScript files - VirtualDOM will be disabled" << std::endl;
	}
	
	return success;
}

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
	// Check if scripts were loaded successfully
	if (amdSetupScript.empty() || snabbdomInitScript.empty()) {
		std::cerr << "[VirtualDOM] JavaScript files not loaded, cannot initialize Snabbdom" << std::endl;
		return false;
	}
	
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
	
	// Execute AMD setup script (from file)
	if (!engine->executeScript(amdSetupScript)) {
		std::cerr << "[VirtualDOM] Failed to set up AMD loader" << std::endl;
		return false;
	}
	
	// Then load the Snabbdom bundle
	if (!engine->executeScript(snabbdomCode)) {
		std::cerr << "[VirtualDOM] Failed to execute Snabbdom bundle" << std::endl;
		return false;
	}
	
	// Initialize Snabbdom using the loaded script (from file)
	if (!engine->executeScript(snabbdomInitScript)) {
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
	if (jsEngine && !windowBootstrapScript.empty())
	{
		if (!jsEngine->executeScript(windowBootstrapScript)) {
			std::cerr << "[VirtualDOM] Failed to execute window bootstrap script" << std::endl;
		}
	}
	else
	{
		std::cerr << "[VirtualDOM] Window bootstrap script not available - VirtualDOM disabled" << std::endl;
		return;
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
			
			// Use AlertManager for rate limiting and proper alert handling
			if (webView && webView->alertManager) {
				webView->alertManager->showAlert(message, [](bool dismissed) {
					// Alert was dismissed - in a more advanced implementation,
					// this is where you'd resume JavaScript execution
					std::cout << "[ALERT] Alert dismissed: " << dismissed << std::endl;
				});
			}
			
			// Check if execution should be interrupted
			if (engine->isExecutionInterrupted()) {
				std::cout << "[VirtualDOM] Execution interrupted - stopping script execution" << std::endl;
				
				// MuJS: Directly throw an error from C++ to stop JavaScript execution
				engine->throwError("Execution interrupted due to infinite loop detection");
				return;
			}
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
	// Check if scripts were loaded successfully
	if (domCreationScript.empty()) {
		std::cerr << "[VirtualDOM] DOM creation script not available - VirtualDOM disabled" << std::endl;
		return;
	}
	
	// Execute the DOM creation script from file
	if (!engine->executeScript(domCreationScript)) {
		std::cerr << "[VirtualDOM] Failed to execute DOM creation script from file" << std::endl;
	}
}

// These methods are now deprecated but kept for backward compatibility
// All new code should go through Snabbdom
void VirtualDOM::updateElementTextContent(const std::string& elementId, const std::string& newText)
{
	std::cout << "[VirtualDOM] updateElementTextContent (legacy): " << elementId << " -> " << newText << std::endl;
	std::cout << "[VirtualDOM] WARNING: Direct updateElementTextContent called - should use Snabbdom instead" << std::endl;
	
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
	
	std::cout << "[VirtualDOM] textContent update failed: " << elementId << std::endl;
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
