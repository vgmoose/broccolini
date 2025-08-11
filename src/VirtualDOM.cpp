#include "VirtualDOM.hpp"
#include "JSEngine.hpp"
#include "WebView.hpp"
#include "StorageManager.hpp"
#include "../utils/BrocContainer.hpp"
#include <litehtml.h>
#include <iostream>

VirtualDOM::VirtualDOM(WebView* webView) : webView(webView), jsEngine(nullptr) {
    rootNode = std::make_shared<VNode>("html");
}

VirtualDOM::~VirtualDOM() {
    // Cleanup is handled by smart pointers
}

bool VirtualDOM::initializeSnabbdom() {
    std::cout << "VirtualDOM::initializeSnabbdom() called" << std::endl;
    
    if (!webView || !webView->jsEngine) {
        std::cerr << "Error: JSEngine not available for VirtualDOM initialization" << std::endl;
        return false;
    }
    
    jsEngine = webView->jsEngine;
    std::cout << "VirtualDOM: JSEngine assigned" << std::endl;
    
    // TODO: Load Snabbdom library from libs/snabbdom/
    // For now, we'll implement a simple virtual DOM without Snabbdom
    // and add Snabbdom integration later
    
    std::cout << "VirtualDOM: Setting up DOM bindings" << std::endl;
    setupDOMBindings();
    std::cout << "VirtualDOM: DOM bindings complete" << std::endl;
    
    return true;
}

void VirtualDOM::setupDOMBindings() {
    std::cout << "VirtualDOM::setupDOMBindings() called" << std::endl;
    
    if (!jsEngine) {
        std::cout << "Error: No JSEngine available for DOM bindings" << std::endl;
        return;
    }
    
    std::cout << "VirtualDOM: Registering DOM functions" << std::endl;
    
    // Register DOM manipulation functions
    jsEngine->registerFunction("createElement", js_createElement);
    jsEngine->registerFunction("createTextNode", js_createTextNode);
    jsEngine->registerFunction("getElementById", js_getElementById);
    
    std::cout << "VirtualDOM: Creating document object" << std::endl;
    
    // Create a document object with enhanced functions
    js_State* J = jsEngine->getState();
    
    try {
        js_newobject(J);
        
        std::cout << "VirtualDOM: Adding createElement to document" << std::endl;
        js_newcfunction(J, js_createElement, "createElement", 1);
        js_setproperty(J, -2, "createElement");
        
        std::cout << "VirtualDOM: Adding createTextNode to document" << std::endl;
        js_newcfunction(J, js_createTextNode, "createTextNode", 1);
        js_setproperty(J, -2, "createTextNode");
        
        std::cout << "VirtualDOM: Adding getElementById to document" << std::endl;
        js_newcfunction(J, js_getElementById, "getElementById", 1);
        js_setproperty(J, -2, "getElementById");
        
        std::cout << "VirtualDOM: Adding querySelector to document" << std::endl;
        js_newcfunction(J, js_querySelector, "querySelector", 1);
        js_setproperty(J, -2, "querySelector");
        
        std::cout << "VirtualDOM: Setting document global" << std::endl;
        js_setglobal(J, "document");
        
        std::cout << "VirtualDOM: Document object created successfully" << std::endl;
        
        // Set up advanced property interceptors
        std::cout << "VirtualDOM: Setting up document properties" << std::endl;
        setupDocumentProperties(J);
        
        std::cout << "VirtualDOM: Setting up window object" << std::endl;
        setupWindowObject(J);
        
    } catch (...) {
        std::cout << "Error: Exception during document object creation" << std::endl;
        return;
    }
    
    std::cout << "VirtualDOM: Setup complete" << std::endl;
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
    if (js_gettop(J) < 1) {
        std::cout << "Warning: createElement called without arguments" << std::endl;
        js_pushnull(J);
        return;
    }
    
    const char* tagName = js_tostring(J, 1);
    if (!tagName) {
        std::cout << "Warning: createElement called with non-string argument" << std::endl;
        js_pushnull(J);
        return;
    }
    
    // Create a JavaScript object representing a DOM element
    js_newobject(J);
    js_pushstring(J, tagName);
    js_setproperty(J, -2, "tagName");
    
    // Store internal properties with underscore prefix
    js_pushstring(J, "");
    js_setproperty(J, -2, "_textContent");
    js_pushstring(J, "");
    js_setproperty(J, -2, "_innerHTML");
    js_pushstring(J, "");
    js_setproperty(J, -2, "id");
    
    // Add DOM manipulation methods
    js_newcfunction(J, js_appendChild, "appendChild", 1);
    js_setproperty(J, -2, "appendChild");
    
    // Add the updateTextContent method to each element
    js_newcfunction(J, js_updateTextContent, "updateTextContent", 1);
    js_setproperty(J, -2, "updateTextContent");
    
    // Add event listener methods
    js_newcfunction(J, js_addEventListener, "addEventListener", 2);
    js_setproperty(J, -2, "addEventListener");
    
    js_newcfunction(J, js_removeEventListener, "removeEventListener", 2);
    js_setproperty(J, -2, "removeEventListener");
    
    // Set up property interceptors for this element
    setupElementPropertySetters(J);
    
    std::cout << "Created element: " << tagName << std::endl;
}

