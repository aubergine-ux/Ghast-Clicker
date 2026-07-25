package com.icysnex.ghosttap.gui;

import net.minecraft.client.gui.FontRenderer;
import net.minecraft.client.gui.Gui;
import org.lwjgl.input.Keyboard;

import java.util.function.IntConsumer;
import java.util.function.IntSupplier;

// Click to arm, then the next key or mouse button is captured by the screen.
public class GuiKeybind extends Widget {

    private static final int BOX_W = 70;

    private final IntSupplier getter;
    private final IntConsumer onChange;

    public boolean listening;

    public GuiKeybind(String label, IntSupplier getter, IntConsumer onChange) {
        super(label);
        this.getter = getter;
        this.onChange = onChange;
    }

    @Override
    public int height() {
        return 18;
    }

    @Override
    public void draw(FontRenderer fr, int mouseX, int mouseY) {
        fr.drawString(label, x, y + 3, 0xFFB8B8B8);

        int boxX = x + width - BOX_W;
        int color = listening ? 0xFF5A9BD4 : (contains(mouseX, mouseY) ? 0xFF3A3A3A : 0xFF2A2A2A);
        Gui.drawRect(boxX, y, boxX + BOX_W, y + 14, color);

        String text = listening ? "> press <" : name(getter.getAsInt());
        fr.drawString(text, boxX + (BOX_W - fr.getStringWidth(text)) / 2, y + 3, 0xFFFFFFFF);
    }

    @Override
    public boolean mouseClicked(int mouseX, int mouseY) {
        if (!contains(mouseX, mouseY))
            return false;
        listening = true;
        return true;
    }

    // Keyboard capture (Escape cancels).
    public void keyPressed(int keyCode) {
        if (keyCode != Keyboard.KEY_ESCAPE)
            onChange.accept(keyCode);
        listening = false;
    }

    // Mouse button capture, stored as (button - 100) to share the keyboard space.
    public void mousePressed(int button) {
        onChange.accept(button - 100);
        listening = false;
    }

    private boolean contains(int mouseX, int mouseY) {
        int boxX = x + width - BOX_W;
        return mouseX >= boxX && mouseX <= boxX + BOX_W && mouseY >= y && mouseY <= y + 14;
    }

    // Name for a keyboard code, or a mouse button (negative code).
    public static String name(int code) {
        if (code < 0) {
            int button = code + 100;
            switch (button) {
                case 0: return "LMB";
                case 1: return "RMB";
                case 2: return "MMB";
                default: return "M" + (button + 1);
            }
        }
        String n = Keyboard.getKeyName(code);
        return n == null ? "NONE" : n;
    }
}
