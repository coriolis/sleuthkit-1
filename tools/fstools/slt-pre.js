
// Our generated code 
importScripts('myrt.js');
self.onmessage = function(ev) {
    setup_file(ev.data['fs']);
    Module['run']([ev.data['fn']]);

};


var Module= { 'noInitialRun': true} ;
//set print to be postMessage
Module['print'] = function (x) {
   self.postMessage(x);
};



// our code end 