void VirtualDOM::js_createTextNode(js_State* J) {
    if (js_gettop(J) < 1) {
        std::cout << "Warning: createTextNode called without arguments" << std::endl;
        js_pushnull(J);
        return;
    }
    
    const char* text = js_tostring(J, 1);
    if (!text) {
        std::cout << "Warning: createTextNode called with non-string argument" << std::endl;
        js_pushnull(J);
        return;
    }
    
    // Create a text node object
    js_newobject(J);
    js_pushstring(J, "#text");
    js_setproperty(J, -2, "tagName");
    js_pushstring(J, text);
    js_setproperty(J, -2, "textContent");
    js_pushstring(J, text);
    js_setproperty(J, -2, "nodeValue");
    
    std::cout << "Created text node: " << text << std::endl;
}

void VirtualDOM::js_getElementById(js_State* J) {
    if (js_gettop(J) < 1) {
        std::cout << "Warning: getElementById called without arguments" << std::endl;
        js_pushnull(J);
        return;
    }
    
    const char* id = js_tostring(J, 1);
    if (!id) {
        std::cout << "Warning: getElementById called with non-string argument" << std::endl;
        js_pushnull(J);
        return;
    }
    
    // Get VirtualDOM instance from JavaScript user data
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom) {
        std::cout << "Warning: VirtualDOM not available in getElementById" << std::endl;
        js_pushnull(J);
        return;
    }
    
    if (!vdom->webView || !vdom->webView->m_doc) {
        js_pushnull(J);
        return;
    }
    
    // Search for element in litehtml document
    auto element = vdom->findElementByIdInLiteHTML(id);
    if (element) {
        // Create JavaScript object representing the found element
        js_newobject(J);
        js_pushstring(J, element->get_tagName());
        js_setproperty(J, -2, "tagName");
        js_pushstring(J, id);
        js_setproperty(J, -2, "id");
        
        // Store internal properties with underscore prefix
        js_pushstring(J, "");
        js_setproperty(J, -2, "_textContent");
        js_pushstring(J, "");
        js_setproperty(J, -2, "_innerHTML");
        
        // Add DOM manipulation methods
        js_newcfunction(J, js_appendChild, "appendChild", 1);
        js_setproperty(J, -2, "appendChild");
        
        // Add method that JavaScript can use for setting text content
        js_newcfunction(J, js_updateTextContent, "updateTextContent", 1);
        js_setproperty(J, -2, "updateTextContent");
        
        // Add event listener methods
        js_newcfunction(J, js_addEventListener, "addEventListener", 2);
        js_setproperty(J, -2, "addEventListener");
        
        js_newcfunction(J, js_removeEventListener, "removeEventListener", 2);
        js_setproperty(J, -2, "removeEventListener");
        
        // Set up property interceptors for this element
        setupElementPropertySetters(J);
        
        std::cout << "Found element by ID: " << id << std::endl;
    } else {
        js_pushnull(J);
        std::cout << "Element not found by ID: " << id << std::endl;
    }
}

void VirtualDOM::js_querySelector(js_State* J) {
    const char* selector = js_tostring(J, 1);
    
    // For now, implement basic ID selector (#id) and tag selector
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom) {
        js_pushnull(J);
        return;
    }
    
    std::string sel(selector);
    if (sel[0] == '#') {
        // ID selector
        std::string id = sel.substr(1);
        // Reuse getElementById logic
        js_pushstring(J, id.c_str());
        js_getElementById(J);
        return;
    }
    
    // For now, return null for other selectors
    js_pushnull(J);
    std::cout << "querySelector not fully implemented for: " << selector << std::endl;
}

