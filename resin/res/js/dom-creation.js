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
			_eventListeners: createMapLike(),
			_props: {},
			_attrs: {}
		};
		
		// Store in our tracking object
		elementVNodes[elementKey] = vnode;
	
	// textContent property with getter/setter that patches through Snabbdom
	// MuJS-compatible property definition
	try {
		Object.defineProperty(element, 'textContent', {
			get: function() {
				return this._textContent || "";
			},
			set: function(value) {
				console.log('[textContent setter] Setting text:', value, 'on element:', this.key);
				this._textContent = value;
				
				console.log('[textContent setter] Step 1: Creating props/attrs');
				// Create new vnode with updated text and patch it
				var props = this._props ? this._props : {};
				var attrs = this._attrs ? this._attrs : {};
				
				console.log('[textContent setter] Step 2: Checking event listeners');
				var events = (this._eventListeners && this._eventListeners.size > 0) ? mapToObject(this._eventListeners) : {};
				
				console.log('[textContent setter] Step 3: Creating vnode');
				var newVnode = window.h(this.tagName.toLowerCase(), { 
					key: this.key,
					props: props,
					attrs: attrs,
					on: events
				}, value);
				
				console.log('[textContent setter] Step 4: Updating tracking');
				// Update our tracking
				var oldVnode = elementVNodes[this.key] || this._vnode;
				elementVNodes[this.key] = newVnode;
				
				console.log('[textContent setter] Step 5: About to patch');
				// Patch through Snabbdom (this will trigger our litehtml module)
				if (window.snabbdomPatch) {
					try {
						var patchResult;
						if (window.currentVTree) {
							console.log('[textContent setter] Step 5a: Patching existing tree');
							patchResult = window.snabbdomPatch(oldVnode, newVnode);
						} else {
							console.log('[textContent setter] Step 5b: Initializing tree');
							// For first patch, use empty vnode or the element's stored vnode
							var initialVnode = this._vnode || window.h('div', { key: this.key }, '');
							patchResult = window.snabbdomPatch(initialVnode, newVnode);
							window.currentVTree = newVnode;
						}
						console.log('[textContent setter] Step 5c: Patch completed, result type:', typeof patchResult);
						// Only update currentVTree if we got a valid result
						if (patchResult && typeof patchResult === 'object') {
							window.currentVTree = patchResult;
						}
					} catch (patchError) {
						console.log('[textContent setter] Step 5 ERROR:', patchError.toString());
						// Don't rethrow since the DOM update was successful, just log the error
					}
				}
				console.log('[textContent setter] Step 6: Completed successfully');
			},
			configurable: true,
			enumerable: true
		});
	} catch (e) {
		console.log('[createElement] Failed to define textContent property, using simple fallback:', e.message);
		// Simple fallback: just use a regular property (this shouldn't normally happen since MuJS supports Object.defineProperty)
		element.textContent = "";
	}
	
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
