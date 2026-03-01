(function () {
    'use strict';

    // --- DOM refs ---
    var profileSelect = document.getElementById('profile-select');
    var speedInput = document.getElementById('speed-input');
    var startBtn = document.getElementById('start-btn');
    var stopBtn = document.getElementById('stop-btn');
    var wpmLabel = document.getElementById('wpm-label');
    var kbEl = document.getElementById('kb');
    var connDot = document.getElementById('conn-dot');
    var connDotPortrait = document.getElementById('conn-dot-portrait');

    // --- Keyboard layers ---
    var LAYER_ALPHA = [
        ['Q','W','E','R','T','Y','U','I','O','P'],
        ['A','S','D','F','G','H','J','K','L'],
        ['Z','X','C','V','B','N','M']
    ];
    var LAYER_NUM = [
        ['1','2','3','4','5','6','7','8','9','0'],
        ['+','-','.',',','/',':', '=','?','('],
        [')','"','\'','@','!','&']
    ];

    var currentLayer = 0; // 0 = alpha, 1 = num

    function buildKeyboard() {
        var layers = currentLayer === 0 ? LAYER_ALPHA : LAYER_NUM;
        var toggleLabel = currentLayer === 0 ? '123' : 'ABC';
        kbEl.innerHTML = '';

        for (var r = 0; r < layers.length; r++) {
            var rowDiv = document.createElement('div');
            rowDiv.className = 'kb-row';
            var row = layers[r];
            for (var c = 0; c < row.length; c++) {
                var btn = document.createElement('button');
                btn.className = 'key';
                btn.textContent = row[c];
                btn.dataset.char = row[c];
                rowDiv.appendChild(btn);
            }
            kbEl.appendChild(rowDiv);
        }

        // Toggle row
        var toggleRow = document.createElement('div');
        toggleRow.className = 'kb-row';
        var toggleBtn = document.createElement('button');
        toggleBtn.className = 'key key-toggle';
        toggleBtn.textContent = toggleLabel;
        toggleBtn.dataset.toggle = '1';
        toggleRow.appendChild(toggleBtn);
        kbEl.appendChild(toggleRow);
    }

    // --- Keyboard input via event delegation ---
    kbEl.addEventListener('pointerdown', function (e) {
        var btn = e.target.closest('.key');
        if (!btn) return;
        e.preventDefault();

        if (btn.dataset.toggle) {
            currentLayer = currentLayer === 0 ? 1 : 0;
            buildKeyboard();
            return;
        }

        if (btn.dataset.char) {
            send({ type: 'key', char: btn.dataset.char });
        }
    });

    // Physical / bluetooth keyboard support
    document.addEventListener('keydown', function (e) {
        if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return;
        if (e.metaKey || e.ctrlKey || e.altKey) return;

        var ch = e.key;
        if (ch.length === 1 && /[a-zA-Z0-9+\-.,\/:=?()'"@!&]/.test(ch)) {
            e.preventDefault();
            send({ type: 'key', char: ch.toUpperCase() });
        }
    });

    // --- WebSocket ---
    var ws = null;
    var running = false;

    function connect() {
        if (ws) {
            ws.onclose = null;
            ws.onerror = null;
            ws.close();
            ws = null;
        }
        var host = window.location.hostname || '192.168.4.1';
        var port = window.location.port || '80';
        ws = new WebSocket('ws://' + host + ':' + port + '/ws');

        ws.onopen = function () {
            setConnected(true);
            send({ type: 'command', cmd: 'status' });
        };

        ws.onclose = function () {
            setConnected(false);
            ws = null;
            setTimeout(connect, 2000);
        };

        ws.onerror = function () {
            ws.close();
        };

        ws.onmessage = function (event) {
            var msg;
            try { msg = JSON.parse(event.data); } catch (e) { return; }
            handleMessage(msg);
        };
    }

    function send(obj) {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify(obj));
        }
    }

    function setConnected(state) {
        var cls = state ? 'conn-dot connected' : 'conn-dot';
        connDot.className = cls;
        connDotPortrait.className = cls;
    }

    // --- Message handling ---
    function handleMessage(msg) {
        switch (msg.type) {
            case 'result':
                flash(msg.correct);
                break;

            case 'session':
                if (msg.state === 'started') {
                    setRunning(true);
                    updateSpeed(msg.speed || speedInput.value);
                } else {
                    setRunning(false);
                }
                break;

            case 'speed_change':
                updateSpeed(msg.speed);
                break;

            case 'status':
                if (msg.running) setRunning(true);
                else setRunning(false);
                updateSpeed(msg.speed);
                if (msg.profile !== undefined) {
                    profileSelect.value = msg.profile;
                }
                break;

            case 'context_lost':
                updateSpeed(msg.speed);
                flash(false);
                break;
        }
    }

    // --- Flash feedback ---
    var flashTimer = null;

    function flash(correct) {
        var cls = correct ? 'flash-correct' : 'flash-wrong';
        document.body.classList.remove('flash-correct', 'flash-wrong');
        // Force reflow so re-adding the class triggers the visual change
        void document.body.offsetWidth;
        document.body.classList.add(cls);

        clearTimeout(flashTimer);
        flashTimer = setTimeout(function () {
            document.body.classList.remove(cls);
        }, 300);
    }

    // --- UI state ---
    function setRunning(state) {
        running = state;
        startBtn.disabled = state;
        stopBtn.disabled = !state;
    }

    function updateSpeed(speed) {
        speedInput.value = speed;
        wpmLabel.textContent = speed + ' WPM';
    }

    // --- Buttons ---
    startBtn.addEventListener('click', function () {
        var profile = parseInt(profileSelect.value, 10);
        var speed = parseInt(speedInput.value, 10);
        send({ type: 'command', cmd: 'start', profile: profile, speed: speed });
    });

    stopBtn.addEventListener('click', function () {
        send({ type: 'command', cmd: 'stop' });
    });

    // --- Init ---
    buildKeyboard();
    connect();
})();
