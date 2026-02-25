package morsetrainer;

import javax.swing.*;
import javax.swing.text.*;
import java.awt.*;
import java.awt.event.*;
import java.io.File;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

public class MorseClient extends JFrame implements SerialConnection.Listener {

    private static final Color GREEN = new Color(0, 140, 0);
    private static final Color RED = new Color(200, 30, 30);
    private static final Color AMBER = new Color(160, 120, 0);
    private static final Color FG = Color.BLACK;
    private static final Font MONO = new Font(Font.MONOSPACED, Font.PLAIN, 14);
    private static final Font MONO_BOLD = new Font(Font.MONOSPACED, Font.BOLD, 14);
    private static final Font STATS_FONT = new Font(Font.MONOSPACED, Font.BOLD, 13);

    private final JComboBox<String> portCombo;
    private final JButton connectBtn;
    private final JButton startBtn;
    private final JLabel statusDot;
    private final JLabel wpmLabel;
    private final JLabel correctLabel;
    private final JLabel wrongLabel;
    private final JLabel sentCharsLabel;
    private final JTextPane resultFeed;
    private final JLabel accuracyLabel;
    private final JLabel sessionTimeLabel;
    private final JLabel weakestLabel;

    private final SerialConnection serial;
    private final SessionLog log;
    private final ToneGenerator tone;
    private final StringBuilder sentChars = new StringBuilder();
    private int currentWpm;
    private boolean sessionActive;
    private boolean showAnswers = true;
    private Timer statsTimer;

