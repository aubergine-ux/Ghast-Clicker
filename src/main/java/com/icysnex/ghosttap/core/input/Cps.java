package com.icysnex.ghosttap.core.input;

import java.util.concurrent.ConcurrentLinkedQueue;

// Counts clicks per second as the game actually sees them (real + spoofed)
public final class Cps {

    private static final ConcurrentLinkedQueue<Long> LEFT = new ConcurrentLinkedQueue<>();
    private static final ConcurrentLinkedQueue<Long> RIGHT = new ConcurrentLinkedQueue<>();

    private Cps() {
    }

    public static void hit(byte button) {
        ConcurrentLinkedQueue<Long> window = button == InputMouse.BUTTON_LEFT ? LEFT : RIGHT;
        long now = System.currentTimeMillis();
        window.add(now);
        // Prune on write too, so the queue stays bounded even when nothing reads
        // it (e.g. the HUD is off) — otherwise it grows unbounded and leaks.
        prune(window, now);
    }

    public static int left() {
        return count(LEFT);
    }

    public static int right() {
        return count(RIGHT);
    }

    private static int count(ConcurrentLinkedQueue<Long> window) {
        long now = System.currentTimeMillis();
        prune(window, now);
        return window.size();
    }

    // Timestamps are FIFO, so the expired ones are at the head.
    private static void prune(ConcurrentLinkedQueue<Long> window, long now) {
        Long head;
        while ((head = window.peek()) != null && now - head > 1000)
            window.poll();
    }
}
