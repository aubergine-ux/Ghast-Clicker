package com.icysnex.ghosttap.gui;

import net.minecraft.client.gui.FontRenderer;

// Base for every config-screen row. The screen sets x/y/width, then draws and
// hit-tests generically without knowing the concrete type.
public abstract class Widget {

    public int x, y, width;
    public String tooltip;
    protected final String label;

    protected Widget(String label) {
        this.label = label;
    }

    public abstract int height();

    public abstract void draw(FontRenderer fr, int mouseX, int mouseY);

    public boolean mouseClicked(int mouseX, int mouseY) {
        return false;
    }

    // Absolute y of the label text, for tooltip hover. Overridden where the label
    // sits at the row top.
    protected int labelTop() {
        return y + 3;
    }

    // Tooltip for the current mouse position, or null. Default: hover the label.
    public String tooltipAt(FontRenderer fr, int mouseX, int mouseY) {
        if (tooltip == null || label == null)
            return null;
        int w = fr.getStringWidth(label);
        int ty = labelTop();
        return mouseX >= x && mouseX <= x + w && mouseY >= ty && mouseY <= ty + 9 ? tooltip : null;
    }
}
