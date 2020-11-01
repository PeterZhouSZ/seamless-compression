var zoomCanvas;
var lens = {
    x: 0,
    y: 0,
    width: 40,
    height: 40,
    element: null
};

function initZoom() {
    document.getElementById("zoom-container").classList.remove("hidden");
    var div = document.getElementById("zoom-window");
    zoomCanvas = document.createElement("canvas");
    div.appendChild(zoomCanvas);
    zoomCanvas.style.width = "100%";
    zoomCanvas.style.height = "100%";
    zoomCanvas.width = onscreenCanvas.offsetWidth;
    zoomCanvas.height = onscreenCanvas.offsetHeight;

    lens.element = document.createElement("div");
    lens.element.setAttribute("id", "img-zoom-lens");

    onscreenCanvas.parentElement.insertBefore(lens.element, onscreenCanvas);

    lens.element.addEventListener("mousemove", moveLens);
    onscreenCanvas.addEventListener("mousemove", moveLens);
    lens.element.addEventListener("click", clickLens);
    onscreenCanvas.addEventListener("click", clickLens);

    var tracking = true;

    var slider = document.getElementById("zoom-slider");
    slider.min = 0;
    slider.max = 90;
    slider.value = 80;

    slider.addEventListener("input", resizeLens);

    resizeLens();

    function placeLens(x, y) {
        if (x > onscreenCanvas.width - lens.width) { x = onscreenCanvas.width - lens.width; }
        if (x < 0) { x = 0; }
        if (y > onscreenCanvas.height - lens.height) { y = onscreenCanvas.height - lens.height; }
        if (y < 0) { y = 0; }

        lens.x = x;
        lens.y = y;

        lens.element.style.left = lens.x + "px";
        lens.element.style.top = lens.y + "px";
    }

    function resizeLens() {
        var pct = (100 - slider.value) / 100;
        lens.width = pct * onscreenCanvas.offsetWidth;
        lens.height = pct * onscreenCanvas.offsetHeight;
        lens.element.style.width = lens.width + "px";
        lens.element.style.height = lens.height + "px";
        placeLens(lens.x, lens.y);
        updateZoomCanvas();
    }

    function clickLens(e) {
        e.preventDefault();
        tracking = !tracking;
        moveLens(e);
    }

    function moveLens(e) {
        e.preventDefault();
        if (tracking) {
            var pos = getCursorPos(e);
            var x = pos.x - (lens.width / 2);
            var y = pos.y - (lens.height / 2);

            placeLens(x, y);
            updateZoomCanvas();
        }
    }

    function getCursorPos(e) {
        e = e || window.event;
        var a = onscreenCanvas.getBoundingClientRect();
        var x = e.pageX - a.left;
        var y = e.pageY - a.top;
        x = x - window.pageXOffset;
        y = y - window.pageYOffset;
        return { x : x, y : y };
    }
}

function updateZoomCanvas() {
    var sx = (lens.x / onscreenCanvas.offsetWidth) * offscreenCanvas.width;
    var sy = (lens.y / onscreenCanvas.offsetHeight) * offscreenCanvas.height;
    var sw = (lens.width / onscreenCanvas.offsetWidth) * offscreenCanvas.width;
    var sh = (lens.height / onscreenCanvas.offsetHeight) * offscreenCanvas.height;

    zoomCanvas.getContext("2d").drawImage(textureUi.zoomSrc, sx, sy, sw, sh,
        0, 0, zoomCanvas.height, zoomCanvas.width);
}