void VirtualDOM::js_appendChild(js_State* J) {
    // Get the child element to append
    if (!js_isobject(J, 1)) {
        js_pushundefined(J);
        return;
    }
    
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom || !vdom->webView || !vdom->webView->m_doc) {
        js_pushundefined(J);
        return;
    }
    
    // Get parent element ID (from 'this' context)
    std::string parentId = "";
    if (js_hasproperty(J, 0, "id")) {
        js_getproperty(J, 0, "id");
        const char* id = js_tostring(J, -1);
        if (id) parentId = id;
        js_pop(J, 1);
    }
    
    // Get child element info
    js_getproperty(J, 1, "tagName");
    const char* childTag = js_tostring(J, -1);
    js_pop(J, 1);
    
    js_getproperty(J, 1, "textContent");
    const char* childText = js_tostring(J, -1);
    js_pop(J, 1);
    
    if (childTag && strlen(childTag) > 0) {
        // This is where we'll create actual litehtml elements and append them
        std::cout << "Attempting to appendChild: " << childTag;
        if (childText && strlen(childText) > 0) {
            std::cout << " with text: " << childText;
        }
        std::cout << std::endl;
        
        // For now, we'll implement this as a DOM manipulation that triggers re-render
        vdom->appendElementToDOM(childTag, childText, parentId);
    }
    
    // Return the appended child
    js_copy(J, 1);
}

// Helper method to update text content of an element
void VirtualDOM::js_updateTextContent(js_State* J) {
    if (js_gettop(J) < 1) {
        js_pushundefined(J);
        return;
    }
    
    const char* newText = js_tostring(J, 1);
    if (!newText) {
        js_pushundefined(J);
        return;
    }
    
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom || !vdom->webView) {
        js_pushundefined(J);
        return;
    }
    
    // Get the element ID from 'this' context
    std::string elementId = "";
    if (js_hasproperty(J, 0, "id")) {
        js_getproperty(J, 0, "id");
        const char* id = js_tostring(J, -1);
        if (id) elementId = id;
        js_pop(J, 1);
    }
    
    if (!elementId.empty()) {
        std::cout << "Updating text content of element '" << elementId << "' to: " << newText << std::endl;
        vdom->updateElementTextContent(elementId, newText);
    }
    
    js_pushundefined(J);
}

// Helper method to actually append to DOM
void VirtualDOM::appendElementToDOM(const std::string& tagName, const std::string& textContent, const std::string& parentId) {
    if (!webView || !webView->m_doc) return;
    
    // Create the new HTML element
    std::string newElement = "<" + tagName + ">" + textContent + "</" + tagName + ">";
    
    std::string& html = webView->contents;
    
    if (!parentId.empty()) {
        // Try to find the specific parent element by ID
        std::string idPattern = "id=\"" + parentId + "\"";
        size_t idPos = html.find(idPattern);
        
        if (idPos != std::string::npos) {
            // Find the end of the opening tag
            size_t tagStart = html.rfind('<', idPos);
            if (tagStart != std::string::npos) {
                size_t tagEnd = html.find('>', idPos);
                if (tagEnd != std::string::npos) {
                    // Find the corresponding closing tag
                    std::string openTag = html.substr(tagStart, tagEnd - tagStart + 1);
                    std::string tagNameOnly = openTag.substr(1, openTag.find(' ') - 1);
                    if (tagNameOnly.find(' ') == std::string::npos && tagNameOnly.find('>') != std::string::npos) {
                        tagNameOnly = tagNameOnly.substr(0, tagNameOnly.find('>'));
                    }
                    
                    std::string closingTag = "</" + tagNameOnly + ">";
                    size_t closePos = html.find(closingTag, tagEnd);
                    
                    if (closePos != std::string::npos) {
                        // Insert the new element just before the closing tag
                        html.insert(closePos, newElement);
                        
                        std::cout << "Appended " << tagName << " to parent element with ID: " << parentId << std::endl;
                        triggerLiteHTMLRerender();
                        return;
                    }
                }
            }
        }
        
        std::cout << "Could not find parent element with ID: " << parentId << ", falling back to body" << std::endl;
    }
    
    // Fallback: append to body
    size_t bodyEnd = html.find("</body>");
    if (bodyEnd != std::string::npos) {
        html.insert(bodyEnd, newElement);
        
        std::cout << "Modified HTML contents. First 500 chars:" << std::endl;
        std::cout << html.substr(0, 500) << std::endl;
        
        // Trigger a re-render
        triggerLiteHTMLRerender();
        
        std::cout << "Appended " << tagName << " to DOM and triggered re-render" << std::endl;
    } else {
        std::cout << "Could not find </body> tag to append element" << std::endl;
        std::cout << "HTML contents preview: " << html.substr(0, 200) << std::endl;
    }
}

