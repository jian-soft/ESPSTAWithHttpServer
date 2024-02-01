const gW = window.innerWidth;
const gH = window.innerHeight;

const body = document.getElementsByTagName("body")[0];
const gMarginTop = parseInt(getComputedStyle(body, null).marginTop);


console.log("width:" + gW + " height:" + gH)
document.addEventListener("DOMContentLoaded", startup);

function startup() {
    const btnIds = ["btn1","btn2","btn3","btn4","btn5","btn6","btn7","btn8"]
    let left = 5;
    for (const [index, bId] of btnIds.entries()) {
        const btn = document.getElementById(bId);
        btn.style.position = "absolute";
        btn.style.width = "30px";
        btn.style.height = "150px";
        btn.style.top = "10px";        
        btn.style.left = left.toString() + "px";
        btn.addEventListener("click", function(){handleBtnClick(index)});
        left += 35;
    }



}

function handleBtnClick(id) {
	console.log("btn1 clicked: " + id)	
    const msg = {};
    msg.type= "pitch";
    msg.value = id;
    postData(msg);
}


