package com.icysnex.ghosttap.gui;

import net.minecraft.client.gui.FontRenderer;
import net.minecraft.client.gui.Gui;

// Nine numbered cells for the hotbar-slot whitelist.
public class GuiSlots extends Widget {

    private static final int LABEL_W = 40;

    private final boolean[] slots;

    public GuiSlots(String label, boolean[] slots) {
        super(label);
        this.slots = slots;
    }

    @Override
    public int height() {
        return 18;
    }

    @Override
    public void draw(FontRenderer fr, int mouseX, int mouseY) {
        fr.drawString(label, x, y + 3, 0xFFB8B8B8);

        int cellsX = x + LABEL_W;
        int cellW = (x + width - cellsX) / slots.length;

        for (int i = 0; i < slots.length; i++) {
            int cx = cellsX + i * cellW;
            boolean hover = mouseX >= cx && mouseX <= cx + cellW && mouseY >= y && mouseY <= y + 14;
            int bg = slots[i] ? 0xFF5A9BD4 : (hover ? 0xFF3A3A3A : 0xFF262626);
            Gui.drawRect(cx, y, cx + cellW - 1, y + 14, bg);

            String n = String.valueOf(i + 1);
            int color = slots[i] ? 0xFFFFFFFF : 0xFF888888;
            fr.drawString(n, cx + (cellW - fr.getStringWidth(n)) / 2, y + 3, color);
        }
    }

    @Override
    public boolean mouseClicked(int mouseX, int mouseY) {
        int cellsX = x + LABEL_W;
        int cellW = (x + width - cellsX) / slots.length;

        if (mouseY < y || mouseY > y + 14 || mouseX < cellsX || mouseX > cellsX + cellW * slots.length)
            return false;

        int i = (mouseX - cellsX) / cellW;
        if (i >= 0 && i < slots.length) {
            slots[i] = !slots[i];
            return true;
        }
        return false;
    }
}