// Helper method to update text content of an existing element
void VirtualDOM::updateElementTextContent(const std::string& elementId, const std::string& newText) {
    if (!webView) return;
    
    std::string& html = webView->contents;
    
    // Find the element with the given ID
    std::string idPattern = "id=\"" + elementId + "\"";
    size_t idPos = html.find(idPattern);
    
    if (idPos != std::string::npos) {
        // Find the opening tag
        size_t tagStart = html.rfind('<', idPos);
        if (tagStart != std::string::npos) {
            size_t tagEnd = html.find('>', idPos);
            if (tagEnd != std::string::npos) {
                // Find the closing tag
                std::string openTag = html.substr(tagStart, tagEnd - tagStart + 1);
                
                // Extract the tag name
                size_t spacePos = openTag.find(' ');
                size_t closePos = openTag.find('>');
                std::string tagName;
                if (spacePos != std::string::npos && spacePos < closePos) {
                    tagName = openTag.substr(1, spacePos - 1);
                } else {
                    tagName = openTag.substr(1, closePos - 1);
                }
                
                std::string closingTag = "</" + tagName + ">";
                size_t closeTagPos = html.find(closingTag, tagEnd);
                
                if (closeTagPos != std::string::npos) {
                    // Replace the content between opening and closing tags
                    size_t contentStart = tagEnd + 1;
                    size_t contentLength = closeTagPos - contentStart;
                    
                    html.replace(contentStart, contentLength, newText);
                    
                    std::cout << "Updated text content of element '" << elementId << "' to: " << newText << std::endl;
                    triggerLiteHTMLRerender();
                    return;
                }
            }
        }
    }
    
    std::cout << "Could not find element with ID: " << elementId << std::endl;
}

// Helper methods for litehtml integration
litehtml::element::ptr VirtualDOM::findElementByIdInLiteHTML(const std::string& id) {
    if (!webView || !webView->m_doc) {
        return nullptr;
    }
    
    // Use litehtml's select_one method on the root element
    std::string selector = "#" + id;
    auto root = webView->m_doc->root();
    if (root) {
        return root->select_one(selector);
    }
    return nullptr;
}

void VirtualDOM::appendElementToLiteHTML(litehtml::element::ptr parent, litehtml::element::ptr child) {
    if (!parent || !child) return;
    
    // Use litehtml's appendChild method
    parent->appendChild(child);
    
    // Trigger re-render
    triggerLiteHTMLRerender();
}

void VirtualDOM::triggerLiteHTMLRerender() {
    if (!webView) return;
    
    // Force a complete document recreation from the modified HTML
    webView->recreateDocument();
    
    std::cout << "Triggered complete litehtml document recreation" << std::endl;
}

VirtualDOM* VirtualDOM::getVirtualDOMFromJS(js_State* J, int idx) {
    // Get the JSEngine from the context
    void* context = js_getcontext(J);
    if (context) {
        JSEngine* jsEngine = static_cast<JSEngine*>(context);
        if (jsEngine && jsEngine->getWebView()) {
            return jsEngine->getWebView()->virtualDOM;
        }
    }
    
    return nullptr;
}

// Location object setup
void VirtualDOM::setupLocationObject(js_State* J) {
    std::cout << "VirtualDOM: Setting up location object" << std::endl;
    
    // Create location object
    js_newobject(J);
    
    // Set up href property with getter/setter
    js_newcfunction(J, js_locationHrefGetter, "get", 0); 
    js_newcfunction(J, js_locationHrefSetter, "set", 1); 
    js_defaccessor(J, -3, "href", 0); 
    
    // Add location methods
    js_newcfunction(J, js_locationReplace, "replace", 1);
    js_setproperty(J, -2, "replace");
    
    js_newcfunction(J, js_locationReload, "reload", 0);
    js_setproperty(J, -2, "reload");
    
    js_newcfunction(J, js_locationAssign, "assign", 1);
    js_setproperty(J, -2, "assign");
    
    // Set location in window (assuming window is at -2)
    js_setproperty(J, -2, "location");
    
    std::cout << "VirtualDOM: Location object created successfully" << std::endl;
}

