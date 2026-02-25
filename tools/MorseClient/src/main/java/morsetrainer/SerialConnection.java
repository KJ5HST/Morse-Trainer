package morsetrainer;

import com.fazecast.jSerialComm.SerialPort;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

public class SerialConnection {

    public interface Listener {
        void onTx(char ch, String pattern, int dist);
        void onResult(boolean correct, char typed, char expected, int prob);
        void onSpeed(int wpm, String direction);
        void onSession(boolean running);
        void onContextLost();
        void onRawLine(String line);
        void onConnectionChanged(boolean connected);
    }

    private SerialPort port;
    private Thread readerThread;
    private volatile boolean running;
    private Listener listener;
    private OutputStream outputStream;

    public SerialConnection(Listener listener) {
        this.listener = listener;
    }

    public static String[] listPorts() {
        SerialPort[] ports = SerialPort.getCommPorts();
        String[] names = new String[ports.length];
        for (int i = 0; i < ports.length; i++) {
            names[i] = ports[i].getSystemPortName();
        }
        return names;
    }

    public boolean connect(String portName) {
        disconnect();
        SerialPort[] ports = SerialPort.getCommPorts();
        for (SerialPort sp : ports) {
            if (sp.getSystemPortName().equals(portName)) {
                port = sp;
                break;
            }
        }
        if (port == null) return false;

        port.setBaudRate(115200);
        port.setNumDataBits(8);
        port.setNumStopBits(1);
        port.setParity(SerialPort.NO_PARITY);
        port.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, 0, 0);

        if (!port.openPort()) {
            port = null;
            return false;
        }

        outputStream = port.getOutputStream();
        running = true;
        readerThread = new Thread(this::readLoop, "SerialReader");
        readerThread.setDaemon(true);
        readerThread.start();
        listener.onConnectionChanged(true);

        // Query current state so UI reflects whether trainer is already running
        sendCommand("/status");
        return true;
    }

    public void disconnect() {
        running = false;
        if (readerThread != null) {
            readerThread.interrupt();
            readerThread = null;
        }
        if (port != null && port.isOpen()) {
            port.closePort();
        }
        port = null;
        outputStream = null;
        listener.onConnectionChanged(false);
    }

    public boolean isConnected() {
        return port != null && port.isOpen();
    }

    public void send(char c) {
        if (outputStream == null) return;
        try {
            outputStream.write(c);
            outputStream.flush();
        } catch (Exception e) {
            disconnect();
        }
    }

    public void sendCommand(String cmd) {
        if (outputStream == null) return;
        try {
            outputStream.write((cmd + "\n").getBytes(StandardCharsets.US_ASCII));
            outputStream.flush();
        } catch (Exception e) {
            disconnect();
        }
    }

    private void readLoop() {
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(port.getInputStream(), StandardCharsets.US_ASCII))) {
            while (running && !Thread.currentThread().isInterrupted()) {
                String line = reader.readLine();
                if (line == null) break;
                line = line.trim();
                if (line.isEmpty()) continue;
                listener.onRawLine(line);
                parseLine(line);
            }
        } catch (Exception e) {
            if (running) {
                disconnect();
            }
        }
    }

    private void parseLine(String line) {
        try {
            if (line.startsWith("[TX] ")) {
                // [TX] A (.-) dist=5
                String rest = line.substring(5);
                char ch = rest.charAt(0);
                String pattern = "";
                int dist = 0;
                int parenOpen = rest.indexOf('(');
                int parenClose = rest.indexOf(')');
                if (parenOpen >= 0 && parenClose > parenOpen) {
                    pattern = rest.substring(parenOpen + 1, parenClose);
                }
                int distIdx = rest.indexOf("dist=");
                if (distIdx >= 0) {
                    dist = Integer.parseInt(rest.substring(distIdx + 5).trim());
                }
                listener.onTx(ch, pattern, dist);

            } else if (line.startsWith("[OK] ")) {
                // [OK] A prob=95
                String rest = line.substring(5);
                char ch = rest.charAt(0);
                int prob = 0;
                int probIdx = rest.indexOf("prob=");
                if (probIdx >= 0) {
                    prob = Integer.parseInt(rest.substring(probIdx + 5).trim());
                }
                listener.onResult(true, ch, ch, prob);

            } else if (line.startsWith("[ERR] ")) {
                // [ERR] typed=B expected=A prob=95
                String rest = line.substring(6);
                char typed = ' ';
                char expected = ' ';
                int prob = 0;
                for (String part : rest.split("\\s+")) {
                    if (part.startsWith("typed=")) {
                        typed = part.charAt(6);
                    } else if (part.startsWith("expected=")) {
                        expected = part.charAt(9);
                    } else if (part.startsWith("prob=")) {
                        prob = Integer.parseInt(part.substring(5));
                    }
                }
                listener.onResult(false, typed, expected, prob);

            } else if (line.startsWith("[SPEED] ")) {
                // [SPEED] 45 WPM (up|down)
                String rest = line.substring(8);
                String[] parts = rest.split("\\s+");
                int wpm = Integer.parseInt(parts[0]);
                String direction = "";
                int parenOpen = rest.indexOf('(');
                int parenClose = rest.indexOf(')');
                if (parenOpen >= 0 && parenClose > parenOpen) {
                    direction = rest.substring(parenOpen + 1, parenClose);
                }
                listener.onSpeed(wpm, direction);

            } else if (line.startsWith("[SESSION] ")) {
                String state = line.substring(10).trim();
                listener.onSession(state.equals("started"));

            } else if (line.startsWith("[CONTEXT LOST]")) {
                listener.onContextLost();

            } else if (line.startsWith("Running: ")) {
                // /status response: "Running: yes" or "Running: no"
                boolean isRunning = line.substring(9).trim().equals("yes");
                listener.onSession(isRunning);

            } else if (line.startsWith("Speed: ")) {
                // /status response: "Speed: 25 WPM"
                String rest = line.substring(7).trim();
                int spaceIdx = rest.indexOf(' ');
                if (spaceIdx > 0) {
                    int wpm = Integer.parseInt(rest.substring(0, spaceIdx));
                    listener.onSpeed(wpm, "");
                }
            }
        } catch (Exception e) {
            // Malformed line — ignore
        }
    }
}
