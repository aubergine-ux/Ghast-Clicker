package com.icysnex.ghosttap.gui;

import net.minecraft.client.gui.FontRenderer;
import net.minecraft.client.gui.Gui;

import java.util.function.BooleanSupplier;
import java.util.function.Consumer;

// Label + ON/OFF pill. Reads live state so external changes stay in sync.
public class GuiToggle extends Widget {

    private final BooleanSupplier getter;
    private final Consumer<Boolean> onChange;

    public GuiToggle(String label, BooleanSupplier getter, Consumer<Boolean> onChange) {
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
        boolean on = getter.getAsBoolean();
        fr.drawString(label, x, y + 3, 0xFFB8B8B8);

        String text = on ? "ON" : "OFF";
        int pillW = 34;
        int pillX = x + width - pillW;
        int color = on ? 0xFF3FA34D : 0xFF7A2E2E;
        if (contains(mouseX, mouseY))
            color |= 0x00202020;

        Gui.drawRect(pillX, y, pillX + pillW, y + 14, color);
        fr.drawString(text, pillX + (pillW - fr.getStringWidth(text)) / 2, y + 3, 0xFFFFFFFF);
    }

    @Override
    public boolean mouseClicked(int mouseX, int mouseY) {
        if (!contains(mouseX, mouseY))
            return false;
        onChange.accept(!getter.getAsBoolean());
        return true;
    }

    private boolean contains(int mouseX, int mouseY) {
        int pillX = x + width - 34;
        return mouseX >= pillX && mouseX <= pillX + 34 && mouseY >= y && mouseY <= y + 14;
    }
}
