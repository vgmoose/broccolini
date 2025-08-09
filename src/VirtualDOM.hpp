#ifndef VIRTUAL_DOM_HPP
#define VIRTUAL_DOM_HPP

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <litehtml.h>
#include "mujs.h"

// Forward declarations
class JSEngine;
class WebView;

// Simple Virtual DOM node representation
struct VNode {
    std::string tag;
    std::map<std::string, std::string> attributes;
    std::map<std::string, std::string> properties;
    std::string textContent;
    std::vector<std::shared_ptr<VNode>> children;
    
    // Reference to corresponding litehtml element (if any)
    litehtml::element* htmlElement = nullptr;
    
    VNode(const std::string& tag = "") : tag(tag) {}
};

class VirtualDOM {
public:
    VirtualDOM(WebView* webView);
    ~VirtualDOM();
    
    // Initialize Snabbdom in the JavaScript engine
    bool initializeSnabbdom();
    
    // Create a virtual node from JavaScript object
    std::shared_ptr<VNode> createVNodeFromJS(js_State* J, int index);
    
    // Convert litehtml document to virtual DOM
    std::shared_ptr<VNode> createVNodeFromLiteHTML(litehtml::element* element);
    
    // Apply virtual DOM changes back to litehtml
    bool applyChangesToLiteHTML(std::shared_ptr<VNode> oldNode, std::shared_ptr<VNode> newNode);
    
    // Execute a DOM update using Snabbdom
    bool updateDOM(const std::string& patchFunction);
    
    // Get the root virtual node
    std::shared_ptr<VNode> getRootNode() { return rootNode; }
    
    // Set up basic DOM API functions in JavaScript
    void setupDOMBindings();
    
    // DOM manipulation methods that work with litehtml
    litehtml::element::ptr findElementByIdInLiteHTML(const std::string& id);
    void appendElementToLiteHTML(litehtml::element::ptr parent, litehtml::element::ptr child);
    void triggerLiteHTMLRerender();
    void appendElementToDOM(const std::string& tagName, const std::string& textContent, const std::string& parentId = "");
    void updateElementTextContent(const std::string& elementId, const std::string& newText);
    
private:
    WebView* webView;
    JSEngine* jsEngine;
    std::shared_ptr<VNode> rootNode;
    
    // JavaScript binding functions
    static void js_createElement(js_State* J);
    static void js_createTextNode(js_State* J);
    static void js_getElementById(js_State* J);
    static void js_querySelector(js_State* J);
    static void js_appendChild(js_State* J);
    static void js_updateTextContent(js_State* J);
    
    // Helper functions
    std::shared_ptr<VNode> findNodeById(std::shared_ptr<VNode> node, const std::string& id);
    void traverseLiteHTMLElement(litehtml::element* element, std::shared_ptr<VNode> parent);
    
    // Get VirtualDOM instance from JavaScript context
    static VirtualDOM* getVirtualDOMFromJS(js_State* J, int index = -1);
};

#endif // VIRTUALDOM_HPP
