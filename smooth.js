var offscreenCanvas = document.createElement("canvas");
var offscreenContext = offscreenCanvas.getContext("2d");

var optimizedCanvas = document.createElement("canvas");
var optimizedContext = optimizedCanvas.getContext("2d");

var meshReady = false;
var textureReady = false;

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

    var buf3D = mesh.get3DBuffer();
    var bufUV = mesh.getUVBuffer();

    geometry.setAttribute('position', new THREE.BufferAttribute(new Float32Array(Module.HEAPU8.buffer, buf3D, mesh.fn() * 9), 3));
    geometry.setAttribute('uv', new THREE.BufferAttribute(new Float32Array(Module.HEAPU8.buffer, bufUV, mesh.fn() * 6), 2));

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

(function() {

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
        var fileReader = new FileReader();
        fileReader.onload = function (loadEvent) {
            var buffer = new Int8Array(loadEvent.target.result);

            FS.writeFile(file.name, buffer);
            mesh.loadMesh(file.name);
            meshReady = true;
            FS.unlink(file.name);
            initThreeJS();
            animate();
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
    var onscreenCanvas = document.createElement("canvas");
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

                if (meshReady)
                    loadTextureFromCanvas(offscreenCanvas);
            };
        };
        fileReader.readAsDataURL(file);
    } else {
        console.log(file + ' is not an image');
    }
}

function optimize(ev) {
    if (!textureReady) {
        alert("Texture file missing!");
        return;
    }

    if (!meshReady) {
        alert("Mesh file missing!");
        return;
    }

    var link = document.createElement('a');
    link.innerHTML = 'Work in progress...';
    document.body.appendChild(link);

    // wait for dom update before diving into c++
    setTimeout(function() {
        var imageData = offscreenContext.getImageData(0, 0, offscreenCanvas.width, offscreenCanvas.height);
        var imgbuf = Module._malloc(imageData.data.length);
        var imgview = new Uint8Array(Module.HEAPU8.buffer, imgbuf, imageData.data.length);
        imgview.set(imageData.data);
        mesh.smoothSeams(imgview.byteOffset, offscreenCanvas.width, offscreenCanvas.height);

        optimizedCanvas.width = offscreenCanvas.width;
        optimizedCanvas.height = offscreenCanvas.height;

        var optimizedImageData = new ImageData(new Uint8ClampedArray(imgview.buffer, imgview.byteOffset, imgview.length), optimizedCanvas.width, optimizedCanvas.height);
        optimizedContext.putImageData(optimizedImageData, 0, 0);
        
        Module._free(imgbuf);

        loadTextureFromCanvas(optimizedCanvas);

        link.innerHTML = 'Save optimized texture';
        link.href = optimizedCanvas.toDataURL();
        link.addEventListener('click', function (ev) {
            link.download = "optimized.png";
        });
    }, 50);
}

document.addEventListener("drop", preventDefaultHandler, false);
document.addEventListener("dragover", preventDefaultHandler, false);
document.getElementById("mesh_drop").addEventListener("drop", meshDropHandler, false);
document.getElementById("mesh_drop").addEventListener("dragover", preventDefaultHandler, false);
document.getElementById("texture_drop").addEventListener("drop", textureDropHandler, false);
document.getElementById("texture_drop").addEventListener("dragover", preventDefaultHandler, false);
document.getElementById("opt").addEventListener("click", optimize, false);

})();
