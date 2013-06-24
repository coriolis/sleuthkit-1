
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


var is_last_message = false;
var stdincache = Array(), stdoutcache = '', stderrcache = '';
function generic_output(type, buffer, x) {

    //convert from 2's compliment to decimal
    if(x < 0)
    {
        x = (-x ^ 0xFF) ;
        x++;
    }

    //do not add \n put by output end funciton
    if(!is_last_message)
        buffer += String.fromCharCode(x);
    if(x == 10) {
        self.postMessage(
                {'type': type, 
                 'text': buffer.toString(),
                 'is_last_message': is_last_message
                }
                );
        buffer = '';
    }
    return buffer;
}

function mystdout(x) {
    stdoutcache = generic_output(1, stdoutcache, x);
}
function mystderr(x) {
    //stderrcache = generic_output(2, stderrcache, x);
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

/* a function gets removed by emcc in O1, which is used for qcow2 files
 * so manually putting in pre
 */
function _llvm_bswap_i64($x$0, $x$1) {
  var __stackBase__  = STACKTOP; STACKTOP += 16; assert(STACKTOP % 4 == 0, "Stack is unaligned"); assert(STACKTOP < STACK_MAX, "Ran out of stack");
  var label;

  var $1=__stackBase__;
  var $__x=(__stackBase__)+(8);
  var $st$2$0=(($1)|0);
  HEAP32[(($st$2$0)>>2)]=$x$0;
  var $st$2$1=(($1+4)|0);
  HEAP32[(($st$2$1)>>2)]=$x$1;
  var $st$6$0=(($1)|0);
  var $2$0=HEAP32[(($st$6$0)>>2)];
  var $st$6$1=(($1+4)|0);
  var $2$1=HEAP32[(($st$6$1)>>2)];
  var $st$10$0=(($__x)|0);
  HEAP32[(($st$10$0)>>2)]=$2$0;
  var $st$10$1=(($__x+4)|0);
  HEAP32[(($st$10$1)>>2)]=$2$1;
  var $st$14$0=(($__x)|0);
  var $3$0=HEAP32[(($st$14$0)>>2)];
  var $st$14$1=(($__x+4)|0);
  var $3$1=HEAP32[(($st$14$1)>>2)];
  var $$emscripten$temp$0$0=255;
  var $$emscripten$temp$0$1=0;
  var $4$0=$3$0 & $$emscripten$temp$0$0;
  var $4$1=$3$1 & $$emscripten$temp$0$1;
  var $5$0=(0 << 24) | (0 >>> 8);
  var $5$1=($4$0 << 24) | (0 >>> 8);
  var $st$24$0=(($__x)|0);
  var $6$0=HEAP32[(($st$24$0)>>2)];
  var $st$24$1=(($__x+4)|0);
  var $6$1=HEAP32[(($st$24$1)>>2)];
  var $$emscripten$temp$1$0=65280;
  var $$emscripten$temp$1$1=0;
  var $7$0=$6$0 & $$emscripten$temp$1$0;
  var $7$1=$6$1 & $$emscripten$temp$1$1;
  var $8$0=(0 << 8) | (0 >>> 24);
  var $8$1=($7$0 << 8) | (0 >>> 24);
  var $9$0=$5$0 | $8$0;
  var $9$1=$5$1 | $8$1;
  var $st$36$0=(($__x)|0);
  var $10$0=HEAP32[(($st$36$0)>>2)];
  var $st$36$1=(($__x+4)|0);
  var $10$1=HEAP32[(($st$36$1)>>2)];
  var $$emscripten$temp$2$0=16711680;
  var $$emscripten$temp$2$1=0;
  var $11$0=$10$0 & $$emscripten$temp$2$0;
  var $11$1=$10$1 & $$emscripten$temp$2$1;
  var $12$0=($11$0 << 24) | (0 >>> 8);
  var $12$1=($11$1 << 24) | ($11$0 >>> 8);
  var $13$0=$9$0 | $12$0;
  var $13$1=$9$1 | $12$1;
  var $st$48$0=(($__x)|0);
  var $14$0=HEAP32[(($st$48$0)>>2)];
  var $st$48$1=(($__x+4)|0);
  var $14$1=HEAP32[(($st$48$1)>>2)];
  var $$emscripten$temp$3$0=-16777216;
  var $$emscripten$temp$3$1=0;
  var $15$0=$14$0 & $$emscripten$temp$3$0;
  var $15$1=$14$1 & $$emscripten$temp$3$1;
  var $16$0=($15$0 << 8) | (0 >>> 24);
  var $16$1=($15$1 << 8) | ($15$0 >>> 24);
  var $17$0=$13$0 | $16$0;
  var $17$1=$13$1 | $16$1;
  var $st$60$0=(($__x)|0);
  var $18$0=HEAP32[(($st$60$0)>>2)];
  var $st$60$1=(($__x+4)|0);
  var $18$1=HEAP32[(($st$60$1)>>2)];
  var $$emscripten$temp$4$0=0;
  var $$emscripten$temp$4$1=255;
  var $19$0=$18$0 & $$emscripten$temp$4$0;
  var $19$1=$18$1 & $$emscripten$temp$4$1;
  var $20$0=($19$0 >>> 8) | ($19$1 << 24);
  var $20$1=($19$1 >>> 8) | (0 << 24);
  var $21$0=$17$0 | $20$0;
  var $21$1=$17$1 | $20$1;
  var $st$72$0=(($__x)|0);
  var $22$0=HEAP32[(($st$72$0)>>2)];
  var $st$72$1=(($__x+4)|0);
  var $22$1=HEAP32[(($st$72$1)>>2)];
  var $$emscripten$temp$5$0=0;
  var $$emscripten$temp$5$1=65280;
  var $23$0=$22$0 & $$emscripten$temp$5$0;
  var $23$1=$22$1 & $$emscripten$temp$5$1;
  var $24$0=($23$0 >>> 24) | ($23$1 << 8);
  var $24$1=($23$1 >>> 24) | (0 << 8);
  var $25$0=$21$0 | $24$0;
  var $25$1=$21$1 | $24$1;
  var $st$84$0=(($__x)|0);
  var $26$0=HEAP32[(($st$84$0)>>2)];
  var $st$84$1=(($__x+4)|0);
  var $26$1=HEAP32[(($st$84$1)>>2)];
  var $$emscripten$temp$6$0=0;
  var $$emscripten$temp$6$1=16711680;
  var $27$0=$26$0 & $$emscripten$temp$6$0;
  var $27$1=$26$1 & $$emscripten$temp$6$1;
  var $28$0=($27$1 >>> 8) | (0 << 24);
  var $28$1=(0 >>> 8) | (0 << 24);
  var $29$0=$25$0 | $28$0;
  var $29$1=$25$1 | $28$1;
  var $st$96$0=(($__x)|0);
  var $30$0=HEAP32[(($st$96$0)>>2)];
  var $st$96$1=(($__x+4)|0);
  var $30$1=HEAP32[(($st$96$1)>>2)];
  var $$emscripten$temp$7$0=0;
  var $$emscripten$temp$7$1=-16777216;
  var $31$0=$30$0 & $$emscripten$temp$7$0;
  var $31$1=$30$1 & $$emscripten$temp$7$1;
  var $32$0=($31$1 >>> 24) | (0 << 8);
  var $32$1=(0 >>> 24) | (0 << 8);
  var $33$0=$29$0 | $32$0;
  var $33$1=$29$1 | $32$1;
  STACKTOP = __stackBase__;
  return [$33$0,$33$1]; 
}
 

// our code end 
