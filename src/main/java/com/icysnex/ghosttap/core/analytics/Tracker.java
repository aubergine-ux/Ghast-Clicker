package com.icysnex.ghosttap.core.analytics;

import com.icysnex.ghosttap.core.Defaults;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.stream.Stream;

// One tracker per clicker; the enabled flag is a global master switch.
public class Tracker {

    public static boolean enabled = Defaults.ANALYTICS;


    private final ConcurrentLinkedQueue<ClickData> history = new ConcurrentLinkedQueue<>();
    private final ConcurrentLinkedQueue<Long> window = new ConcurrentLinkedQueue<>();

    public void record(double targetCps, long hold, long interval, double trend) {
        add(targetCps, hold, interval, trend);
    }

    // Physical click: no generated target/trend, and hold isn't known until the
    // button is released, so the mixin patches holdNanos on the returned row.
    public ClickData recordReal(long interval) {
        return add(0, 0, interval, 0);
    }

    private ClickData add(double targetCps, long hold, long interval, double trend) {
        if (!enabled)
            return null;

        long now = System.currentTimeMillis();
        window.add(now);

        if (history.size() >= 2000)
            history.poll();

        ClickData d = new ClickData(now, targetCps, getCurrentCps(), hold, interval, trend);
        history.add(d);
        return d;
    }


    public void clear() {
        history.clear();
        window.clear();
    }

    public int size() {
        return history.size();
    }

    public Stream<ClickData> stream() {
        return history.stream();
    }

    public List<ClickData> copy() {
        return new ArrayList<>(history);
    }


    public double getCurrentCps() {
        long now = System.currentTimeMillis();

        window.removeIf(timestamp -> now - timestamp > 1000);
        return window.size();
    }

    public double getAverageCps() {
        return history.stream().mapToDouble(d -> d.actualCps).average().orElse(0.0);
    }
}
