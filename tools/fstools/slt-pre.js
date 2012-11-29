
// Our generated code 
importScripts('myrt.js');
self.onmessage = function(ev) {
    setup_file(ev.data['fs']);
    Module['run']([ev.data['fn']]);

};


var stdoutcache = '', stderrcache = '';
function generic_output(type, buffer, x) {
    if(x == 10) {
        self.postMessage({'type': type, 'text': buffer});
        buffer = '';
    }
    else
        buffer += String.fromCharCode(x);
    return buffer;
}

function mystdout(x) {
    stdoutcache = generic_output(1, stdoutcache, x);
}
function mystderr(x) {
    stderrcache = generic_output(2, stderrcache, x);
}

function mystdin() {
    return null;
}

function vmx_do_init() {
    FS.init(mystdin, mystdout, mystderr);
}

var Module= { 'noInitialRun': true} ;
//set print to be postMessage
Module['print'] = mystdout;

Module['preInit'] = vmx_do_init;



// our code end 
