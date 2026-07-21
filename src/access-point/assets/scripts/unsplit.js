/**
 * unsplit.js — Handles the share reconstruction flow.
 *
 * Collects shares from 3 input rows, sends them to /reconstruct,
 * and displays the recovered secret with a copy button.
 */

(function () {
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

    function reconstruct() {
        var d = [];
        var x = [];

        shareRows.forEach(function (row) {
            var xInput = row.querySelector('.x-input');
            var dInput = row.querySelector('.d-input');
            var val = dInput.value.trim();
            if (val === '') return;

            /* Support "x:hex" format (e.g. "3:60b5b9cfc025") */
            var colon = val.indexOf(':');
            if (colon !== -1) {
                var prefix = val.substring(0, colon);
                var hexPart = val.substring(colon + 1);
                x.push(prefix);
                d.push(hexPart);
            } else {
                /* Plain hex — use the separate x input */
                var xVal = xInput.value.trim();
                x.push(xVal || '0');
                d.push(val);
            }
        });

        if (d.length < 2) {
            showToast('Enter at least 2 shares');
            return;
        }

        var url = '/reconstruct?d=' + d.join(',') +
                  '&x=' + x.join(',');

        fetch(url)
            .then(function (r) {
                if (!r.ok) throw new Error('HTTP ' + r.status);
                return r.json();
            })
            .then(function (data) {
                resultText.textContent = data.secret || '(empty)';
                resultBox.classList.remove('hidden');
            })
            .catch(function (err) {
                showToast('Error: ' + err.message);
            });
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
