package morsetrainer;

import javax.sound.sampled.*;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * Generates CW sidetone through laptop speakers.
 * Plays morse patterns (e.g. ".-" for A) with proper timing based on WPM.
 * Continuous phase tracking and raised-cosine ramp for a clean, smooth tone.
 */
public class ToneGenerator {

    private static final int SAMPLE_RATE = 44100;
    private static final double RAMP_MS = 5.0; // raised-cosine ramp duration

    private final BlockingQueue<ToneRequest> queue = new LinkedBlockingQueue<>();
    private final Thread playerThread;
    private volatile boolean running = true;
    private volatile int freqHz = 700;
    private volatile int spacingPct = 100;
    private SourceDataLine line;
    private double phase; // continuous phase across all tones

    private static class ToneRequest {
        final String pattern;
        final int wpm;
        ToneRequest(String pattern, int wpm) {
            this.pattern = pattern;
            this.wpm = wpm;
        }
    }

    public ToneGenerator() {
        playerThread = new Thread(this::playLoop, "ToneGenerator");
        playerThread.setDaemon(true);
        playerThread.start();
    }

    public void setFrequency(int hz) {
        this.freqHz = hz;
    }

    public int getFrequency() {
        return freqHz;
    }

    public void setSpacing(int percent) {
        this.spacingPct = percent;
    }

    public int getSpacing() {
        return spacingPct;
    }

    public void play(String pattern, int wpm) {
        if (pattern == null || pattern.isEmpty() || wpm <= 0) return;
        queue.offer(new ToneRequest(pattern, wpm));
    }

    public void shutdown() {
        running = false;
        playerThread.interrupt();
        if (line != null) {
            line.close();
        }
    }

    private void playLoop() {
        try {
            AudioFormat format = new AudioFormat(SAMPLE_RATE, 16, 1, true, false);
            DataLine.Info info = new DataLine.Info(SourceDataLine.class, format);
            line = (SourceDataLine) AudioSystem.getLine(info);
            line.open(format, SAMPLE_RATE * 2 / 10); // ~100ms buffer
            line.start();

            while (running) {
                ToneRequest req = queue.take();
                playPattern(req.pattern, req.wpm);
            }
        } catch (InterruptedException e) {
            // shutdown
        } catch (LineUnavailableException e) {
            System.err.println("Audio unavailable: " + e.getMessage());
        }
    }

    private void playPattern(String pattern, int wpm) {
        int ditMs = 1200 / wpm;
        int freq = freqHz; // snapshot for this pattern

        for (int i = 0; i < pattern.length(); i++) {
            char element = pattern.charAt(i);
            int durationMs;
            if (element == '.') {
                durationMs = ditMs;
            } else if (element == '-') {
                durationMs = ditMs * 3;
            } else {
                continue;
            }

            writeTone(durationMs, freq);

            if (i < pattern.length() - 1) {
                int gapMs = ditMs * spacingPct / 100;
                writeSilence(Math.max(gapMs, 1));
            }
        }

        line.drain();
    }

    private void writeTone(int durationMs, int freq) {
        int numSamples = (int) (SAMPLE_RATE * durationMs / 1000.0);
        int rampSamples = (int) (SAMPLE_RATE * RAMP_MS / 1000.0);
        rampSamples = Math.min(rampSamples, numSamples / 2);
        byte[] buf = new byte[numSamples * 2];
        double phaseInc = 2.0 * Math.PI * freq / SAMPLE_RATE;

        for (int i = 0; i < numSamples; i++) {
            double sample = Math.sin(phase);
            phase += phaseInc;

            // Raised-cosine envelope for smooth attack/release
            double envelope = 1.0;
            if (i < rampSamples) {
                envelope = 0.5 * (1.0 - Math.cos(Math.PI * i / rampSamples));
            } else if (i >= numSamples - rampSamples) {
                envelope = 0.5 * (1.0 - Math.cos(Math.PI * (numSamples - i) / rampSamples));
            }

            short val = (short) (sample * envelope * Short.MAX_VALUE * 0.6);
            buf[i * 2] = (byte) (val & 0xFF);
            buf[i * 2 + 1] = (byte) ((val >> 8) & 0xFF);
        }

        // Keep phase in [0, 2π) to avoid floating point drift
        phase %= (2.0 * Math.PI);

        line.write(buf, 0, buf.length);
    }

    private void writeSilence(int durationMs) {
        int numSamples = (int) (SAMPLE_RATE * durationMs / 1000.0);
        byte[] buf = new byte[numSamples * 2];
        line.write(buf, 0, buf.length);
    }
}