void VirtualDOM::setupWindowHierarchy(js_State* J) {
    std::cout << "VirtualDOM: Setting up window hierarchy" << std::endl;
    
    // For a simple browser, window.parent should reference the same window
    // (unless we're in a frame, which we'll handle later)
    
    // Get reference to the window object (should be at -1)
    js_copy(J, -1); // duplicate window object
    js_setproperty(J, -2, "parent"); // window.parent = window
    
    js_copy(J, -1); // duplicate window object again
    js_setproperty(J, -2, "top"); // window.top = window
    
    js_copy(J, -1); // duplicate window object again
    js_setproperty(J, -2, "self"); // window.self = window
    
    std::cout << "VirtualDOM: Window hierarchy set up successfully" << std::endl;
}

// Property setter system implementation
void VirtualDOM::setupElementPropertySetters(js_State* J) {
    // Set up textContent property with getter/setter using js_defaccessor
    js_newcfunction(J, js_textContentGetter, "get", 0); // getter at -2
    js_newcfunction(J, js_textContentSetter, "set", 1); // setter at -1
    js_defaccessor(J, -3, "textContent", 0); // object at -3
    
    // Set up innerHTML property with getter/setter using js_defaccessor
    js_newcfunction(J, js_innerHTMLGetter, "get", 0); // getter at -2
    js_newcfunction(J, js_innerHTMLSetter, "set", 1); // setter at -1
    js_defaccessor(J, -3, "innerHTML", 0); // object at -3
}

void VirtualDOM::setupWindowObject(js_State* J) {
    std::cout << "VirtualDOM: Setting up window object" << std::endl;
    
    // Create window object
    js_newobject(J);
    
    // Add document reference to window
    js_getglobal(J, "document");
    js_setproperty(J, -2, "document");
    
    // Add alert function
    js_newcfunction(J, [](js_State *J) {
        const char* message = js_tostring(J, 1);
        std::cout << "[ALERT] " << (message ? message : "undefined") << std::endl;
    }, "alert", 1);
    js_setproperty(J, -2, "alert");
    
    // Add setTimeout function
    js_newcfunction(J, [](js_State *J) {
        // Simple setTimeout implementation - just execute the function immediately
        // In a real implementation, you'd schedule this with a timer
        if (js_iscallable(J, 1)) {
            std::cout << "setTimeout called - executing immediately (simplified implementation)" << std::endl;
            js_copy(J, 1); // copy the function
            js_call(J, 0); // call it with no arguments
            js_pop(J, 1); // pop the result
        } else {
            std::cout << "setTimeout called with non-callable first argument" << std::endl;
        }
        js_pushundefined(J);
    }, "setTimeout", 2);
    js_setproperty(J, -2, "setTimeout");
    
    // Add console reference to window
    js_getglobal(J, "console");
    js_setproperty(J, -2, "console");
    
    // Set up location object and add it to window
    setupLocationObject(J);
    
    // Set up localStorage object and add it to window
    js_newobject(J); // Create localStorage object
    
    // Add localStorage methods
    js_newcfunction(J, js_localStorage_getItem, "getItem", 1);
    js_setproperty(J, -2, "getItem");
    
    js_newcfunction(J, js_localStorage_setItem, "setItem", 2);
    js_setproperty(J, -2, "setItem");
    
    js_newcfunction(J, js_localStorage_removeItem, "removeItem", 1);
    js_setproperty(J, -2, "removeItem");
    
    js_newcfunction(J, js_localStorage_clear, "clear", 0);
    js_setproperty(J, -2, "clear");
    
    js_newcfunction(J, js_localStorage_key, "key", 1);
    js_setproperty(J, -2, "key");
    
    // Add localStorage length property (as a simple property for now)
    js_pushnumber(J, 0);
    js_setproperty(J, -2, "length");
    
    js_setproperty(J, -2, "localStorage"); // Add to window
    
    // Set up window hierarchy (window.parent, etc.)
    setupWindowHierarchy(J);
    
    // Set window as global
    js_setglobal(J, "window");
    
    // Now set window properties as globals for convenience
    std::cout << "VirtualDOM: Setting up global window properties" << std::endl;
    
    // Make alert available globally
    js_getglobal(J, "window");
    js_getproperty(J, -1, "alert");
    js_setglobal(J, "alert");
    
    // Make setTimeout available globally
    js_getproperty(J, -1, "setTimeout");
    js_setglobal(J, "setTimeout");
    
    // Make location available globally
    js_getproperty(J, -1, "location");
    js_setglobal(J, "location");
    
    // Make localStorage available globally
    js_getproperty(J, -1, "localStorage");
    js_setglobal(J, "localStorage");
    
    js_pop(J, 1); // pop window object
    
    std::cout << "VirtualDOM: Window object created successfully" << std::endl;
}

