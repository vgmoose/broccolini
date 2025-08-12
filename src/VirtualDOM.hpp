#ifndef VIRTUAL_DOM_HPP
#define VIRTUAL_DOM_HPP

#include "JSEngine.hpp"
#include <functional>
#include <litehtml.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class WebView;

// Simple Virtual DOM node representation
struct VNode
{
	std::string tag;
	std::map<std::string, std::string> attributes;
	std::map<std::string, std::string> properties;
	std::string textContent;
	std::vector<std::shared_ptr<VNode>> children;

	// Reference to corresponding litehtml element (if any)
	litehtml::element* htmlElement = nullptr;

	VNode(const std::string& tag = "")
		: tag(tag)
	{
	}
};

class VirtualDOM
{
public:
	VirtualDOM(WebView* webView);
	~VirtualDOM();

	// Initialize Snabbdom in the JavaScript engine
	bool initializeSnabbdom();

	// Convert litehtml document to virtual DOM
	std::shared_ptr<VNode> createVNodeFromLiteHTML(litehtml::element* element);

	// Apply virtual DOM changes back to litehtml
	bool applyChangesToLiteHTML(std::shared_ptr<VNode> oldNode,
		std::shared_ptr<VNode> newNode);

	// Execute a DOM update using Snabbdom
	bool updateDOM(const std::string& patchFunction);

	// Get the root virtual node
	std::shared_ptr<VNode> getRootNode() { return rootNode; }

	// Set up basic DOM API functions in JavaScript
	void setupDOMBindings();

	// DOM manipulation methods that work with litehtml
	litehtml::element::ptr findElementByIdInLiteHTML(const std::string& id);
	void appendElementToLiteHTML(litehtml::element::ptr parent,
		litehtml::element::ptr child);
	void triggerLiteHTMLRerender();
	void appendElementToDOM(const std::string& tagName,
		const std::string& textContent,
		const std::string& parentId = "");
	void updateElementTextContent(const std::string& elementId,
		const std::string& newText);

private:
	WebView* webView;
	JSEngine* jsEngine;  // owning elsewhere
	JSEngine* engine;    // convenience pointer (same as jsEngine)
	std::shared_ptr<VNode> rootNode;

	// Helper functions for the new simplified approach
	void registerCppCallbacks();
	void createDOMWithJavaScript();
	void setupDocumentProperties();
	void setupWindowHierarchy();
	void setupLocationObject();
	void setupLocalStorageObject();
	void defineElementAccessors();

	// Internal helpers for callbacks
	void cb_createElement();
	void cb_createTextNode();
	void cb_getElementById();
	void cb_querySelector();
	void cb_appendChild();
	void cb_updateTextContent();
	void cb_addEventListener();
	void cb_removeEventListener();
	void cb_textContentGetter();
	void cb_textContentSetter();
	void cb_innerHTMLGetter();
	void cb_innerHTMLSetter();
	void cb_documentTitleGetter();
	void cb_documentTitleSetter();
	void cb_locationHrefGetter();
	void cb_locationHrefSetter();
	void cb_locationReplace();
	void cb_locationReload();
	void cb_locationAssign();
	void cb_getCookies();
	void cb_setCookie();
	void cb_localStorage_getItem();
	void cb_localStorage_setItem();
	void cb_localStorage_removeItem();
	void cb_localStorage_clear();
	void cb_localStorage_key();
};

#endif // VIRTUAL_DOM_HPP
