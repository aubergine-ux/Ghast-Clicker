package com.icysnex.ghosttap.gui;

import net.minecraft.client.gui.FontRenderer;
import net.minecraft.client.gui.Gui;

import java.util.function.DoubleConsumer;
import java.util.function.DoubleSupplier;

// Reads/writes its value live through the getter/setter. Any cross-value rules
// (e.g. Min pushing Mean) live in the setter, not here.
public class GuiSlider extends Widget {

    private final double absMin;
    private final double absMax;
    private final int decimals;
    private final boolean percent;
    private final DoubleSupplier getter;
    private final DoubleConsumer setter;

    public boolean dragging;

    public GuiSlider(String label, double min, double max, int decimals, boolean percent, DoubleSupplier getter, DoubleConsumer setter) {
        super(label);
        this.absMin = min;
        this.absMax = max;
        this.decimals = decimals;
        this.percent = percent;
        this.getter = getter;
        this.setter = setter;
    }

    @Override
    public int height() {
        return 24;
    }

    @Override
    protected int labelTop() {
        return y;
    }

    @Override
    public void draw(FontRenderer fr, int mouseX, int mouseY) {
        double value = clamp(getter.getAsDouble(), absMin, absMax);

        int trackTop = y + 13;
        int trackBottom = y + 17;

        fr.drawString(label, x, y, 0xFFB8B8B8);
        String text = format(value);
        fr.drawString(text, x + width - fr.getStringWidth(text), y, 0xFFFFFFFF);

        Gui.drawRect(x, trackTop, x + width, trackBottom, 0xFF2A2A2A);

        double t = (value - absMin) / (absMax - absMin);
        int knobX = x + (int) Math.round(clamp(t, 0, 1) * width);
        Gui.drawRect(x, trackTop, knobX, trackBottom, 0xFF5A9BD4);

        boolean hover = dragging || contains(mouseX, mouseY);
        Gui.drawRect(knobX - 2, y + 10, knobX + 3, y + 20, hover ? 0xFFFFFFFF : 0xFFD0D0D0);
    }

    @Override
    public boolean mouseClicked(int mouseX, int mouseY) {
        if (!contains(mouseX, mouseY))
            return false;
        dragging = true;
        updateFromMouse(mouseX);
        return true;
    }

    public void mouseDragged(int mouseX) {
        if (dragging)
            updateFromMouse(mouseX);
    }

    public void mouseReleased() {
        dragging = false;
    }

    public boolean hovers(int mouseX, int mouseY) {
        return contains(mouseX, mouseY);
    }

    // One smallest-representable step, for precise scroll-wheel adjustment.
    public void nudge(int direction) {
        double step = 1.0 / Math.pow(10, percent ? decimals + 2 : decimals);
        double v = clamp(getter.getAsDouble(), absMin, absMax);
        setter.accept(clamp(round(v + direction * step), absMin, absMax));
    }

    private void updateFromMouse(int mouseX) {
        double t = (mouseX - x) / (double) width;
        setter.accept(clamp(round(absMin + t * (absMax - absMin)), absMin, absMax));
    }

    private boolean contains(int mouseX, int mouseY) {
        return mouseX >= x && mouseX <= x + width && mouseY >= y && mouseY <= y + 22;
    }

    private double clamp(double v, double lo, double hi) {
        return Math.max(lo, Math.min(hi, v));
    }

    private double round(double v) {
        double factor = Math.pow(10, percent ? decimals + 2 : decimals);
        return Math.round(v * factor) / factor;
    }

    private String format(double value) {
        if (percent)
            return String.format("%." + decimals + "f%%", value * 100);
        return String.format("%." + decimals + "f", value);
    }
}
