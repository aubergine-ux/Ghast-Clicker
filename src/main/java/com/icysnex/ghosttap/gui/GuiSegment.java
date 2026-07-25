package com.icysnex.ghosttap.gui;

import net.minecraft.client.gui.FontRenderer;
import net.minecraft.client.gui.Gui;

import java.util.function.IntConsumer;
import java.util.function.IntSupplier;

// Segmented selector: label, then pills filling the rest of the width.
public class GuiSegment extends Widget {

    private static final int LABEL_W = 42;

    private final String[] options;
    private final IntSupplier getIndex;
    private final IntConsumer setIndex;

    public GuiSegment(String label, String[] options, IntSupplier getIndex, IntConsumer setIndex) {
        super(label);
        this.options = options;
        this.getIndex = getIndex;
        this.setIndex = setIndex;
    }

    @Override
    public int height() {
        return 18;
    }

    private int edge(int i) {
        int segX = x + LABEL_W;
        return segX + (x + width - segX) * i / options.length;
    }

    @Override
    public void draw(FontRenderer fr, int mouseX, int mouseY) {
        fr.drawString(label, x, y + 3, 0xFFB8B8B8);

        int active = getIndex.getAsInt();
        for (int i = 0; i < options.length; i++) {
            int px = edge(i);
            int pxEnd = edge(i + 1);
            boolean isActive = i == active;
            boolean hover = mouseX >= px && mouseX <= pxEnd && mouseY >= y && mouseY <= y + 14;

            int right = i == options.length - 1 ? pxEnd : pxEnd - 1;
            int bg = isActive ? 0xFF5A9BD4 : (hover ? 0xFF3A3A3A : 0xFF262626);
            Gui.drawRect(px, y, right, y + 14, bg);

            int color = isActive ? 0xFFFFFFFF : 0xFFC8C8C8;
            fr.drawString(options[i], px + (pxEnd - px - fr.getStringWidth(options[i])) / 2, y + 3, color);
        }
    }

    @Override
    public boolean mouseClicked(int mouseX, int mouseY) {
        if (mouseY < y || mouseY > y + 14 || mouseX < x + LABEL_W || mouseX > x + width)
            return false;

        for (int i = 0; i < options.length; i++) {
            if (mouseX >= edge(i) && mouseX < edge(i + 1)) {
                setIndex.accept(i);
                return true;
            }
        }
        return false;
    }
}
