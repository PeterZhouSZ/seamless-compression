var offscreenCanvas = document.createElement("canvas");
var offscreenContext = offscreenCanvas.getContext("2d");

var optimizedCanvas = document.createElement("canvas");
var optimizedContext = optimizedCanvas.getContext("2d");

var onscreenCanvas = null;

var meshReady = false;
var textureReady = false;

var downloadTarget = null;

var textureUi = {
    input: null,
    inputName: null,
    outputName: null,
    output: null,
    current: null,
    zoomSrc: null,
};

(function() {

function render()
{
    renderer.render(scene, camera);
}

function animate()
{
    requestAnimationFrame(animate);
    controls.update();
    render();
}

function loadTextureFromCanvas(canvas)
{
    renderMesh.material.map = new THREE.CanvasTexture(canvas);
    renderMesh.material.needsUpdate = true;
    render();
}

function initThreeJS() {
    var div = document.getElementById("mesh_drop");
    while (div.hasChildNodes())
        div.removeChild(div.firstChild);
    var renderCanvas = document.createElement("canvas");
    div.appendChild(renderCanvas);
    renderCanvas.style.width = "100%";
    renderCanvas.style.height = "100%";
    renderCanvas.width = renderCanvas.offsetWidth;
    renderCanvas.height = renderCanvas.offsetHeight;

    scene = new THREE.Scene();
    scene.background = new THREE.Color(0xcccccccc);
    renderer = new THREE.WebGLRenderer({canvas: renderCanvas});
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.setSize(renderCanvas.width, renderCanvas.height);

    camera = new THREE.PerspectiveCamera(60, renderCanvas.width / renderCanvas.height, 0.01, 1000);

    controls = new THREE.TrackballControls(camera, renderer.domElement);
    controls.rotateSpeed = 1.0;
    controls.zoomSpeed = 1.2;
    controls.panSpeed = 0.8;
    controls.staticMoving = true;
    controls.addEventListener('change', render);

    var geometry = new THREE.BufferGeometry();
    var material = new THREE.MeshBasicMaterial();

    var buf3D = proc.get3DBuffer();
    var bufUV = proc.getUVBufferMirrorV();

    geometry.setAttribute('position', new THREE.BufferAttribute(new Float32Array(Module.HEAPU8.buffer, buf3D, proc.fn() * 9), 3));
    geometry.setAttribute('uv', new THREE.BufferAttribute(new Float32Array(Module.HEAPU8.buffer, bufUV, proc.fn() * 6), 2));

    geometry.computeBoundingSphere();

    controls.target = geometry.boundingSphere.center;
    camera.position.set(geometry.boundingSphere.center.x, geometry.boundingSphere.center.y, geometry.boundingSphere.center.z - 1.5 * geometry.boundingSphere.radius);
    camera.lookAt(controls.target.x, controls.target.y, controls.target.z);

    renderMesh = new THREE.Mesh(geometry, material);
    scene.add(renderMesh);

    if (textureReady)
        loadTextureFromCanvas(offscreenCanvas);

    render();
}

function createInfoDiv(name, w, h, encoding) {
    var div = document.createElement("div");
    div.setAttribute("class", "fr");
    div.setAttribute("id", "texture-info");
    var dname = document.createElement("div");
    dname.innerHTML = name;
    var dres = document.createElement("div");
    dres.innerHTML = w + "x" + h;
    var denc = document.createElement("div");
    denc.innerHTML = encoding;
    div.appendChild(dname);
    div.appendChild(dres);
    div.appendChild(denc);

    return div;
}

function setTexture(tex) {
    var textureInfo = document.getElementById("texture-info");
    if (textureInfo)
        textureInfo.remove();
    if (tex == "output") {
        document.getElementById("texture-ui").appendChild(textureUi.output);
        textureUi.current = "output";
        textureUi.zoomSrc = optimizedCanvas;
        onscreenCanvas.getContext("2d").drawImage(optimizedCanvas, 0, 0, onscreenCanvas.width, onscreenCanvas.height);
        if (meshReady)
            loadTextureFromCanvas(optimizedCanvas);
    } else if (tex == "input") {
        document.getElementById("texture-ui").appendChild(textureUi.input);
        textureUi.current = "input";
        textureUi.zoomSrc = offscreenCanvas;
        onscreenCanvas.getContext("2d").drawImage(offscreenCanvas, 0, 0, onscreenCanvas.width, onscreenCanvas.height);
        if (meshReady)
            loadTextureFromCanvas(offscreenCanvas);
    }
    if (zoomCanvas)
        updateZoomCanvas();
}

function switchTexture() {
    if (textureUi.current == "input")
        setTexture("output");
    else if (textureUi.current == "output")
        setTexture("input");
}

function enableTextureSwitch() {
    var btn = document.getElementById("switch-texture");
    btn.style.display = "block";
}

function preventDefaultHandler(ev) {
    ev.preventDefault()
}

function meshDropHandler(ev) {
    console.log('Mesh dropped');
    
    ev.preventDefault();

    if (ev.dataTransfer.files.length > 0) {
        loadOBJ(ev.dataTransfer.files[0]);
    }
}

function loadOBJ(file) {
    if (file instanceof File) {
        console.log(file.name);
        var fileReader = new FileReader();
        fileReader.onload = function (loadEvent) {
            var buffer = new Int8Array(loadEvent.target.result);

            FS.writeFile(file.name, buffer);
            proc.loadMesh(file.name);
            meshReady = true;
            FS.unlink(file.name);
            initThreeJS();
            animate();

            if (textureReady) {
                var cbseamless = document.getElementById("cbseamless");
                cbseamless.disabled = false;
            }
        };
        fileReader.readAsArrayBuffer(file);
    }
}

function textureDropHandler(ev) {
    console.log('Texture dropped');

    ev.preventDefault();
    if (ev.dataTransfer.files.length > 0) {
        loadTextureImage(ev.dataTransfer.files[0]);
    }
}

function createOnscreenCanvas() {
    var div = document.getElementById("texture_drop");
    while (div.hasChildNodes())
        div.removeChild(div.firstChild);
    onscreenCanvas = document.createElement("canvas");
    div.appendChild(onscreenCanvas);
    onscreenCanvas.style.width = "100%";
    onscreenCanvas.style.height = "100%";
    onscreenCanvas.width = onscreenCanvas.offsetWidth;
    onscreenCanvas.height = onscreenCanvas.offsetHeight;
    onscreenCanvas.getContext("2d").drawImage(offscreenCanvas, 0, 0, onscreenCanvas.width, onscreenCanvas.height);
};

function loadTextureImage(file) {
    if (file instanceof File && file.type.indexOf('image') !== -1) {
        var fileReader = new FileReader();
        fileReader.onload = function (loadEvent) {
            var img = new Image();
            img.src = loadEvent.target.result;
            img.onload = function () {
                offscreenCanvas.width = img.naturalWidth;
                offscreenCanvas.height = img.naturalHeight;
                offscreenContext.drawImage(img, 0, 0);
                createOnscreenCanvas();
                textureReady = true;

                var cbcompress = document.getElementById("cbcompress");
                cbcompress.disabled = false;

                if (meshReady) {
                    loadTextureFromCanvas(offscreenCanvas);
                    var cbseamless = document.getElementById("cbseamless");
                    cbseamless.disabled = false;
                }

                textureUi.inputName = file.name;
                textureUi.input = createInfoDiv(textureUi.inputName, img.naturalWidth, img.naturalHeight, "RGB");
                setTexture("input");
                initZoom();
            };
        };
        fileReader.readAsDataURL(file);
    } else {
        console.log(file + ' is not an image');
    }
}

function optimize(ev) {
    var compress = document.getElementById("cbcompress").checked == true;
    var smooth = document.getElementById("cbseamless").checked == true;
    var alpha = parseFloat(document.getElementById("alpha").value);

    if (!compress && !smooth)
        return;

    if (!textureReady) {
        alert("Texture file missing!");
        return;
    }

    if (smooth && !meshReady) {
        alert("Mesh file missing!");
        return;
    }

    var download = document.getElementById("btn-save");
    var wip = document.getElementById("output-wip");

    download.classList.add("hidden");
    wip.classList.remove("hidden");

    setTimeout(function() {
        var imageData = offscreenContext.getImageData(0, 0, offscreenCanvas.width, offscreenCanvas.height);
        proc.allocateImageBuffers(offscreenCanvas.width, offscreenCanvas.height);
        var imgbuf = proc.getImageBuf();
        imgbuf.set(imageData.data);

        var encoding;
        var outputName;
        if (compress && smooth) {
            proc.compressAndSmooth(alpha);
            downloadTarget = "compressed-texture";
            encoding = "DXT1 Seamless";
            outputName = "compressed-seamless.DDS";
        } else if (compress) {
            proc.compress();
            downloadTarget = "compressed-texture";
            encoding = "DXT1";
            outputName = "compressed.DDS";
        } else if (smooth) {
            proc.smooth(alpha);
            downloadTarget = "output-canvas";
            encoding = "RGB Seamless";
            outputName = "seamless.png";
        } else {
            console.log("This should never happen");
            return;
        }

        optimizedCanvas.width = offscreenCanvas.width;
        optimizedCanvas.height = offscreenCanvas.height;
        var optimizedImageData = new ImageData(Uint8ClampedArray.from(proc.getOutputBuf()), optimizedCanvas.width, optimizedCanvas.height);
        optimizedContext.putImageData(optimizedImageData, 0, 0);

        textureUi.outputName = outputName;
        textureUi.output = createInfoDiv(textureUi.outputName, optimizedCanvas.width, optimizedCanvas.height, encoding);
        enableTextureSwitch();
        setTexture("output");

        wip.classList.add("hidden");
        download.classList.remove("hidden");

        if (downloadTarget == "compressed-texture") {
            download.innerHTML = "Save DXT texture";
            download.onclick = function(ev) {
                var file = new File([proc.getCompressedBuf()], textureUi.outputName, { type: "application/octet-stream" });
                saveAs(file);
            };
        } else if (downloadTarget == "output-canvas") {
            download.innerHTML = "Save PNG texture";
            download.onclick = function(ev) {
                optimizedCanvas.toBlob(function(blob) {
                    saveAs(blob, textureUi.outputName);
                });
            };
        } else {
            console.log("This should never happen");
        }
    }, 50);
}

document.addEventListener("drop", preventDefaultHandler, false);
document.addEventListener("dragover", preventDefaultHandler, false);
document.getElementById("mesh_drop").addEventListener("drop", meshDropHandler, false);
document.getElementById("mesh_drop").addEventListener("dragover", preventDefaultHandler, false);
document.getElementById("texture_drop").addEventListener("drop", textureDropHandler, false);
document.getElementById("texture_drop").addEventListener("dragover", preventDefaultHandler, false);
document.getElementById("opt").addEventListener("click", optimize, false);
document.getElementById("switch-texture").addEventListener("click", switchTexture, false);


document.getElementById("cbcompress").disabled = true;
document.getElementById("cbseamless").disabled = true;
document.getElementById("cbcompress").checked = true;
document.getElementById("cbseamless").checked = true;
})();

