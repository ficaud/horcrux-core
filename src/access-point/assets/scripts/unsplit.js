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

    /* ── QR Code scanner (dual-mode: live camera or capture) ── */
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

    var qrBtns = document.querySelectorAll('.qr-btn');
    qrBtns.forEach(function (btn) {
        btn.addEventListener('click', function () {
            targetRow = parseInt(btn.getAttribute('data-row'), 10);
            openQRScanner();
        });
    });

    function openQRScanner() {
        /* Try live camera first (needs HTTPS or localhost) */
        if (navigator.mediaDevices && navigator.mediaDevices.getUserMedia) {
            navigator.mediaDevices.getUserMedia({ video: { facingMode: 'environment', width: { ideal: 640 }, height: { ideal: 480 } } })
                .then(function (stream) {
                    qrStream = stream;
                    qrVideo.srcObject = stream;
                    qrVideo.play();
                    qrOverlay.classList.remove('hidden');
                    qrStatus.textContent = 'Point the camera at a QR code';
                    startQRScan();
                })
                .catch(function () {
                    /* getUserMedia failed — show guide, then file input */
                    showToast('📷 Pick from Photo Library (pre-take QR photo with Camera app)');
                    setTimeout(function () { qrFileInput.click(); }, 800);
                });
        } else {
            /* No getUserMedia — show guide, then file input */
            showToast('📷 Pick from Photo Library (pre-take QR photo with Camera app)');
            setTimeout(function () { qrFileInput.click(); }, 800);
        }
    }

    function startQRScan() {
        function tick() {
            if (qrVideo.readyState >= qrVideo.HAVE_ENOUGH_DATA && qrVideo.videoWidth > 0) {
                var w = qrVideo.videoWidth, h = qrVideo.videoHeight;
                if (qrCanvas.width !== w || qrCanvas.height !== h) {
                    qrCanvas.width = w; qrCanvas.height = h;
                }
                qrCtx.drawImage(qrVideo, 0, 0, w, h);
                var imgData = qrCtx.getImageData(0, 0, w, h);
                var code = jsQR(imgData.data, w, h, { inversionAttempts: 'attemptBoth' });
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

    /* Fallback: decode from file/capture image */
    qrFileInput.addEventListener('change', function () {
        var file = qrFileInput.files[0];
        if (!file) return;
        var reader = new FileReader();
        reader.onload = function (e) {
            var img = new Image();
            img.onload = function () {
                /* Downscale large images — jsQR works best at moderate resolution */
                var MAX = 800;
                var w = img.width, h = img.height;
                if (w > MAX || h > MAX) {
                    var ratio = Math.min(MAX / w, MAX / h);
                    w = Math.round(w * ratio);
                    h = Math.round(h * ratio);
                }
                var code = decodeQRFromImage(img, w, h);
                if (code) {
                    fillShareFromQR(code);
                } else {
                    showToast('No QR code found in image');
                }
            };
            img.src = e.target.result;
        };
        reader.readAsDataURL(file);
    });

    /* Try all 4 rotations — handles EXIF-rotated iPhone photos */
    function decodeQRFromImage(img, w, h) {
        qrCanvas.width = w; qrCanvas.height = h;

        /* 0° */   qrCtx.drawImage(img, 0, 0, w, h);
        var r = tryDecode(w, h); if (r) return r;

        /* 90° */  qrCtx.save(); qrCtx.translate(h, 0); qrCtx.rotate(Math.PI / 2);
                    qrCtx.drawImage(img, 0, 0, w, h); qrCtx.restore();
        r = tryDecode(h, w); if (r) return r;

        /* 180° */ qrCtx.save(); qrCtx.translate(w, h); qrCtx.rotate(Math.PI);
                    qrCtx.drawImage(img, 0, 0, w, h); qrCtx.restore();
        r = tryDecode(w, h); if (r) return r;

        /* 270° */ qrCtx.save(); qrCtx.translate(0, w); qrCtx.rotate(-Math.PI / 2);
                    qrCtx.drawImage(img, 0, 0, w, h); qrCtx.restore();
        r = tryDecode(h, w); if (r) return r;

        return null;
    }

    function tryDecode(w, h) {
        var imgData = qrCtx.getImageData(0, 0, w, h);
        var code = jsQR(imgData.data, w, h, { inversionAttempts: 'attemptBoth' });
        return code ? code.data.trim() : null;
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
