/**
 * app.js — Horcrux WASM demo
 *
 * Bridges the C SSS implementation via Emscripten and manages the UI.
 */

(function () {
    'use strict';

    // ─── Module initialisation ───

    function wasmReady() {
        return new Promise(function (resolve) {
            var iv = setInterval(function () {
                if (Module && Module._sss_split_wasm && Module._malloc) {
                    clearInterval(iv);
                    resolve();
                }
            }, 10);
        });
    }

    function sizeofShare() {
        // struct sss_share (horcrux-connect): uint8_t(1) + uint8_t[256](1) + padding(3) + size_t(4)
        // On WASM (32-bit) = 1 + 256 + 3 + 4 = 264 bytes
        return 264;
    }

    function makeShare(ptr) {
        var x = Module.getValue(ptr, "i8") & 0xFF;          // offset 0
        var dataPtr = ptr + 1;                               // offset 1: data[256]
        var len = Module.getValue(ptr + 260, "i32");         // offset 260: size_t (aligned)
        var data = '';
        for (var i = 0; i < len; i++) {
            var b = Module.getValue(dataPtr + i, "i8") & 0xFF;
            data += ('0' + b.toString(16)).slice(-2);
        }
        return { x: x, len: len, d: data };
    }

    // ─── Split logic ───

    var msgInput = document.getElementById('msg-input');
    var msgBtn = document.getElementById('msg-btn');
    var shareList = document.getElementById('share-list');
    var toast = document.getElementById('toast');

    function showToast(text, isError) {
        toast.textContent = text;
        toast.style.background = isError ? '#e74c3c' : '#4caf50';
        toast.classList.add('visible');
        setTimeout(function () { toast.classList.remove('visible'); }, 2000);
    }

    function copyText(text, btn) {
        if (navigator.clipboard) {
            navigator.clipboard.writeText(text).then(function () {
                btn.classList.add('copied');
                setTimeout(function () { btn.classList.remove('copied'); }, 1000);
                showToast('Copied!');
            });
        } else {
            var ta = document.createElement('textarea');
            ta.value = text; ta.style.position = 'fixed'; ta.style.opacity = '0';
            document.body.appendChild(ta); ta.select();
            document.execCommand('copy');
            document.body.removeChild(ta);
            btn.classList.add('copied');
            setTimeout(function () { btn.classList.remove('copied'); }, 1000);
            showToast('Copied!');
        }
    }

    function doSplit() {
        var msg = msgInput.value;
        if (msg.trim() === '') return;

        wasmReady().then(function () {
            var secret = msg;
            var secretLen = secret.length;
            var n = 5, k = 3;

            // Seed PRNG from browser crypto
            var seed = 0;
            if (window.crypto && window.crypto.getRandomValues) {
                var a = new Uint32Array(1);
                window.crypto.getRandomValues(a);
                seed = a[0];
            } else {
                seed = Date.now() ^ Math.random() * 0xFFFFFFFF;
            }
            Module._wasm_set_seed(seed);

            // Allocate memory
            var secretPtr = Module._malloc(secretLen);
            var sharesPtr = Module._malloc(n * sizeofShare());

            // Copy secret to wasm heap
            for (var i = 0; i < secretLen; i++) {
                Module.setValue(secretPtr + i, secret.charCodeAt(i), "i8");
            }

            // Call sss_split
            var ret = Module._sss_split_wasm(secretPtr, secretLen, n, k, sharesPtr);

            if (ret !== 0) {
                showToast('Split failed', true);
                Module._free(secretPtr);
                Module._free(sharesPtr);
                return;
            }

            // Read shares from wasm heap
            var shares = [];
            for (var si = 0; si < n; si++) {
                var sharePtr = sharesPtr + si * sizeofShare();
                shares.push(makeShare(sharePtr));
            }

            Module._free(secretPtr);
            Module._free(sharesPtr);

            // Display shares
            displayShares(shares);
        });
    }

    function displayShares(shares) {
        shareList.innerHTML = '';
        shareList.classList.remove('hidden');

        var allText = '';

        shares.forEach(function (s, i) {
            var item = document.createElement('div');
            item.className = 'share-item';

            var label = document.createElement('span');
            label.className = 'share-label';
            label.textContent = '#' + (i + 1);
            item.appendChild(label);

            var data = document.createElement('span');
            data.className = 'share-data';
            data.textContent = s.d;
            item.appendChild(data);

            var btn = document.createElement('button');
            btn.className = 'copy-btn';
            btn.textContent = '\u{1F4CB}';
            btn.title = 'Copy share #' + (i + 1);
            item.appendChild(btn);

            var shareText = s.x + ':' + s.d;
            btn.addEventListener('click', function () {
                copyText(shareText, btn);
            });

            shareList.appendChild(item);

            if (i > 0) allText += '\n';
            allText += shareText;
        });

        var copyAllBtn = document.createElement('button');
        copyAllBtn.className = 'copy-all';
        copyAllBtn.textContent = '\u{1F4CB} Copy all shares';
        copyAllBtn.addEventListener('click', function () {
            copyText(allText, copyAllBtn);
        });
        shareList.appendChild(copyAllBtn);
    }

    msgBtn.addEventListener('click', doSplit);
    msgInput.addEventListener('keydown', function (e) {
        if (e.key === 'Enter') doSplit();
    });

    // ─── Unsplit (reconstruct) logic ───

    var sharesTextarea = document.getElementById('shares-textarea');
    var reconstructBtn = document.getElementById('reconstruct-btn');
    var resultBox = document.getElementById('result-box');
    var resultText = document.getElementById('result-text');
    var copyResultBtn = document.getElementById('copy-result');

    function doReconstruct() {
        var text = sharesTextarea.value.trim();
        if (text === '') return;

        var lines = text.split('\n').map(function (l) { return l.trim(); }).filter(Boolean);
        if (lines.length < 2) {
            showToast('Enter at least 2 shares', true);
            return;
        }

        wasmReady().then(function () {
            var d = [], x = [];

            lines.forEach(function (line) {
                var colon = line.indexOf(':');
                if (colon === -1) { showToast('Invalid format: ' + line, true); return; }
                var xVal = parseInt(line.substring(0, colon), 10);
                var hexVal = line.substring(colon + 1);
                if (isNaN(xVal) || xVal < 1 || xVal > 255) {
                    showToast('Invalid x: ' + line, true); return;
                }
                if (hexVal.length % 2 !== 0 || hexVal.length === 0) {
                    showToast('Invalid hex: ' + line, true); return;
                }
                x.push(xVal);
                d.push(hexVal);
            });

            if (d.length < 2) return;

            var k = d.length;
            var secretLen = d[0].length / 2; // bytes

            // Allocate memory
            var sharesPtr = Module._malloc(k * sizeofShare());
            var secretPtr = Module._malloc(secretLen);

            // Write shares to wasm heap (horcrux-connect struct layout)
            for (var si = 0; si < k; si++) {
                var sp = sharesPtr + si * sizeofShare();
                Module.setValue(sp, x[si], "i8");                         // share.x  (offset 0)
                for (var j = 0; j < secretLen; j++) {
                    var byteVal = parseInt(d[si].substr(j * 2, 2), 16);
                    Module.setValue(sp + 1 + j, byteVal, "i8");           // share.data[j] (offset 1)
                }
                Module.setValue(sp + 260, secretLen, "i32");              // share.len (offset 260)
            }

            // Call sss_combine
            var ret = Module._sss_combine_wasm(sharesPtr, k, secretPtr, secretLen);

            if (ret !== 0) {
                showToast('Reconstruction failed', true);
                Module._free(sharesPtr);
                Module._free(secretPtr);
                return;
            }

            // Read secret from wasm heap
            var secret = '';
            for (var bi = 0; bi < secretLen; bi++) {
                secret += String.fromCharCode(Module.getValue(secretPtr + bi, "i8") & 0xFF);
            }

            Module._free(sharesPtr);
            Module._free(secretPtr);

            resultText.textContent = secret;
            resultBox.classList.remove('hidden');
            showToast('Reconstructed!');
        });
    }

    reconstructBtn.addEventListener('click', doReconstruct);

    copyResultBtn.addEventListener('click', function () {
        copyText(resultText.textContent, copyResultBtn);
    });

    // ─── Tab switching ───

    var tabSplit = document.getElementById('tab-split');
    var tabUnsplit = document.getElementById('tab-unsplit');
    var pageSplit = document.getElementById('page-split');
    var pageUnsplit = document.getElementById('page-unsplit');

    function showTab(name) {
        tabSplit.classList.toggle('active', name === 'split');
        tabUnsplit.classList.toggle('active', name === 'unsplit');
        pageSplit.classList.toggle('active', name === 'split');
        pageUnsplit.classList.toggle('active', name === 'unsplit');
    }

    tabSplit.addEventListener('click', function () { showTab('split'); });
    tabUnsplit.addEventListener('click', function () { showTab('unsplit'); });

    // Welcome toast
    wasmReady().then(function () {
        showToast('WASM module loaded');
    });
})();
