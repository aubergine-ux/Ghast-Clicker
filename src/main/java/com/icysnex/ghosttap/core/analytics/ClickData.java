package com.icysnex.ghosttap.core.analytics;

public class ClickData {
    public ClickData(long timestamp, double targetCps, double actualCps, long holdNanos, long intervalNanos, double trend) {
        this.timestamp = timestamp;
        this.targetCps = targetCps;
        this.actualCps = actualCps;
        this.holdNanos = holdNanos;
        this.intervalNanos = intervalNanos;
        this.trend = trend;
    }

    public long timestamp;

    public double targetCps;
    public double actualCps;

    public long holdNanos;
    public long intervalNanos;

    public double trend;
}
