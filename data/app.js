// === Morse Trainer Web UI — Config & Stats Dashboard ===

(function () {
    'use strict';

    // --- DOM refs ---
    var connStatus = document.getElementById('connection-status');
    var profileSelect = document.getElementById('profile-select');
    var speedInput = document.getElementById('speed-input');
    var startBtn = document.getElementById('start-btn');
    var stopBtn = document.getElementById('stop-btn');
    var currentSpeedEl = document.getElementById('current-speed');
    var correctCountEl = document.getElementById('correct-count');
    var wrongCountEl = document.getElementById('wrong-count');
    var accuracyEl = document.getElementById('accuracy');
    var sentCharsEl = document.getElementById('sent-chars');
    var resultFeed = document.getElementById('result-feed');
    var heatmapEl = document.getElementById('heatmap');

    // --- State ---
    var ws = null;
    var correctCount = 0;
    var wrongCount = 0;
    var sentChars = '';
    var running = false;

    // --- WebSocket ---
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
            connStatus.textContent = 'Connected';
            connStatus.className = 'status connected';
            send({ type: 'command', cmd: 'status' });
        };

        ws.onclose = function () {
            connStatus.textContent = 'Disconnected';
            connStatus.className = 'status disconnected';
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

    // --- Message handlers ---
    function handleMessage(msg) {
        switch (msg.type) {
            case 'char_sent':
                if (msg.char && msg.char !== ' ') {
                    sentChars += msg.char;
                    if (sentChars.length > 40) sentChars = sentChars.slice(-40);
                } else {
                    sentChars += ' ';
                }
                sentCharsEl.textContent = sentChars;
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
    }

    // --- Heatmap ---
    function updateHeatmap(data) {
        heatmapEl.innerHTML = '';
        if (!data) return;

        var maxProb = 1;
        for (var i = 0; i < data.length; i++) {
            if (data[i].prob > maxProb) maxProb = data[i].prob;
        }

        for (var j = 0; j < data.length; j++) {
            var item = data[j];
            var cell = document.createElement('div');
            cell.className = 'heatmap-cell';

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

    // --- Buttons ---
    startBtn.addEventListener('click', function () {
        var profile = parseInt(profileSelect.value, 10);
        var speed = parseInt(speedInput.value, 10);

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
    });

    // --- Init ---
    connect();
})();
