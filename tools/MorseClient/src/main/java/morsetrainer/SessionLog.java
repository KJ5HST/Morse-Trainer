package morsetrainer;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SessionLog {

    private static final DateTimeFormatter TIMESTAMP_FMT =
            DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");

    private final Map<Character, int[]> charStats = new HashMap<>(); // [correct, wrong]
    private final List<Entry> entries = new ArrayList<>();
    private long sessionStartMs;

    private static class Entry {
        final LocalDateTime timestamp;
        final char expected;
        final char typed;
        final boolean correct;
        final int speed;
        final int prob;

        Entry(char expected, char typed, boolean correct, int speed, int prob) {
            this.timestamp = LocalDateTime.now();
            this.expected = expected;
            this.typed = typed;
            this.correct = correct;
            this.speed = speed;
            this.prob = prob;
        }
    }

    public void startSession() {
        charStats.clear();
        entries.clear();
        sessionStartMs = System.currentTimeMillis();
    }

    public void record(char expected, char typed, boolean correct, int speed, int prob) {
        entries.add(new Entry(expected, typed, correct, speed, prob));
        int[] stats = charStats.computeIfAbsent(Character.toUpperCase(expected), k -> new int[2]);
        if (correct) {
            stats[0]++;
        } else {
            stats[1]++;
        }
    }

    public int getCorrectCount() {
        int total = 0;
        for (int[] s : charStats.values()) total += s[0];
        return total;
    }

    public int getWrongCount() {
        int total = 0;
        for (int[] s : charStats.values()) total += s[1];
        return total;
    }

    public double getAccuracy() {
        int correct = getCorrectCount();
        int total = correct + getWrongCount();
        if (total == 0) return 0;
        return (correct * 100.0) / total;
    }

    /** Returns top N characters by error count, formatted as "R x3, S x2". */
    public String getWeakest(int n) {
        List<Map.Entry<Character, int[]>> sorted = new ArrayList<>(charStats.entrySet());
        sorted.sort(Comparator.comparingInt((Map.Entry<Character, int[]> e) -> e.getValue()[1]).reversed());

        StringBuilder sb = new StringBuilder();
        int count = 0;
        for (Map.Entry<Character, int[]> e : sorted) {
            if (e.getValue()[1] == 0) continue;
            if (count > 0) sb.append(", ");
            sb.append(e.getKey()).append(" \u00d7").append(e.getValue()[1]);
            if (++count >= n) break;
        }
        return sb.length() == 0 ? "None" : sb.toString();
    }

    /** Session elapsed time formatted as MM:SS. */
    public String getElapsedTime() {
        long elapsed = System.currentTimeMillis() - sessionStartMs;
        long secs = elapsed / 1000;
        return String.format("%02d:%02d", secs / 60, secs % 60);
    }

    public void exportCSV(File file) throws IOException {
        try (PrintWriter pw = new PrintWriter(new FileWriter(file))) {
            pw.println("timestamp,expected,typed,correct,speed,prob");
            for (Entry e : entries) {
                pw.printf("%s,%c,%c,%s,%d,%d%n",
                        e.timestamp.format(TIMESTAMP_FMT),
                        e.expected, e.typed, e.correct, e.speed, e.prob);
            }
        }
    }

    public boolean hasEntries() {
        return !entries.isEmpty();
    }
}
