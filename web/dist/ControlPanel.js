const gW = window.innerWidth;
const gH = window.innerHeight;
const canvasW = gW*2/3;
const canvasH = gH/2;
const gRadius = canvasW/2;
const gCanvas = document.getElementById("canvas");
const gCtx = gCanvas.getContext("2d");
const firstTouch = {}

const body = document.getElementsByTagName("body")[0];
const gMarginTop = parseInt(getComputedStyle(body, null).marginTop);
log(`marginTop: ${gMarginTop}`);

console.log("width:" + gW + " height:" + gH)

document.addEventListener("DOMContentLoaded", startup);

function clearCanvas() {
    gCtx.clearRect(0, 0, canvasW, canvasW);

    gCtx.beginPath();
    gCtx.arc(canvasW/2, canvasW/2, gRadius, 0, 2 * Math.PI, false); // a circle
    gCtx.strokeStyle = "#FF0000"
    gCtx.lineWidth = 1;
    gCtx.stroke();

    gCtx.beginPath();
    gCtx.moveTo(canvasW/2, 0);
    gCtx.lineTo(canvasW/2, canvasW);
    gCtx.strokeStyle = "#888888";
    gCtx.stroke();

    gCtx.beginPath();
    gCtx.moveTo(gRadius + gRadius*Math.cos(Math.PI/4), gRadius - gRadius*Math.sin(Math.PI/4));
    gCtx.lineTo(gRadius - gRadius*Math.cos(Math.PI/4), gRadius + gRadius*Math.sin(Math.PI/4));
    gCtx.stroke();

    gCtx.beginPath();
    gCtx.moveTo(gRadius - gRadius*Math.cos(Math.PI/4), gRadius - gRadius*Math.sin(Math.PI/4));
    gCtx.lineTo(gRadius + gRadius*Math.cos(Math.PI/4), gRadius + gRadius*Math.sin(Math.PI/4));
    gCtx.stroke();
}

function startup() {
	gCanvas.width = canvasW;
	gCanvas.height = canvasW;

	gCanvas.addEventListener("touchstart", handleStart);
    gCanvas.addEventListener("touchend", handleEnd);
    gCanvas.addEventListener("touchcancel", handleCancel);
    gCanvas.addEventListener("touchmove", handleMove);
    log("Initialized. convas width:" + gCanvas.width);

	const btn1 = document.getElementById("btn1")
	const btn2 = document.getElementById("btn2")
	btn1.addEventListener("click", handleBtn1Click);
	btn2.addEventListener("click", handleBtn2Click);

    clearCanvas();
}

function handleBtn1Click(evt) {
	log("btn1 clicked")	
    const msg = {};
    msg.type= "btn";
    msg.value = 1;
    postData(msg);
}

function handleBtn2Click(evt) {
	log("btn2 clicked")	
    const msg = {};
    msg.type= "btn";
    msg.value = 2;
    postData(msg);
}

function handleStart(evt) {
    evt.preventDefault();
    const ctx = gCanvas.getContext("2d");
    const touches = evt.changedTouches;
    //log(`touch start, touch num:${touches.length}, first touch's id is:${touches[0].identifier}`);
    log(`touch start`);
    firstTouch.identifier = touches[0].identifier;
    firstTouch.pageX = touches[0].pageX;
    firstTouch.pageY = touches[0].pageY;
}

var _gLastEventTs = Date.now();
function handleMove(evt) {
    evt.preventDefault();
    const touches = evt.changedTouches;
    let i;
    for (i = 0; i < touches.length; i++) {
        if (touches[i].identifier === firstTouch.identifier) {
            break;
        }
    }
    if (i === touches.length) {
        log("handleMove: another touch move, ignore it")
        return;
    }

    let now = Date.now();
    if (now - _gLastEventTs < 100) {
        return;
    }
    _gLastEventTs = now;

    const pageX = touches[i].pageX;
    const pageY = touches[i].pageY;

    let dx = Math.round((pageX - gRadius - gMarginTop)/gRadius * 100);
    let dy = Math.round((pageY - gRadius - gMarginTop)/gRadius * 100);


    let s, s2, d, m;

    let r = Math.sqrt(dx*dx + dy*dy);

    let rad = Math.acos(Math.abs(dx)/r)
   
    s = 100;
    if (rad < Math.PI/4) {
        s2 = 100;
    } else {
        s2 = 50;
    }


    if (dx >= 0 && dy >= 0) {
        d = 1; //"right"
        m = 1; //"foward"
    } else if (dx >= 0 && dy < 0) {
        d = 2; //"left";
        m = 1; //"foward";
    } else if (dx < 0 && dy >= 0) {
        d = 1; //"right";
        m = 2; //"back";
    } else if (dx < 0 && dy < 0) {
        d = 2; //"left";
        m = 2; //"back";
    }

    //log(`joystick: s:${s} s2:${s2} d:${d} m:${m}`); 
    clearCanvas();
    gCtx.beginPath();
    gCtx.moveTo(canvasW/2, canvasW/2);
    gCtx.lineTo(pageX - gMarginTop, pageY - gMarginTop);
    gCtx.strokeStyle = "red";
    gCtx.stroke();
    firstTouch.pageX = touches[i].pageX;
    firstTouch.pageY = touches[i].pageY;

    const msg = {};
    msg.type = "joystick";
    msg.s = s;
    msg.s2 = s2;
    msg.d = d;
    msg.m = m;
    postData(msg);

}

function handleEnd(evt) {
    evt.preventDefault();
    const touches = evt.changedTouches;

    for (let i = 0; i < touches.length; i++) {
        if (touches[i].identifier !== firstTouch.identifier) {
            continue;
        }

        log("touch end"); 
        clearCanvas();
        const msg = {};
        msg.type = "joystick";
        msg.s = 0;
        msg.s2 = 0;       
        msg.m = 0;
        msg.d = 0;
        postData(msg);
        break;
    }
}

function handleCancel(evt) {
    evt.preventDefault();
    const touches = evt.changedTouches;
    for (let i = 0; i < touches.length; i++) {
        if (touches[i].identifier !== firstTouch.identifier) {
            continue;
        }

        log("touch cancel"); 
        //clearCanvas();
    }
}

function log(msg) {
    const container = document.getElementById("log");
    container.textContent = `${msg} \n${container.textContent}`;
}



