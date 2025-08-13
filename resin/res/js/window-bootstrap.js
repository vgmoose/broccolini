(function(){
  if(typeof window==='undefined'){window={};}
  if(typeof document!=='undefined'){ if(typeof window.document==='undefined') window.document=document; }
  if(!window.parent) window.parent=window;
  if(!window.top) window.top=window;
  if(!window.self) window.self=window;
  if(typeof window.alert!=='function'){ window.alert=function(msg){ console.log('[ALERT]', msg); }; if(typeof alert==='undefined') alert=window.alert; }
  if(typeof window.setTimeout!=='function'){ window.setTimeout=function(cb,ms){ if(typeof cb==='function'){ cb(); } }; if(typeof setTimeout==='undefined') setTimeout=window.setTimeout; }
  if(!window.parent) window.parent = window;
  if(!window.parent.location) window.parent.location = window.location;
})();
