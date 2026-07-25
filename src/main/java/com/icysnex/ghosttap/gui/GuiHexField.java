package com.icysnex.ghosttap.gui;

import net.minecraft.client.gui.FontRenderer;
import net.minecraft.client.gui.Gui;
import org.lwjgl.input.Keyboard;

import java.util.function.IntConsumer;
import java.util.function.IntSupplier;

// Editable "#RRGGBB" (or "#AARRGGBB") color field with a live swatch. Applies on
// every keystroke; re-syncs if the color changes elsewhere while unfocused.
public class GuiHexField extends Widget {

    private static final int BOX_W = 62;
    private static final int SWATCH_W = 14;

    private final boolean alpha;
    private final IntSupplier getter;
    private final IntConsumer setter;

    public boolean focused;
    private String text;
    private int lastExternal;

    public GuiHexField(String label, boolean alpha, IntSupplier getter, IntConsumer setter) {
        super(label);
        this.alpha = alpha;
        this.getter = getter;
        this.setter = setter;
        pull();
    }

    @Override
    public int height() {
        return 18;
    }

    private void pull() {
        lastExternal = getter.getAsInt();
        text = alpha ? String.format("%08X", lastExternal) : String.format("%06X", lastExternal & 0xFFFFFF);
    }

    @Override
    public void draw(FontRenderer fr, int mouseX, int mouseY) {
        if (!focused && getter.getAsInt() != lastExternal)
            pull();

        fr.drawString(label, x, y + 3, 0xFFB8B8B8);

        int boxX = x + width - SWATCH_W - 2 - BOX_W;
        Gui.drawRect(boxX, y, boxX + BOX_W, y + 14, focused ? 0xFF3A3A3A : 0xFF262626);
        boolean blink = focused && (System.currentTimeMillis() / 500) % 2 == 0;
        fr.drawString("#" + text + (blink ? "_" : ""), boxX + 3, y + 3, 0xFFE0E0E0);

        int swatchX = x + width - SWATCH_W;
        Gui.drawRect(swatchX, y, swatchX + SWATCH_W, y + 14, 0xFF000000 | (getter.getAsInt() & 0xFFFFFF));
    }

    @Override
    public boolean mouseClicked(int mouseX, int mouseY) {
        int boxX = x + width - SWATCH_W - 2 - BOX_W;
        focused = mouseX >= boxX && mouseX <= boxX + BOX_W && mouseY >= y && mouseY <= y + 14;
        return focused;
    }

    public void keyTyped(char typedChar, int keyCode) {
        if (keyCode == Keyboard.KEY_BACK) {
            if (!text.isEmpty())
                text = text.substring(0, text.length() - 1);
        } else if (isHex(typedChar) && text.length() < (alpha ? 8 : 6)) {
            text += Character.toUpperCase(typedChar);
        } else {
            return;
        }
        apply();
    }

    private void apply() {
        if (text.isEmpty())
            return;
        try {
            int color = (int) Long.parseLong(text, 16);
            if (!alpha)
                color = 0xFF000000 | (color & 0xFFFFFF);
            setter.accept(color);
            lastExternal = color;
        } catch (NumberFormatException ignored) {
        }
    }

    private static boolean isHex(char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }
}
