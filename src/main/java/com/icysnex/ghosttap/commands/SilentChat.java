package com.icysnex.ghosttap.commands;

import com.icysnex.ghosttap.config.ConfigHandler;
import net.minecraft.client.Minecraft;
import net.minecraft.util.ChatComponentText;
import org.lwjgl.input.Keyboard;

// Silent chat command intercepted before sending
// - ".ghosttap" opens the menu
// - ".ghosttap key <name>" rebinds menu
public final class SilentChat {

    private static final String PREFIX = ".ghosttap";
    private static volatile boolean openRequested = false;

    private SilentChat() {
    }

    public static boolean handle(String message) {
        String msg = message.trim();
        if (!msg.toLowerCase().startsWith(PREFIX))
            return false;

        String[] parts = msg.split("\\s+");
        if (parts.length >= 2 && parts[1].equalsIgnoreCase("key")) {
            if (parts.length != 3) {
                feedback("Invalid amount of arguments");
            }
            else {
                int code = Keyboard.getKeyIndex(parts[2].toUpperCase());
                if (code == Keyboard.KEY_NONE) {
                    feedback("Unknown key: " + parts[2]);
                } else {
                    ConfigHandler.openGuiKey = code;
                    ConfigHandler.saveConfig();
                    feedback("Menu key set to " + Keyboard.getKeyName(code) + ".");
                }
            }
        } else {
            openRequested = true;
        }
        return true;
    }

    public static boolean consumeOpenRequest() {
        if (!openRequested)
            return false;
        openRequested = false;
        return true;
    }

    private static void feedback(String text) {
        Minecraft mc = Minecraft.getMinecraft();
        if (mc.thePlayer != null)
            mc.thePlayer.addChatMessage(new ChatComponentText("[GhostTap] " + text));
    }
}
