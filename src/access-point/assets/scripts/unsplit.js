/**
 * unsplit.js — Shamir's Secret Sharing reconstruct flow.
 *
 * Dual-mode: uses WASM (client-side) when sss.js is available (GitHub Pages
 * demo), falls back to fetch(/reconstruct) on the embedded device.
 */

(function () {
    'use strict';

    var Module = null;
    var useWasm = false;

    /* ── WASM bootstrap ── */
    (function () {
        var script = document.createElement('script');
        script.src = 'scripts/sss.js';               // relative to HTML page
        script.onload = function () {
            SSS().then(function (m) { Module = m; useWasm = true; });
        };
        script.onerror = function () { /* fetch fallback */ };
        document.head.appendChild(script);
    })();

    /* ── Shared helpers ── */
    var shareRows = document.querySelectorAll('.share-row');
    var unsplitBtn = document.getElementById('unsplit-btn');
    var resultBox = document.getElementById('result-box');
    var resultText = document.getElementById('result-text');
    var copyResultBtn = document.getElementById('copy-result');
    var toast = document.getElementById('toast');

    function showToast(text) {
        toast.textContent = text;
        toast.classList.add('visible');
        setTimeout(function () { toast.classList.remove('visible'); }, 1500);
    }

    function copyText(text, btn) {
        if (navigator.clipboard && navigator.clipboard.writeText) {
            navigator.clipboard.writeText(text).then(function () {
                btn.classList.add('copied');
                setTimeout(function () { btn.classList.remove('copied'); }, 1000);
                showToast('Copied!');
            });
        } else {
            var ta = document.createElement('textarea');
            ta.value = text;
            ta.style.position = 'fixed';
            ta.style.opacity = '0';
            document.body.appendChild(ta);
            ta.select();
            document.execCommand('copy');
            document.body.removeChild(ta);
            btn.classList.add('copied');
            setTimeout(function () { btn.classList.remove('copied'); }, 1000);
            showToast('Copied!');
        }
    }

    /* ── Parse share rows ── */
    function parseShares() {
        var d = [], x = [];
        shareRows.forEach(function (row) {
            var xInput = row.querySelector('.x-input');
            var dInput = row.querySelector('.d-input');
            var val = dInput.value.trim();
            if (val === '') return;

            var colon = val.indexOf(':');
            if (colon !== -1) {
                d.push(val.substring(colon + 1));
                x.push(parseInt(val.substring(0, colon), 10));
            } else {
                d.push(val);
                x.push(parseInt(xInput.value.trim(), 10) || 0);
            }
        });
        return { d: d, x: x };
    }

    /* ── WASM implementation ── */
    function sizeofShare() { return 264; }

    function reconstructWasm(d, x) {
        var k = d.length;
        var secretLen = d[0].length / 2;

        var sharesPtr = Module._malloc(k * sizeofShare());
        var secretPtr = Module._malloc(secretLen);

        for (var si = 0; si < k; si++) {
            var sp = sharesPtr + si * sizeofShare();
            Module.setValue(sp, x[si], 'i8');                 // share.x
            for (var j = 0; j < secretLen; j++) {
                var byteVal = parseInt(d[si].substr(j * 2, 2), 16);
                Module.setValue(sp + 1 + j, byteVal, 'i8');   // share.data[j]
            }
            Module.setValue(sp + 260, secretLen, 'i32');      // share.len
        }

        var ret = Module._sss_combine_wasm(sharesPtr, k, secretPtr, secretLen);
        if (ret !== 0) { showToast('Reconstruction failed'); return; }

        var secret = '';
        for (var bi = 0; bi < secretLen; bi++) {
            secret += String.fromCharCode(Module.getValue(secretPtr + bi, 'i8') & 0xFF);
        }

        Module._free(sharesPtr);
        Module._free(secretPtr);

        resultText.textContent = secret || '(empty)';
        resultBox.classList.remove('hidden');
        showToast('Reconstructed!');
    }

    /* ── Fetch implementation (device) ── */
    function reconstructFetch(d, x) {
        var url = '/reconstruct?d=' + d.join(',') + '&x=' + x.join(',');
        fetch(url)
            .then(function (r) {
                if (!r.ok) throw new Error('HTTP ' + r.status);
                return r.json();
            })
            .then(function (data) {
                resultText.textContent = data.secret || '(empty)';
                resultBox.classList.remove('hidden');
            })
            .catch(function (err) { showToast('Error: ' + err.message); });
    }

    /* ── Entry point ── */
    function reconstruct() {
        var parsed = parseShares();
        if (parsed.d.length < 2) { showToast('Enter at least 2 shares'); return; }

        if (useWasm && Module && Module._sss_combine_wasm) {
            reconstructWasm(parsed.d, parsed.x);
        } else {
            reconstructFetch(parsed.d, parsed.x);
        }
    }

    unsplitBtn.addEventListener('click', reconstruct);

    shareRows.forEach(function (row) {
        var dInput = row.querySelector('.d-input');
        dInput.addEventListener('keydown', function (e) {
            if (e.key === 'Enter') reconstruct();
        });
    });

    copyResultBtn.addEventListener('click', function () {
        copyText(resultText.textContent, copyResultBtn);
    });

    /* ── QR Code scanner (dual-mode: live camera or file picker) ── */
    var qrFileInput = document.getElementById('qr-file-input');
    var qrOverlay    = document.getElementById('qr-overlay');
    var qrVideo      = document.getElementById('qr-video');
    var qrStatus     = document.getElementById('qr-status');
    var qrClose      = document.getElementById('qr-close');
    var qrCanvas     = document.createElement('canvas');
    var qrCtx        = qrCanvas.getContext('2d', { willReadFrequently: true });
    var targetRow    = null;
    var qrStream     = null;
    var qrAnim       = null;
    var cameraFailed = false;

    var qrBtns = document.querySelectorAll('.qr-btn');
    qrBtns.forEach(function (btn) {
        btn.addEventListener('click', function () {
            targetRow = parseInt(btn.getAttribute('data-row'), 10);
            openQRScanner();
        });
    });

    function openQRScanner() {
        // Only try live camera on secure contexts (HTTPS / localhost).
        // On HTTP (ESP32 on 192.168.4.1) or after a previous failure we go
        // directly to file picker — synchronous .click() works on iOS Safari.
        if (!cameraFailed && window.isSecureContext &&
            navigator.mediaDevices && navigator.mediaDevices.getUserMedia) {
            navigator.mediaDevices.getUserMedia({
                video: { facingMode: 'environment', width: { ideal: 640 }, height: { ideal: 480 } }
            })
                .then(function (stream) {
                    qrStream = stream;
                    qrVideo.srcObject = stream;
                    qrVideo.play();
                    qrOverlay.classList.remove('hidden');
                    qrStatus.textContent = 'Point the camera at a QR code';
                    startQRScan();
                })
                .catch(function () {
                    cameraFailed = true;
                    showToast('Camera unavailable — tap \uD83D\uDCF7 again');
                });
        } else {
            // Synchronous user-gesture path — no setTimeout!
            qrFileInput.click();
        }
    }

    function startQRScan() {
        function tick() {
            if (qrVideo.readyState >= qrVideo.HAVE_ENOUGH_DATA && qrVideo.videoWidth > 0) {
                var w = qrVideo.videoWidth, h = qrVideo.videoHeight;
                qrCanvas.width = w; qrCanvas.height = h;
                qrCtx.drawImage(qrVideo, 0, 0, w, h);
                var code = jsQR(qrCtx.getImageData(0, 0, w, h).data, w, h,
                    { inversionAttempts: 'attemptBoth' });
                if (code && code.data) {
                    stopQRScan();
                    fillShareFromQR(code.data.trim());
                    return;
                }
            }
            qrAnim = requestAnimationFrame(tick);
        }
        qrAnim = requestAnimationFrame(tick);
    }

    function stopQRScan() {
        if (qrAnim) { cancelAnimationFrame(qrAnim); qrAnim = null; }
        if (qrStream) { qrStream.getTracks().forEach(function (t) { t.stop(); }); qrStream = null; }
        qrVideo.srcObject = null;
        qrOverlay.classList.add('hidden');
    }

    qrClose.addEventListener('click', stopQRScan);
    qrOverlay.addEventListener('click', function (e) {
        if (e.target === qrOverlay) stopQRScan();
    });

    /* ── File / gallery scan ── */
    qrFileInput.addEventListener('change', function () {
        var file = qrFileInput.files[0];
        if (!file) return;
        qrFileInput.value = '';   // allow re-selecting the same file

        var url = URL.createObjectURL(file);
        var img = new Image();
        img.onload = function () {
            URL.revokeObjectURL(url);
            var text = decodeFromImg(img);
            if (text) { fillShareFromQR(text); }
            else { showToast('No QR code found — try a clearer photo'); }
        };
        img.onerror = function () {
            URL.revokeObjectURL(url);
            showToast('Could not load the image');
        };
        img.src = url;
    });

    /* ── Multi-scale × 4-rotation decode (handles iPhone EXIF) ──
       Each rotation + scale combo uses its own canvas dimensions so
       getImageData never overflows.  jsQR works best when the QR module
       area is roughly 200-1500 px wide; we sweep 4 targets × 4 rotations.
       Rendering is nearest-neighbour when downscaling > 2× — keeps QR
       module edges sharp instead of blurring them via bilinear resample. ── */
    function decodeFromImg(img) {
        var iw = img.naturalWidth, ih = img.naturalHeight;
        if (!iw || !ih) return null;

        var targets = [1000, 1500, 600, 400];

        // { drawWidth, drawHeight, rotation-radians, translateX, translateY }
        var steps = [
            { dw: iw,  dh: ih,  angle: 0,            tx: 0,   ty: 0   },
            { dw: ih,  dh: iw,  angle:  Math.PI / 2,  tx: ih,  ty: 0   },
            { dw: iw,  dh: ih,  angle:  Math.PI,      tx: iw,  ty: ih  },
            { dw: ih,  dh: iw,  angle: -Math.PI / 2,  tx: 0,   ty: iw  }
        ];

        for (var si = 0; si < steps.length; si++) {
            var s = steps[si];
            for (var ti = 0; ti < targets.length; ti++) {
                var longSide = Math.max(s.dw, s.dh);
                var scale = Math.min(1, targets[ti] / longSide);
                var w = Math.round(s.dw * scale);
                var h = Math.round(s.dh * scale);
                if (w < 60 || h < 60) continue;

                // Resize canvas for this specific rotation + scale.
                qrCanvas.width  = w;
                qrCanvas.height = h;
                qrCtx.imageSmoothingEnabled = (scale > 0.5);

                if (s.angle === 0) {
                    qrCtx.setTransform(1, 0, 0, 1, 0, 0);
                    qrCtx.drawImage(img, 0, 0, w, h);
                } else {
                    qrCtx.setTransform(
                         Math.cos(s.angle) * scale, Math.sin(s.angle) * scale,
                        -Math.sin(s.angle) * scale, Math.cos(s.angle) * scale,
                        s.tx * scale, s.ty * scale
                    );
                    qrCtx.drawImage(img, 0, 0);
                }

                var code = jsQR(
                    qrCtx.getImageData(0, 0, w, h).data, w, h,
                    { inversionAttempts: 'attemptBoth' }
                );
                if (code && code.data) return code.data.trim();
            }
        }
        return null;
    }

    function fillShareFromQR(text) {
        var row = shareRows[targetRow];
        if (!row) return;

        var xInput = row.querySelector('.x-input');
        var dInput = row.querySelector('.d-input');

        var colon = text.indexOf(':');
        if (colon !== -1) {
            var xVal = text.substring(0, colon);
            var dVal = text.substring(colon + 1);
            xInput.value = parseInt(xVal, 10) || 1;
            dInput.value = dVal;
        } else {
            dInput.value = text;
        }

        showToast('QR code scanned!');
    }

})();
