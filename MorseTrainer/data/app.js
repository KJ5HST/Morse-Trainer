// === Morse Trainer Web UI ===

(function () {
    'use strict';

    // --- DOM refs ---
    const connStatus = document.getElementById('connection-status');
    const profileSelect = document.getElementById('profile-select');
    const speedInput = document.getElementById('speed-input');
    const startBtn = document.getElementById('start-btn');
    const stopBtn = document.getElementById('stop-btn');
    const currentSpeedEl = document.getElementById('current-speed');
    const correctCountEl = document.getElementById('correct-count');
    const wrongCountEl = document.getElementById('wrong-count');
    const accuracyEl = document.getElementById('accuracy');
    const toneIndicator = document.getElementById('tone-indicator');
    const sentCharsEl = document.getElementById('sent-chars');
    const resultFeed = document.getElementById('result-feed');
    const charInput = document.getElementById('char-input');
    const heatmapEl = document.getElementById('heatmap');

    // --- State ---
    let ws = null;
    let audioCtx = null;
    let oscillator = null;
    let gainNode = null;
    let toneActive = false;
    let correctCount = 0;
    let wrongCount = 0;
    let sentChars = '';
    let running = false;

    // --- Web Audio ---
    function initAudio() {
        if (audioCtx) return;
        audioCtx = new (window.AudioContext || window.webkitAudioContext)();
        oscillator = audioCtx.createOscillator();
        gainNode = audioCtx.createGain();
        oscillator.type = 'sine';
        oscillator.frequency.setValueAtTime(800, audioCtx.currentTime);
        gainNode.gain.setValueAtTime(0, audioCtx.currentTime);
        oscillator.connect(gainNode);
        gainNode.connect(audioCtx.destination);
        oscillator.start();
    }

    function toneOn() {
        if (!audioCtx) initAudio();
        if (audioCtx.state === 'suspended') audioCtx.resume();
        // Click-free ramp
        gainNode.gain.cancelScheduledValues(audioCtx.currentTime);
        gainNode.gain.setTargetAtTime(0.3, audioCtx.currentTime, 0.005);
        toneIndicator.className = 'on';
        toneActive = true;
    }

    function toneOff() {
        if (!audioCtx || !toneActive) return;
        gainNode.gain.cancelScheduledValues(audioCtx.currentTime);
        gainNode.gain.setTargetAtTime(0, audioCtx.currentTime, 0.005);
        toneIndicator.className = 'off';
        toneActive = false;
    }

    // --- WebSocket ---
    function connect() {
        const host = window.location.hostname || '192.168.4.1';
        const port = window.location.port || '80';
        ws = new WebSocket('ws://' + host + ':' + port + '/ws');

        ws.onopen = function () {
            connStatus.textContent = 'Connected';
            connStatus.className = 'status connected';
        };

        ws.onclose = function () {
            connStatus.textContent = 'Disconnected';
            connStatus.className = 'status disconnected';
            toneOff();
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

    // --- Message handlers ---
    function handleMessage(msg) {
        switch (msg.type) {
            case 'morse_element':
                if (msg.state === 'on') toneOn();
                else toneOff();
                break;

            case 'char_sent':
                if (msg.char && msg.char !== ' ') {
                    sentChars += msg.char;
                    if (sentChars.length > 40) sentChars = sentChars.slice(-40);
                    sentCharsEl.textContent = sentChars;
                } else {
                    sentChars += ' ';
                    sentCharsEl.textContent = sentChars;
                }
                break;

            case 'result':
                if (msg.correct) {
                    correctCount++;
                    addResult('OK: ' + msg.typed, true);
                } else {
                    wrongCount++;
                    addResult('ERR: typed ' + msg.typed + ', expected ' + msg.expected, false);
                }
                updateStats();
                // Request updated probabilities
                send({ type: 'command', cmd: 'probs' });
                break;

            case 'speed_change':
                currentSpeedEl.textContent = msg.speed;
                addResult('Speed ' + msg.direction + ' to ' + msg.speed + ' WPM', msg.direction === 'up');
                break;

            case 'session':
                if (msg.state === 'started') {
                    setRunning(true);
                    currentSpeedEl.textContent = msg.speed || speedInput.value;
                    send({ type: 'command', cmd: 'probs' });
                } else {
                    setRunning(false);
                    toneOff();
                }
                break;

            case 'context_lost':
                addResult('Context lost! Speed down to ' + msg.speed + ' WPM', false);
                currentSpeedEl.textContent = msg.speed;
                break;

            case 'probs':
                updateHeatmap(msg.data);
                break;

            case 'status':
                if (msg.running) setRunning(true);
                currentSpeedEl.textContent = msg.speed;
                break;
        }
    }

    function addResult(text, isCorrect) {
        var div = document.createElement('div');
        div.className = 'result-entry ' + (isCorrect ? 'correct' : 'wrong');
        div.textContent = text;
        resultFeed.prepend(div);
        // Limit entries
        while (resultFeed.children.length > 50) {
            resultFeed.removeChild(resultFeed.lastChild);
        }
    }

    function updateStats() {
        correctCountEl.textContent = correctCount;
        wrongCountEl.textContent = wrongCount;
        var total = correctCount + wrongCount;
        if (total > 0) {
            accuracyEl.textContent = Math.round((correctCount / total) * 100);
        }
    }

    function setRunning(state) {
        running = state;
        startBtn.disabled = state;
        stopBtn.disabled = !state;
        charInput.disabled = !state;
        if (state) {
            charInput.focus();
        }
    }

    // --- Heatmap ---
    function updateHeatmap(data) {
        heatmapEl.innerHTML = '';
        if (!data) return;

        // Find max probability for scaling
        var maxProb = 1;
        for (var i = 0; i < data.length; i++) {
            if (data[i].prob > maxProb) maxProb = data[i].prob;
        }

        for (var j = 0; j < data.length; j++) {
            var item = data[j];
            var cell = document.createElement('div');
            cell.className = 'heatmap-cell';

            // Color: low prob = green, high prob = red
            var ratio = item.prob / maxProb;
            var r = Math.round(233 * ratio);
            var g = Math.round(155 * (1 - ratio));
            var b = 30;
            cell.style.background = 'rgb(' + r + ',' + g + ',' + b + ')';

            var charSpan = document.createElement('span');
            charSpan.className = 'char';
            charSpan.textContent = item.char;

            var probSpan = document.createElement('span');
            probSpan.className = 'prob';
            probSpan.textContent = item.prob;

            cell.appendChild(charSpan);
            cell.appendChild(probSpan);
            heatmapEl.appendChild(cell);
        }
    }

    // --- Keyboard input ---
    document.addEventListener('keydown', function (e) {
        if (!running) return;
        if (e.target.tagName === 'SELECT' || e.target.tagName === 'INPUT' && e.target.type === 'number') return;

        var key = e.key;
        if (key.length === 1 && /[a-zA-Z0-9 .,;:!?/=+\-_'"@&()]/.test(key)) {
            e.preventDefault();
            var ch = key.toUpperCase();
            send({ type: 'key', char: ch });
            charInput.value = ch;
            // Flash input
            charInput.style.borderColor = '#ffcc00';
            setTimeout(function () { charInput.style.borderColor = ''; }, 150);
        }
    });

    charInput.addEventListener('input', function () {
        if (!running) return;
        var ch = charInput.value.toUpperCase();
        if (ch.length > 0) {
            send({ type: 'key', char: ch });
            charInput.value = '';
        }
    });

    // --- Buttons ---
    startBtn.addEventListener('click', function () {
        initAudio();
        if (audioCtx.state === 'suspended') audioCtx.resume();

        var profile = parseInt(profileSelect.value, 10);
        var speed = parseInt(speedInput.value, 10);

        // Reset counters
        correctCount = 0;
        wrongCount = 0;
        sentChars = '';
        sentCharsEl.textContent = '';
        resultFeed.innerHTML = '';
        updateStats();

        send({ type: 'command', cmd: 'start', profile: profile, speed: speed });
    });

    stopBtn.addEventListener('click', function () {
        send({ type: 'command', cmd: 'stop' });
        toneOff();
    });

    // --- Init ---
    connect();
})();