void VirtualDOM::setupDocumentProperties(js_State* J) {
    std::cout << "VirtualDOM: Setting up document properties" << std::endl;
    
    js_getglobal(J, "document");
    
    // Set up document.title property with getter/setter using js_defaccessor
    js_newcfunction(J, js_documentTitleGetter, "get", 0); // getter at -2
    js_newcfunction(J, js_documentTitleSetter, "set", 1); // setter at -1
    js_defaccessor(J, -3, "title", 0); // object at -3
    
    // Set up document.cookie property with getter/setter
    js_newcfunction(J, js_getCookies, "get", 0); // getter at -2
    js_newcfunction(J, js_setCookie, "set", 1); // setter at -1
    js_defaccessor(J, -3, "cookie", 0); // object at -3
    
    js_pop(J, 1); // pop document object
    
    std::cout << "VirtualDOM: Document properties set up successfully" << std::endl;
}

// Location object methods and properties
void VirtualDOM::js_locationHrefGetter(js_State* J) {
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (vdom && vdom->webView) {
        js_pushstring(J, vdom->webView->url.c_str());
    } else {
        js_pushstring(J, "");
    }
}

void VirtualDOM::js_locationHrefSetter(js_State* J) {
    const char* newUrl = js_tostring(J, 1);
    std::cout << "Property assignment intercepted: location.href = \"" << newUrl << "\"" << std::endl;
    
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (vdom && vdom->webView && newUrl) {
        std::cout << "Navigating to: " << newUrl << std::endl;
        vdom->webView->goTo(newUrl);
    }
}

void VirtualDOM::js_locationReplace(js_State* J) {
    const char* newUrl = js_tostring(J, 1);
    std::cout << "Method called: location.replace(\"" << (newUrl ? newUrl : "undefined") << "\")" << std::endl;
    
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (vdom && vdom->webView && newUrl) {
        std::cout << "Replacing location with: " << newUrl << std::endl;
        
        // Set navigation flag to prevent button overlay issues
        if (vdom->webView->container) {
            vdom->webView->container->navigationInProgress = true;
        }
        
        // TODO: don't update history for replace
        vdom->webView->goTo(newUrl);
    }
    
    js_pushundefined(J);
}

void VirtualDOM::js_locationReload(js_State* J) {
    std::cout << "Method called: location.reload()" << std::endl;
    
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (vdom && vdom->webView) {
        std::cout << "Reloading current page" << std::endl;
        vdom->webView->reloadPage();
    }
    
    js_pushundefined(J);
}

void VirtualDOM::js_locationAssign(js_State* J) {
    const char* newUrl = js_tostring(J, 1);
    std::cout << "Method called: location.assign(\"" << (newUrl ? newUrl : "undefined") << "\")" << std::endl;
    
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (vdom && vdom->webView && newUrl) {
        std::cout << "Assigning location to: " << newUrl << std::endl;
        vdom->webView->goTo(newUrl);
    }
    
    js_pushundefined(J);
}

// Property getter/setter implementations
void VirtualDOM::js_textContentGetter(js_State* J) {
    // Get the internal _textContent property
    if (js_hasproperty(J, 0, "_textContent")) {
        js_getproperty(J, 0, "_textContent");
    } else {
        js_pushstring(J, "");
    }
}

void VirtualDOM::js_textContentSetter(js_State* J) {
    const char* value = js_tostring(J, 1);
    std::cout << "Property assignment intercepted: textContent = \"" << value << "\"" << std::endl;
    
    // Store in internal property
    js_pushstring(J, value);
    js_setproperty(J, 0, "_textContent");
    
    // Get element ID if available and trigger DOM update
    if (js_hasproperty(J, 0, "id")) {
        js_getproperty(J, 0, "id");
        const char* elementId = js_tostring(J, -1);
        js_pop(J, 1);
        
        if (elementId && strlen(elementId) > 0) {
            std::cout << "Triggering DOM update for element '" << elementId << "'" << std::endl;
            
            // Get VirtualDOM instance and update
            VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
            if (vdom) {
                vdom->updateElementTextContent(elementId, value);
                
                // Trigger document recreation
                if (vdom->webView) {
                    vdom->webView->recreateDocument();
                }
            }
        }
    }
}

