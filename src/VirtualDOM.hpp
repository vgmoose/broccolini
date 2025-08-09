#ifndef VIRTUALDOM_HPP
#define VIRTUALDOM_HPP

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "JSEngine.hpp"

// Forward declarations
class WebView;
namespace litehtml {
    class element;
    class document;
}

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
    
private:
    WebView* webView;
    JSEngine* jsEngine;
    std::shared_ptr<VNode> rootNode;
    
    // JavaScript binding functions
    static void js_createElement(js_State* J);
    static void js_createTextNode(js_State* J);
    static void js_getElementById(js_State* J);
    
    // Helper functions
    std::shared_ptr<VNode> findNodeById(std::shared_ptr<VNode> node, const std::string& id);
    void traverseLiteHTMLElement(litehtml::element* element, std::shared_ptr<VNode> parent);
};

#endif // VIRTUALDOM_HPP
