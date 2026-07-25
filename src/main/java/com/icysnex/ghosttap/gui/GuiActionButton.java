package com.icysnex.ghosttap.gui;

import net.minecraft.client.gui.FontRenderer;
import net.minecraft.client.gui.Gui;

// Full-width clickable button running a supplied action.
public class GuiActionButton extends Widget {

    private final String text;
    private final Runnable action;

    public GuiActionButton(String text, Runnable action) {
        super(null);
        this.text = text;
        this.action = action;
    }

    @Override
    public int height() {
        return 20;
    }

    @Override
    public void draw(FontRenderer fr, int mouseX, int mouseY) {
        boolean hover = contains(mouseX, mouseY);
        Gui.drawRect(x, y, x + width, y + 16, hover ? 0xFF3A3A3A : 0xFF262626);
        int color = hover ? 0xFFFFFFFF : 0xFFC8C8C8;
        fr.drawString(text, x + (width - fr.getStringWidth(text)) / 2, y + 4, color);
    }

    @Override
    public boolean mouseClicked(int mouseX, int mouseY) {
        if (!contains(mouseX, mouseY))
            return false;
        action.run();
        return true;
    }

    @Override
    public String tooltipAt(FontRenderer fr, int mouseX, int mouseY) {
        return tooltip != null && contains(mouseX, mouseY) ? tooltip : null;
    }

    boolean contains(int mouseX, int mouseY) {
        return mouseX >= x && mouseX <= x + width && mouseY >= y && mouseY <= y + 16;
    }
}