void VirtualDOM::js_innerHTMLGetter(js_State* J) {
    // Get the internal _innerHTML property
    if (js_hasproperty(J, 0, "_innerHTML")) {
        js_getproperty(J, 0, "_innerHTML");
    } else {
        js_pushstring(J, "");
    }
}

void VirtualDOM::js_innerHTMLSetter(js_State* J) {
    const char* value = js_tostring(J, 1);
    std::cout << "Property assignment intercepted: innerHTML = \"" << value << "\"" << std::endl;
    
    // Store in internal property
    js_pushstring(J, value);
    js_setproperty(J, 0, "_innerHTML");
    
    // Get element ID if available and trigger DOM update
    if (js_hasproperty(J, 0, "id")) {
        js_getproperty(J, 0, "id");
        const char* elementId = js_tostring(J, -1);
        js_pop(J, 1);
        
        if (elementId && strlen(elementId) > 0) {
            std::cout << "Triggering innerHTML update for element '" << elementId << "'" << std::endl;
            
            // Get VirtualDOM instance and update
            VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
            if (vdom) {
                // For innerHTML, we'd need more sophisticated parsing
                // For now, treat it like textContent
                vdom->updateElementTextContent(elementId, value);
                
                // Trigger document recreation
                if (vdom->webView) {
                    vdom->webView->recreateDocument();
                }
            }
        }
    }
}

void VirtualDOM::js_documentTitleGetter(js_State* J) {
    // Get current document title from WebView
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (vdom && vdom->webView) {
        js_pushstring(J, vdom->webView->windowTitle.c_str());
    } else {
        js_pushstring(J, "");
    }
}

void VirtualDOM::js_documentTitleSetter(js_State* J) {
    const char* title = js_tostring(J, 1);
    std::cout << "Property assignment intercepted: document.title = \"" << title << "\"" << std::endl;
    
    // Get VirtualDOM instance and update title
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (vdom && vdom->webView) {
        std::cout << "Setting document title to: " << title << std::endl;
        vdom->webView->setTitle(title);
    }
}

// Event listener implementations
void VirtualDOM::js_addEventListener(js_State* J) {
    // Check argument count
    int argc = js_gettop(J);
    if (argc < 3) {  // 'this' + eventType + handler
        std::cout << "addEventListener called with insufficient arguments" << std::endl;
        return;
    }
    
    // Get the event type
    const char* eventType = js_tostring(J, 1);
    if (!eventType) {
        std::cout << "addEventListener: event type must be a string" << std::endl;
        return;
    }
    
    // Get the handler function or string
    std::string handlerFunction;
    bool isFunction = false;
    
    if (js_iscallable(J, 2)) {
        // Function handler - convert to string representation for storage
        // For now, we'll generate a unique function name and store the function
        static int functionCounter = 0;
        std::string funcName = "_eventHandler_" + std::to_string(++functionCounter);
        
        // Copy the function to the top of the stack, then store it globally
        js_copy(J, 2);  // Copy the function
        js_setglobal(J, funcName.c_str());
        
        handlerFunction = funcName;
        isFunction = true;
    } else if (js_isstring(J, 2)) {
        // String handler (inline code)
        handlerFunction = js_tostring(J, 2);
        isFunction = false;
    } else {
        std::cout << "addEventListener: handler must be a function or string" << std::endl;
        return;
    }
    
    // Get the element's ID to find it in the DOM
    js_getproperty(J, 0, "id");
    const char* elementId = nullptr;
    if (js_isstring(J, -1)) {
        elementId = js_tostring(J, -1);
    }
    js_pop(J, 1);
    
    // Get VirtualDOM instance
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom || !vdom->webView || !vdom->webView->container) {
        std::cout << "addEventListener: VirtualDOM or container not available" << std::endl;
        return;
    }
    
    // Find the litehtml element
    litehtml::element::ptr element = nullptr;
    if (elementId && strlen(elementId) > 0) {
        element = vdom->findElementByIdInLiteHTML(elementId);
    }
    
    if (!element) {
        std::cout << "addEventListener: Could not find element with ID '" << (elementId ? elementId : "") << "'" << std::endl;
        return;
    }
    
    std::cout << "Adding event listener for '" << eventType << "' on element with ID '" << elementId << "'" << std::endl;
    
    // Add the event listener through BrocContainer
    vdom->webView->container->addEventListener(element, eventType, handlerFunction, isFunction);
}

