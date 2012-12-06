
// Our generated code 
importScripts('myrt.js');
self.onmessage = function(ev) {

    switch(ev.data['type']) {
        case 'init': 
            setup_file(ev.data.data);
            break
        case 'send':
            var s = ev.data.data; 
            s[s.length-1] = '/' + fileInstance.name;
            try {
                Module['run'](s);
            } 
            catch(e) {
                //pass
            }

            break;
    }
    //Module['run']([ev.data.data]);

};


var stdincache = Array(), stdoutcache = '', stderrcache = '';
function generic_output(type, buffer, x) {
    buffer += String.fromCharCode(x);
    if(x == 10) {
        self.postMessage({'type': type, 'text': buffer});
        buffer = '';
    }
    return buffer;
}

function mystdout(x) {
    stdoutcache = generic_output(1, stdoutcache, x);
}
function mystderr(x) {
    stderrcache = generic_output(2, stderrcache, x);
}

function send_log_message(x) {
        self.postMessage({'type': 2, 'text': x});
}

function mystdin() {
    line = stdincache.shift();
    return line;
}

function vmx_do_init() {
    FS.init(mystdin, mystdout, mystderr);
}

var Module= { 'noInitialRun': true, 'noExitRuntime': true} ;
//set print to be postMessage
Module['print'] = send_log_message;

Module['preInit'] = vmx_do_init;



// our code end 
