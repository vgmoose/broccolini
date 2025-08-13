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
