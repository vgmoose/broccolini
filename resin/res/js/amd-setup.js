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
