package com.icysnex.ghosttap.gui;

import net.minecraft.client.gui.FontRenderer;

import java.util.function.Supplier;

// Read-only row: label left, live value right.
public class GuiStat extends Widget {

    private final Supplier<String> value;

    public GuiStat(String label, Supplier<String> value) {
        super(label);
        this.value = value;
    }

    @Override
    public int height() {
        return 13;
    }

    @Override
    protected int labelTop() {
        return y;
    }

    @Override
    public void draw(FontRenderer fr, int mouseX, int mouseY) {
        fr.drawString(label, x, y, 0xFFB8B8B8);
        String text = value.get();
        fr.drawString(text, x + width - fr.getStringWidth(text), y, 0xFFFFFFFF);
    }
}
