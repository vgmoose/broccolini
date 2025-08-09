#include "VirtualDOM.hpp"
#include "WebView.hpp"
#include <litehtml.h>
#include <iostream>

VirtualDOM::VirtualDOM(WebView* webView) : webView(webView), jsEngine(nullptr) {
    rootNode = std::make_shared<VNode>("html");
}

VirtualDOM::~VirtualDOM() {
    // Cleanup is handled by smart pointers
}

bool VirtualDOM::initializeSnabbdom() {
    if (!webView || !webView->jsEngine) {
        std::cerr << "Error: JSEngine not available for VirtualDOM initialization" << std::endl;
        return false;
    }
    
    jsEngine = webView->jsEngine;
    
    // TODO: Load Snabbdom library from libs/snabbdom/
    // For now, we'll implement a simple virtual DOM without Snabbdom
    // and add Snabbdom integration later
    
    setupDOMBindings();
    
    return true;
}

void VirtualDOM::setupDOMBindings() {
    if (!jsEngine) return;
    
    // Register basic DOM creation functions
    jsEngine->registerFunction("createElement", js_createElement);
    jsEngine->registerFunction("createTextNode", js_createTextNode);
    jsEngine->registerFunction("getElementById", js_getElementById);
    
    // Create a simple document object with basic functions
    js_State* J = jsEngine->getState();
    js_newobject(J);
    {
        js_newcfunction(J, js_createElement, "createElement", 1);
        js_setproperty(J, -2, "createElement");
        
        js_newcfunction(J, js_createTextNode, "createTextNode", 1);
        js_setproperty(J, -2, "createTextNode");
        
        js_newcfunction(J, js_getElementById, "getElementById", 1);
        js_setproperty(J, -2, "getElementById");
    }
    js_setglobal(J, "document");
}

std::shared_ptr<VNode> VirtualDOM::createVNodeFromJS(js_State* J, int index) {
    if (!js_isobject(J, index)) {
        return nullptr;
    }
    
    auto node = std::make_shared<VNode>();
    
    // Get tag name
    js_getproperty(J, index, "tagName");
    if (js_isstring(J, -1)) {
        node->tag = js_tostring(J, -1);
    }
    js_pop(J, 1);
    
    // Get text content
    js_getproperty(J, index, "textContent");
    if (js_isstring(J, -1)) {
        node->textContent = js_tostring(J, -1);
    }
    js_pop(J, 1);
    
    // TODO: Handle attributes and children
    
    return node;
}

std::shared_ptr<VNode> VirtualDOM::createVNodeFromLiteHTML(litehtml::element* element) {
    if (!element) return nullptr;
    
    auto node = std::make_shared<VNode>();
    node->htmlElement = element;
    
    // Get tag name
    auto tagName = element->get_tagName();
    if (tagName) {
        node->tag = tagName;
    }
    
    // Get text content for text nodes
    if (node->tag.empty()) {
        // This might be a text node
        // Note: litehtml handles text differently, we might need to adapt this
    }
    
    // TODO: Extract attributes and traverse children
    // This is a simplified version - we'll need to handle the full litehtml element tree
    
    return node;
}

bool VirtualDOM::applyChangesToLiteHTML(std::shared_ptr<VNode> oldNode, std::shared_ptr<VNode> newNode) {
    // This is where we would implement the actual DOM diffing and patching
    // For now, this is a placeholder that indicates where the magic happens
    
    if (!oldNode || !newNode) return false;
    
    // Compare nodes and apply changes
    if (oldNode->tag != newNode->tag) {
        // Node type changed - need to replace
        std::cout << "VirtualDOM: Node type changed from " << oldNode->tag << " to " << newNode->tag << std::endl;
        return true;
    }
    
    if (oldNode->textContent != newNode->textContent) {
        // Text content changed
        std::cout << "VirtualDOM: Text content changed" << std::endl;
        return true;
    }
    
    // TODO: Compare attributes and children recursively
    
    return false;
}

bool VirtualDOM::updateDOM(const std::string& patchFunction) {
    if (!jsEngine) return false;
    
    // Execute the patch function in JavaScript
    std::string script = patchFunction + "();";
    return jsEngine->executeScript(script);
}

std::shared_ptr<VNode> VirtualDOM::findNodeById(std::shared_ptr<VNode> node, const std::string& id) {
    if (!node) return nullptr;
    
    // Check if this node has the matching id
    auto it = node->attributes.find("id");
    if (it != node->attributes.end() && it->second == id) {
        return node;
    }
    
    // Search children
    for (auto& child : node->children) {
        auto found = findNodeById(child, id);
        if (found) return found;
    }
    
    return nullptr;
}

// JavaScript binding implementations
void VirtualDOM::js_createElement(js_State* J) {
    const char* tagName = js_tostring(J, 1);
    
    // Create a simple JavaScript object representing a DOM element
    js_newobject(J);
    js_pushstring(J, tagName);
    js_setproperty(J, -2, "tagName");
    js_pushstring(J, "");
    js_setproperty(J, -2, "textContent");
    
    // Add basic methods
    // TODO: Add appendChild, setAttribute, etc.
}

void VirtualDOM::js_createTextNode(js_State* J) {
    const char* text = js_tostring(J, 1);
    
    // Create a text node object
    js_newobject(J);
    js_pushstring(J, "#text");
    js_setproperty(J, -2, "tagName");
    js_pushstring(J, text);
    js_setproperty(J, -2, "textContent");
}

void VirtualDOM::js_getElementById(js_State* J) {
    const char* id = js_tostring(J, 1);
    
    // For now, return null - we'll implement this properly later
    // when we have full DOM tree synchronization
    js_pushnull(J);
    
    std::cout << "getElementById called for: " << id << std::endl;
}
