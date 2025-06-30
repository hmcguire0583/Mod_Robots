/* 
 * User class
 * User is a wrapper around the three.js Camera observing the scene,
 *  containing view settings (projection type, zoom level, etc)
 *  as well as methods to switch view settings
 * This module also handles other types of user input
 *  (such as window resizing)
*/

import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { gScene, gCanvas, gMiniCanvas, gUser, gRenderer, gMiniRenderer, gLights, toggleRenderMode } from "./main.js";
import { CameraType, VisConfigData } from "./utils.js";

/* ****************************** */
/* User class */
/* ****************************** */
export class User {
    constructor() {
        this.cameraStyle = CameraType.ORTHOGRAPHIC;
        this.headlamp = new THREE.PointLight(0xFFFFFF, 50.0);
        this.headlamp.layers.set(1);
        this.headlamp.position.set(0.0, 0.0, 0.0);
        gLights.headlamp = this.headlamp;
        gLights._defaultHeadlampIntensity = this.headlamp.intensity;
        this.resetCamera();
        this.miniHeadlamp = new THREE.PointLight(0xFFFFFF, 50.0);
        this.miniHeadlamp.layers.set(2);
        this.miniHeadlamp.position.set(0.0, 0.0, 0.0);
        gLights.miniHeadlamp = this.miniHeadlamp;
        gLights._defaultMiniHeadlampIntensity = this.miniHeadlamp.intensity;
        this.resetMiniCamera();
        this.initReferenceCamera();
        window.gwUser = this;
    }

    resetCamera() {
        let newCamera;
        switch (this.cameraStyle) {
            case CameraType.PERSPECTIVE: newCamera = new THREE.PerspectiveCamera( 75, gCanvas.clientWidth / gCanvas.clientHeight, 0.1, 250.0 ); break;
            case CameraType.ORTHOGRAPHIC: {
                let width = gCanvas.clientWidth / 100;
                let height = gCanvas.clientHeight / 100;
                newCamera = new THREE.OrthographicCamera( -width, width, height, -height, 0.1, 250.0 ); break;
            }
        }
        newCamera.layers.enable(1); // Layer 1 is for main view exclusive content
        newCamera.position.x = window.gwScenarioCentroid.x;
        newCamera.position.y = window.gwScenarioCentroid.y;
        newCamera.position.z = this.cameraStyle === CameraType.PERSPECTIVE
            ? window.gwScenarioCentroid.z + window.gwScenarioRadius + 3.0
            // Numbers selected through trial and error, but it seems to work
            : 5.0 + Math.max(VisConfigData.bounds.z.max, VisConfigData.bounds.x.max);

        this.controls = new OrbitControls(newCamera, gCanvas);
        this.controls.target.set(...window.gwScenarioCentroid);
        this.camera = newCamera;
        this.camera.add(this.headlamp);
        gScene.add(this.camera);
    }

    resetMiniCamera() {
        let initial_position = new THREE.Vector3(...window.gwScenarioCentroid);
        if (this.miniCamera) {
            initial_position.add(this.miniCamera.position).sub(this.miniControls.target);
        } else {
            initial_position.add(0.0, 0.0, 3.0);
        }
        this.miniCamera = new THREE.PerspectiveCamera( 75, gMiniCanvas.clientWidth / gMiniCanvas.clientHeight, 0.1, 250.0 );
        this.miniCamera.layers.enable(2); // Layer 2 is for mini view exclusive content
        this.miniCamera.position.set(...initial_position);

        this.miniControls = new OrbitControls(this.miniCamera, gMiniCanvas);
        this.miniControls.target.set(...window.gwScenarioCentroid);
        this.miniControls.enablePan = false;
        this.miniCamera.add(this.miniHeadlamp);
        gScene.add(this.miniCamera);
    }

    initReferenceCamera() {
        this.referenceCamera = new THREE.PerspectiveCamera( 75, 1.0, 0.1, 250.0 );
        this.referenceCamera.layers.set(3); // Reference camera can see reference module, nothing else
        this.referenceCamera.position.z = 1.5;
        gScene.add(this.referenceCamera);
    }

    toggleCameraStyle() {
        if (window._isPainterModeActive) return;
        let newCameraStyle;
        switch(this.cameraStyle) {
            case CameraType.PERSPECTIVE: newCameraStyle = CameraType.ORTHOGRAPHIC; break;
            case CameraType.ORTHOGRAPHIC: newCameraStyle = CameraType.PERSPECTIVE; break;
        }
        this.cameraStyle = newCameraStyle;
        this.resetCamera();
    }
}

/* ****************************** */
/* User interaction callbacks */
/* ****************************** */
function window_resize_callback() {
    const width = window.innerWidth * gCanvas._xscale;
    const height = window.innerHeight * gCanvas._yscale;

    let newAspect = width/height;
    switch (gUser.cameraStyle) {
        case CameraType.PERSPECTIVE: gUser.camera.aspect = newAspect; break;
        case CameraType.ORTHOGRAPHIC: {
            let oldAspect = gUser.camera.right / gUser.camera.top;
            gUser.camera.left = gUser.camera.left * (newAspect / oldAspect);
            gUser.camera.right = gUser.camera.right * (newAspect / oldAspect);
            break;
        }
    }

    gUser.camera.updateProjectionMatrix();
    gRenderer.setSize(width, height);
    
    // Mini View
    gUser.miniCamera.aspect = newAspect;
    gUser.miniCamera.updateProjectionMatrix();
    gMiniRenderer.setSize(0.25 * width, 0.25 * height);
}

let mx = 0, my = 0;
function mousemove_callback(event) {
    mx = event.clientX;
    my = event.clientY;
}

function keydown_input_callback(event) {
    let key = event.key;
    switch (key) {
        case 'p': gUser.toggleCameraStyle(); break;
        case 'r': gUser.resetCamera(); break;
        case 'ArrowRight': _requestForwardAnim(); break;
        case 'ArrowLeft': _requestBackwardAnim(); break;
        case 'M': toggleRenderMode(); break;
        case 'P': console.log(gRenderer.domElement); break;
        case `c`: selectModule(mx, my); break;
        default: break;
    }
}

// Register callbacks only after page loads
document.addEventListener("DOMContentLoaded", async function () {
    window.addEventListener('resize', window_resize_callback);
    window.addEventListener('keydown', keydown_input_callback);
    gCanvas.addEventListener('mousemove', mousemove_callback);
});

/* ****************************** */
/* Global function definitions */
/* ****************************** */
// Unused for now
function selectModule(viewportX, viewportY) {
    let clipPoint = new THREE.Vector4(viewportX / gCanvas.clientWidth * 2.0 - 1.0, viewportY / gCanvas.clientHeight * 2.0 - 1.0, 0.0, 1.0);
    let viewPoint = clipPoint.clone().applyMatrix4(gUser.camera.projectionMatrixInverse);
    let worldPoint = viewPoint.clone().applyMatrix4(gUser.camera.matrixWorldInverse);
    worldPoint.multiplyScalar(1.0 / worldPoint.w);

    console.log(viewportX, viewportY);
    console.log(clipPoint);
    console.log(viewPoint);
    console.log(worldPoint);
    console.log(gUser.camera.position);
}

window._requestForwardAnim = function () {
    if (window._isPainterModeActive) return;
    window.gwNextAnimationRequested = true; 
    window.gwForward = true;
}
window._requestBackwardAnim = function () {
    if (window._isPainterModeActive) return;
    window.gwNextAnimationRequested = true;
    window.gwForward = false;
}

