/**
 * split.js — Handles the split/encrypt flow on the captive portal.
 *
 * Reads a secret from the input, sends it to /divide, and displays
 * the resulting shares with copy-to-clipboard buttons.
 */

(function () {
    var msgInput = document.getElementById('msg-input');
    var msgBtn = document.getElementById('msg-btn');
    var shareList = document.getElementById('share-list');
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
            // Fallback for older browsers
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

            var shareText = s.x + ':' + s.d; // e.g. "3:60b5b9cfc025"
            btn.addEventListener('click', function () {
                copyText(shareText, btn);
            });

            shareList.appendChild(item);

            if (i > 0) allText += '\n';
            allText += shareText;
        });

        // "Copy all" button
        var copyAllBtn = document.createElement('button');
        copyAllBtn.className = 'copy-all';
        copyAllBtn.textContent = '\u{1F4CB} Copy all shares';
        copyAllBtn.addEventListener('click', function () {
            copyText(allText, copyAllBtn);
        });
        shareList.appendChild(copyAllBtn);
    }

    function encrypt() {
        var msg = msgInput.value;
        if (msg.trim() === '') return;

        fetch('/divide?msg=' + encodeURIComponent(msg))
            .then(function (r) {
                if (!r.ok) throw new Error('HTTP ' + r.status);
                return r.json();
            })
            .then(function (data) {
                displayShares(data);
            })
            .catch(function (err) {
                showToast('Error: ' + err.message);
            });
    }

    msgBtn.addEventListener('click', encrypt);
    msgInput.addEventListener('keydown', function (e) {
        if (e.key === 'Enter') encrypt();
    });
})();
