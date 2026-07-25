package com.icysnex.ghosttap.core.analytics;

import com.icysnex.ghosttap.core.click.Clicker;
import com.icysnex.ghosttap.utils.FileIO;
import com.icysnex.ghosttap.utils.Notice;

import java.util.List;

// Export/clear used by the config UI; one CSV per button.
public class Analytics {

    private static int totalSize() {
        return Clicker.LEFT.tracker.size() + Clicker.RIGHT.tracker.size();
    }

    public static void clear() {
        Clicker.LEFT.tracker.clear();
        Clicker.RIGHT.tracker.clear();
        Notice.show("Cleared analytics data.");
    }

    public static void export() {
        if (totalSize() <= 0) {
            Notice.show("No analytics data. Enable tracking and click first.");
            return;
        }

        Notice.show("Exporting analytics...");
        exportButton(Clicker.LEFT, "left");
        exportButton(Clicker.RIGHT, "right");
    }

    private static void exportButton(Clicker c, String label) {
        if (c.tracker.size() <= 0)
            return;

        String fileName = "analytics_" + label + "_" + System.currentTimeMillis() + ".csv";
        List<ClickData> snapshot = c.tracker.copy();

        FileIO.saveAsync(fileName, (writer) -> {
            writer.printf("# --- START METADATA (%s) ---%n", label.toUpperCase());
            writer.printf("# CPS: %.1f (SD: %.1f, Range %.1f-%.1f +-%.1f)%n", c.cpsMean, c.cpsStandardDeviation, c.cpsMin, c.cpsMax, c.cpsMinMaxFallout);
            writer.printf("# - Spike %.1f%% (%.1f-%.1f) %n", c.spikeChance * 100, c.spikeMin, c.spikeMax);
            writer.printf("# - Stutter %.1f%% (%.1f-%.1f) %n", c.stutterChance * 100, c.stutterMin, c.stutterMax);
            writer.printf("# Hold: %.1fms (SD: %.1f, Range %.1f-%.1f)%n", c.holdMsMean, c.holdMsStandardDeviation, c.holdMsMin, c.holdMsMax);
            writer.printf("# - Heavy: %.1f%% (%.1f-%.1f)%n", c.holdMsHeavyChance, c.holdMsHeavyMin, c.holdMsHeavyMax);
            writer.printf("# Rythm: %.1f%% %.1f%n", c.rhythmVolatility, c.rhythmTension);
            writer.println("# --- END METADATA ---");

            writer.println("Timestamp,TargetCPS,ActualCPS,HoldMS,IntervalMS,Trend");
            for (ClickData d : snapshot) {
                writer.printf("%d,%.2f,%.2f,%.2f,%.2f,%.2f%n", d.timestamp, d.targetCps, d.actualCps, d.holdNanos / 1_000_000.0, d.intervalNanos / 1_000_000.0, d.trend);
            }
        }, (success) -> {
            if (success) {
                Notice.show("Exported " + label + ": " + fileName);
            } else {
                Notice.show("Export failed for " + label + " (see console).");
            }
        });
    }
}
