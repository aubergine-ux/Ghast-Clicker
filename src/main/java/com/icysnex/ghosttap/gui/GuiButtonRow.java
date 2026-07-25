package com.icysnex.ghosttap.gui;

import net.minecraft.client.gui.FontRenderer;

// Several action buttons side by side, with an optional left label.
public class GuiButtonRow extends Widget {

    private static final int GAP = 4;
    private static final int LABEL_W = 44;

    private final GuiActionButton[] buttons;

    public GuiButtonRow(String label, GuiActionButton... buttons) {
        super(label);
        this.buttons = buttons;
    }

    @Override
    public int height() {
        return 20;
    }

    @Override
    public void draw(FontRenderer fr, int mouseX, int mouseY) {
        layout();
        if (label != null)
            fr.drawString(label, x, y + 3, 0xFFB8B8B8);
        for (GuiActionButton b : buttons)
            b.draw(fr, mouseX, mouseY);
    }

    @Override
    public boolean mouseClicked(int mouseX, int mouseY) {
        for (GuiActionButton b : buttons) {
            if (b.mouseClicked(mouseX, mouseY))
                return true;
        }
        return false;
    }

    @Override
    public String tooltipAt(FontRenderer fr, int mouseX, int mouseY) {
        for (GuiActionButton b : buttons) {
            if (b.tooltip != null && b.contains(mouseX, mouseY))
                return b.tooltip;
        }
        return null;
    }

    private void layout() {
        int startX = label != null ? x + LABEL_W : x;
        int each = (x + width - startX - GAP * (buttons.length - 1)) / buttons.length;
        for (int i = 0; i < buttons.length; i++) {
            GuiActionButton b = buttons[i];
            b.x = startX + i * (each + GAP);
            b.y = y;
            b.width = each;
        }
    }
}
