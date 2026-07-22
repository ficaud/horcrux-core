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
})();