    public MorseClient() {
        super("Morse Trainer Client");
        serial = new SerialConnection(this);
        log = new SessionLog();
        tone = new ToneGenerator();

        setDefaultCloseOperation(EXIT_ON_CLOSE);
        setSize(600, 500);
        setMinimumSize(new Dimension(500, 400));
        // --- Top bar: port selector + status ---
        JPanel topBar = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 4));
        topBar.setBorder(BorderFactory.createEmptyBorder(4, 8, 4, 8));

        portCombo = new JComboBox<>();
        portCombo.setFont(MONO);
        portCombo.setPreferredSize(new Dimension(200, 28));
        refreshPorts();

        JButton refreshBtn = new JButton("Refresh");
        refreshBtn.setFont(MONO);
        refreshBtn.setFocusable(false);
        refreshBtn.addActionListener(e -> refreshPorts());

        connectBtn = new JButton("Connect");
        connectBtn.setFont(MONO);
        connectBtn.setFocusable(false);
        connectBtn.addActionListener(e -> toggleConnection());

        statusDot = new JLabel("\u25CF");
        statusDot.setFont(new Font(Font.SANS_SERIF, Font.BOLD, 18));
        statusDot.setForeground(RED);

        startBtn = new JButton("Start");
        startBtn.setFont(MONO);
        startBtn.setFocusable(false);
        startBtn.setEnabled(false);
        startBtn.addActionListener(e -> {
            if (sessionActive) {
                serial.sendCommand("/stop");
            } else {
                serial.sendCommand("/start");
            }
            requestFocusInWindow();
        });

        topBar.add(portCombo);
        topBar.add(refreshBtn);
        topBar.add(connectBtn);
        topBar.add(statusDot);
        topBar.add(Box.createHorizontalStrut(12));
        topBar.add(startBtn);

        // --- Stats bar ---
        JPanel statsBar = new JPanel(new FlowLayout(FlowLayout.LEFT, 20, 2));
        statsBar.setBorder(BorderFactory.createEmptyBorder(2, 8, 2, 8));

        wpmLabel = makeStatLabel("-- WPM");
        correctLabel = makeStatLabel("Correct: 0");
        wrongLabel = makeStatLabel("Wrong: 0");

        JCheckBox showAnswersBox = new JCheckBox("Show Answers", true);
        showAnswersBox.setFont(STATS_FONT);
        showAnswersBox.setFocusable(false);
        showAnswersBox.addActionListener(e -> showAnswers = showAnswersBox.isSelected());

        statsBar.add(wpmLabel);
        statsBar.add(correctLabel);
        statsBar.add(wrongLabel);
        statsBar.add(Box.createHorizontalStrut(12));
        statsBar.add(showAnswersBox);

        // --- Tone controls ---
        JPanel toneBar = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 2));
        toneBar.setBorder(BorderFactory.createEmptyBorder(2, 8, 2, 8));

        JLabel pitchLabel = makeStatLabel("Pitch:");
        JLabel pitchValue = makeStatLabel(tone.getFrequency() + " Hz");
        JSlider pitchSlider = new JSlider(300, 2400, tone.getFrequency());
        pitchSlider.setFocusable(false);
        pitchSlider.setPreferredSize(new Dimension(120, 24));
        pitchSlider.addChangeListener(e -> {
            int hz = pitchSlider.getValue();
            tone.setFrequency(hz);
            pitchValue.setText(hz + " Hz");
        });

        JLabel spacingLabel = makeStatLabel("Spacing:");
        JLabel spacingValue = makeStatLabel(tone.getSpacing() + "%");
        JSlider spacingSlider = new JSlider(25, 300, tone.getSpacing());
        spacingSlider.setFocusable(false);
        spacingSlider.setPreferredSize(new Dimension(120, 24));
        spacingSlider.addChangeListener(e -> {
            int pct = spacingSlider.getValue();
            tone.setSpacing(pct);
            spacingValue.setText(pct + "%");
        });

        toneBar.add(pitchLabel);
        toneBar.add(pitchSlider);
        toneBar.add(pitchValue);
        toneBar.add(Box.createHorizontalStrut(12));
        toneBar.add(spacingLabel);
        toneBar.add(spacingSlider);
        toneBar.add(spacingValue);

        // --- Sent chars ---
        sentCharsLabel = new JLabel("Sent: ");
        sentCharsLabel.setFont(MONO_BOLD);
        sentCharsLabel.setBorder(BorderFactory.createEmptyBorder(4, 12, 4, 12));

        // Combine top panels
        JPanel headerPanel = new JPanel();
        headerPanel.setLayout(new BoxLayout(headerPanel, BoxLayout.Y_AXIS));
        headerPanel.add(topBar);
        headerPanel.add(statsBar);
        headerPanel.add(toneBar);
        headerPanel.add(sentCharsLabel);

        // --- Result feed (center) ---
        resultFeed = new JTextPane();
        resultFeed.setEditable(false);
        resultFeed.setFont(MONO);
        resultFeed.setFocusable(false);

        JScrollPane scrollPane = new JScrollPane(resultFeed);
        scrollPane.setBorder(BorderFactory.createEmptyBorder());

        // --- Bottom summary ---
        JPanel bottomPanel = new JPanel(new GridLayout(2, 1));
        bottomPanel.setBorder(BorderFactory.createEmptyBorder(4, 12, 4, 12));

        accuracyLabel = makeStatLabel("Accuracy: --    Session: 00:00");
        weakestLabel = makeStatLabel("Need work: --");

        bottomPanel.add(accuracyLabel);
        bottomPanel.add(weakestLabel);

        sessionTimeLabel = new JLabel(); // updated by timer, displayed in accuracyLabel

        // --- Layout ---
        setLayout(new BorderLayout());
        add(headerPanel, BorderLayout.NORTH);
        add(scrollPane, BorderLayout.CENTER);
        add(bottomPanel, BorderLayout.SOUTH);

        // --- Key listener on the frame ---
        addKeyListener(new KeyAdapter() {
            @Override
            public void keyTyped(KeyEvent e) {
                char c = e.getKeyChar();
                if (c == '\n' || c == '\r') return; // ignore enter for typing
                if (serial.isConnected()) {
                    serial.send(c);
                }
            }

            @Override
            public void keyPressed(KeyEvent e) {
                // Send enter for command mode (when typing /start etc via sent chars)
                if (e.getKeyCode() == KeyEvent.VK_ENTER && serial.isConnected()) {
                    serial.send('\n');
                }
            }
        });
        setFocusable(true);

        // Stats refresh timer
        statsTimer = new Timer(1000, e -> updateSummary());
        statsTimer.start();

        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                if (log.hasEntries()) {
                    String filename = "morse_session_" +
                            LocalDateTime.now().format(DateTimeFormatter.ofPattern("yyyyMMdd_HHmmss")) + ".csv";
                    try {
                        log.exportCSV(new File(filename));
                        appendResult("Session log saved: " + filename, AMBER);
                    } catch (Exception ex) {
                        // best effort
                    }
                }
                serial.disconnect();
                tone.shutdown();
            }
        });
    }

    private JLabel makeStatLabel(String text) {
        JLabel label = new JLabel(text);
        label.setFont(STATS_FONT);
        return label;
    }

    private void refreshPorts() {
        String selected = (String) portCombo.getSelectedItem();
        portCombo.removeAllItems();
        for (String name : SerialConnection.listPorts()) {
            portCombo.addItem(name);
        }
        if (selected != null) {
            portCombo.setSelectedItem(selected);
        }
    }

    private void toggleConnection() {
        if (serial.isConnected()) {
            if (sessionActive) {
                serial.sendCommand("/stop");
            }
            serial.disconnect();
        } else {
            String port = (String) portCombo.getSelectedItem();
            if (port == null) {
                appendResult("No port selected", RED);
                return;
            }
            if (!serial.connect(port)) {
                appendResult("Failed to open " + port, RED);
            }
        }
    }

    private void appendResult(String text, Color color) {
        SwingUtilities.invokeLater(() -> {
            StyledDocument doc = resultFeed.getStyledDocument();
            SimpleAttributeSet attrs = new SimpleAttributeSet();
            StyleConstants.setForeground(attrs, color);
            StyleConstants.setFontFamily(attrs, Font.MONOSPACED);
            StyleConstants.setFontSize(attrs, 14);
            try {
                doc.insertString(0, text + "\n", attrs);
            } catch (BadLocationException e) {
                // ignore
            }
            resultFeed.setCaretPosition(0);
        });
    }

    private void updateStatsBar() {
        SwingUtilities.invokeLater(() -> {
            wpmLabel.setText(currentWpm > 0 ? currentWpm + " WPM" : "-- WPM");
            correctLabel.setText("Correct: " + log.getCorrectCount());
            wrongLabel.setText("Wrong: " + log.getWrongCount());
        });
    }

    private void updateSummary() {
        SwingUtilities.invokeLater(() -> {
            String acc = log.getCorrectCount() + log.getWrongCount() > 0
                    ? String.format("%.0f%%", log.getAccuracy()) : "--";
            String time = sessionActive ? log.getElapsedTime() : "00:00";
            accuracyLabel.setText("Accuracy: " + acc + "    Session: " + time);
            weakestLabel.setText("Need work: " + log.getWeakest(5));
        });
    }

    // --- SerialConnection.Listener callbacks (called from reader thread) ---

    @Override
    public void onTx(char ch, String pattern, int dist) {
        // Play sidetone immediately — runs on its own thread
        if (currentWpm > 0 && !pattern.isEmpty()) {
            tone.play(pattern, currentWpm);
        }
        if (showAnswers) {
            SwingUtilities.invokeLater(() -> {
                sentChars.append(ch).append(' ');
                // Keep last ~30 chars visible
                String display = sentChars.toString();
                if (display.length() > 60) {
                    display = display.substring(display.length() - 60);
                }
                sentCharsLabel.setText("Sent: " + display);
            });
            appendResult("TX  " + ch + "  (" + pattern + ")  dist=" + dist, FG);
        }
    }

    @Override
    public void onResult(boolean correct, char typed, char expected, int prob) {
        log.record(expected, typed, correct, currentWpm, prob);
        if (correct) {
            appendResult("OK  " + typed, GREEN);
        } else {
            appendResult("ERR " + typed + " (expected " + expected + ")", RED);
        }
        updateStatsBar();
    }

    @Override
    public void onSpeed(int wpm, String direction) {
        currentWpm = wpm;
        if (!direction.isEmpty()) {
            String arrow = "up".equals(direction) ? "\u2191" : "\u2193";
            appendResult("Speed " + arrow + " " + wpm + " WPM", AMBER);
        }
        updateStatsBar();
    }

    @Override
    public void onSession(boolean running) {
        sessionActive = running;
        SwingUtilities.invokeLater(() -> {
            startBtn.setText(running ? "Stop" : "Start");
        });
        if (running) {
            log.startSession();
            sentChars.setLength(0);
            appendResult("--- Session started ---", AMBER);
        } else {
            appendResult("--- Session stopped ---", AMBER);
            if (log.hasEntries()) {
                appendResult(String.format("Accuracy: %.0f%%  |  Weakest: %s",
                        log.getAccuracy(), log.getWeakest(5)), AMBER);
            }
        }
        updateStatsBar();
        updateSummary();
    }

    @Override
    public void onContextLost() {
        appendResult("! Context lost - resynchronizing...", RED);
    }

    @Override
    public void onRawLine(String line) {
        // Could add a debug console; for now, just ignore unrecognized lines
    }

    @Override
    public void onConnectionChanged(boolean connected) {
        SwingUtilities.invokeLater(() -> {
            statusDot.setForeground(connected ? GREEN : RED);
            connectBtn.setText(connected ? "Disconnect" : "Connect");
            startBtn.setEnabled(connected);
            if (connected) {
                startBtn.setText("Start");
                appendResult("Connected to " + portCombo.getSelectedItem(), GREEN);
            } else {
                startBtn.setEnabled(false);
                startBtn.setText("Start");
                appendResult("Disconnected", RED);
            }
        });
    }

    // --- Main ---

    public static void main(String[] args) {
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception ignored) {}

        SwingUtilities.invokeLater(() -> {
            MorseClient client = new MorseClient();
            client.setLocationRelativeTo(null);
            client.setVisible(true);
            client.requestFocusInWindow();
        });
    }
}
