package com.icysnex.ghosttap.events;

import com.icysnex.ghosttap.commands.SilentChat;
import com.icysnex.ghosttap.config.ConfigHandler;
import com.icysnex.ghosttap.core.ActivationMode;
import com.icysnex.ghosttap.core.click.Clicker;
import com.icysnex.ghosttap.core.click.Gates;
import com.icysnex.ghosttap.core.input.InputMouse;
import com.icysnex.ghosttap.gui.GuiGhostTap;
import net.minecraft.client.Minecraft;
import net.minecraftforge.fml.common.eventhandler.SubscribeEvent;
import net.minecraftforge.fml.common.gameevent.TickEvent;
import org.lwjgl.input.Keyboard;
import org.lwjgl.input.Mouse;

// Polled every client tick rather than driven by key events
public class GhostTapKeybindListener {

    private final ButtonState left = new ButtonState();
    private final ButtonState right = new ButtonState();
    private boolean wasOpenDown = false;

    
    @SubscribeEvent
    public void onClientTick(TickEvent.ClientTickEvent event) {
        if (event.phase != TickEvent.Phase.END)
            return;

        Minecraft mc = Minecraft.getMinecraft();
        boolean screenOpen = mc.currentScreen != null;
        boolean ownGui = mc.currentScreen instanceof GuiGhostTap;

        boolean openDown = !screenOpen && isDown(ConfigHandler.openGuiKey);
        if ((openDown && !wasOpenDown) || (!screenOpen && SilentChat.consumeOpenRequest()))
            mc.displayGuiScreen(new GuiGhostTap());
        wasOpenDown = openDown;

        process(Clicker.LEFT, ConfigHandler.toggleLeftKey, ConfigHandler.leftMode, InputMouse.BUTTON_LEFT, left, screenOpen, ownGui);
        process(Clicker.RIGHT, ConfigHandler.toggleRightKey, ConfigHandler.rightMode, InputMouse.BUTTON_RIGHT, right, screenOpen, ownGui);
    }

    private void process(Clicker clicker, int key, ActivationMode mode, byte button, ButtonState st, boolean screenOpen, boolean ownGui) {
        boolean context = !ownGui && (!screenOpen || clicker.gates.allowInMenu);
        boolean keyDown = context && isDown(key);

        boolean intent = false;
        switch (mode) {
            case TOGGLE:
                if (keyDown && !st.wasKeyDown)
                    clicker.toggledOn = !clicker.toggledOn;
                intent = clicker.toggledOn;
                break;

            case HOLD:
                clicker.held = isDown(key);
                intent = keyDown;
                break;

            case MOUSE:
                if (keyDown && !st.wasKeyDown)
                    clicker.armed = !clicker.armed;
                boolean realDown = context && InputMouse.real(button) == InputMouse.STATE_DOWN;
                intent = clicker.armed && realDown;
                break;
        }
        st.wasKeyDown = keyDown;

        if (intent && !st.wasIntent)
            st.intentSince = System.currentTimeMillis();
        st.wasIntent = intent;
        boolean delayMet = mode != ActivationMode.MOUSE
                || System.currentTimeMillis() - st.intentSince >= clicker.startDelayMs;

        boolean engaged = intent && delayMet && context;
        boolean enabled = engaged && Gates.pass(clicker.gates, button);
        clicker.setEnabled(enabled);

        boolean mask = mode == ActivationMode.MOUSE
                ? engaged && !Gates.blockBreakHold(clicker.gates, button)
                : enabled;
        InputMouse.setMask(button, mask);
    }

    private static boolean isDown(int code) {
        if (code < 0)
            return Mouse.isButtonDown(code + 100);
        return Keyboard.isKeyDown(code);
    }

    private static class ButtonState {
        boolean wasKeyDown;
        boolean wasIntent;
        long intentSince;
    }
}