void VirtualDOM::js_removeEventListener(js_State* J) {
    // Check argument count
    int argc = js_gettop(J);
    if (argc < 2) {  // 'this' + eventType
        std::cout << "removeEventListener called with insufficient arguments" << std::endl;
        return;
    }
    
    // Get the event type
    const char* eventType = js_tostring(J, 1);
    if (!eventType) {
        std::cout << "removeEventListener: event type must be a string" << std::endl;
        return;
    }
    
    // Get optional handler function (if provided)
    std::string handlerFunction = "";
    if (argc >= 3 && js_isstring(J, 2)) {
        handlerFunction = js_tostring(J, 2);
    }
    
    // Get the element's ID to find it in the DOM
    js_getproperty(J, 0, "id");
    const char* elementId = nullptr;
    if (js_isstring(J, -1)) {
        elementId = js_tostring(J, -1);
    }
    js_pop(J, 1);
    
    // Get VirtualDOM instance
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom || !vdom->webView || !vdom->webView->container) {
        std::cout << "removeEventListener: VirtualDOM or container not available" << std::endl;
        return;
    }
    
    // Find the litehtml element
    litehtml::element::ptr element = nullptr;
    if (elementId && strlen(elementId) > 0) {
        element = vdom->findElementByIdInLiteHTML(elementId);
    }
    
    if (!element) {
        std::cout << "removeEventListener: Could not find element with ID '" << (elementId ? elementId : "") << "'" << std::endl;
        return;
    }
    
    std::cout << "Removing event listener for '" << eventType << "' on element with ID '" << elementId << "'" << std::endl;
    
    // Remove the event listener through BrocContainer
    vdom->webView->container->removeEventListener(element, eventType, handlerFunction);
}

// Cookie Storage JavaScript APIs
void VirtualDOM::js_getCookies(js_State* J) {
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom) {
        js_pushstring(J, "");
        return;
    }
    
    std::string cookies = StorageManager::getInstance().getAllCookiesAsString();
    js_pushstring(J, cookies.c_str());
}

void VirtualDOM::js_setCookie(js_State* J) {
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom) {
        return;
    }
    
    if (js_isstring(J, 1)) {
        std::string cookieString = js_tostring(J, 1);
        StorageManager::getInstance().setCookieFromString(cookieString);
    }
}

// localStorage JavaScript APIs
void VirtualDOM::js_localStorage_getItem(js_State* J) {
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom) {
        js_pushnull(J);
        return;
    }
    
    if (js_isstring(J, 1)) {
        std::string key = js_tostring(J, 1);
        std::string value = StorageManager::getInstance().getLocalStorageItem(key);
        if (value.empty()) {
            js_pushnull(J);
        } else {
            js_pushstring(J, value.c_str());
        }
    } else {
        js_pushnull(J);
    }
}

void VirtualDOM::js_localStorage_setItem(js_State* J) {
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom) {
        return;
    }
    
    if (js_isstring(J, 1) && js_isstring(J, 2)) {
        std::string key = js_tostring(J, 1);
        std::string value = js_tostring(J, 2);
        StorageManager::getInstance().setLocalStorageItem(key, value);
    }
}

void VirtualDOM::js_localStorage_removeItem(js_State* J) {
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom) {
        return;
    }
    
    if (js_isstring(J, 1)) {
        std::string key = js_tostring(J, 1);
        StorageManager::getInstance().removeLocalStorageItem(key);
    }
}

void VirtualDOM::js_localStorage_clear(js_State* J) {
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom) {
        return;
    }
    
    StorageManager::getInstance().clearLocalStorage();
}

void VirtualDOM::js_localStorage_key(js_State* J) {
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom) {
        js_pushnull(J);
        return;
    }
    
    if (js_isnumber(J, 1)) {
        int index = js_tonumber(J, 1);
        std::string key = StorageManager::getInstance().getLocalStorageKey(index);
        if (key.empty()) {
            js_pushnull(J);
        } else {
            js_pushstring(J, key.c_str());
        }
    } else {
        js_pushnull(J);
    }
}

void VirtualDOM::js_localStorage_length(js_State* J) {
    VirtualDOM* vdom = getVirtualDOMFromJS(J, 0);
    if (!vdom) {
        js_pushnumber(J, 0);
        return;
    }
    
    int length = StorageManager::getInstance().getLocalStorageLength();
    js_pushnumber(J, length);
}
