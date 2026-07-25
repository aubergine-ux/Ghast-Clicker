package com.icysnex.ghosttap.hud;

import com.icysnex.ghosttap.config.ConfigHandler;
import com.icysnex.ghosttap.core.HudAnchor;
import com.icysnex.ghosttap.core.click.Clicker;
import com.icysnex.ghosttap.core.input.Cps;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.FontRenderer;
import net.minecraft.client.gui.Gui;
import net.minecraft.client.gui.ScaledResolution;
import net.minecraftforge.client.event.RenderGameOverlayEvent;
import net.minecraftforge.fml.common.eventhandler.SubscribeEvent;

import java.util.ArrayList;
import java.util.List;

// Configurable overlay:
// - A classic "CPS: L | R" counter
// - And/Or per-clicker status
public class ClickerHUD {

    private static final int GREEN = 0xFF55FF55;
    private static final int RED = 0xFFFF5555;
    private static final int GRAY = 0xFF888888;

    @SubscribeEvent
    public void onRenderGameOverlay(RenderGameOverlayEvent.Post event) {
        if (event.type != RenderGameOverlayEvent.ElementType.TEXT)
            return;
        if (!ConfigHandler.hudEnabled)
            return;

        Minecraft mc = Minecraft.getMinecraft();
        if (mc.thePlayer == null)
            return;

        // Hide while a screen (chat, inventory) is open or the F3 debug overlay is up.
        if (ConfigHandler.hudHideInMenu && mc.gameSettings.showDebugInfo)
            return;

        List<Line> lines = buildLines();
        if (lines.isEmpty())
            return;

        FontRenderer fr = mc.fontRendererObj;
        int lineHeight = fr.FONT_HEIGHT + 1;

        int w = 0;
        for (Line line : lines)
            w = Math.max(w, line.width(fr));
        int textW = w - 1;
        int textH = lines.size() * lineHeight - 2;

        ScaledResolution sr = new ScaledResolution(mc);
        int x = anchorX(textW, sr.getScaledWidth());
        int y = anchorY(textH, sr.getScaledHeight());

        if (ConfigHandler.hudBackground) {
            int pad = ConfigHandler.hudPadding;
            Gui.drawRect(x - pad, y - pad, x + textW + pad, y + textH + pad, ConfigHandler.hudBgColor);
        }

        int color = 0xFF000000 | (ConfigHandler.hudTextColor & 0xFFFFFF);
        for (int i = 0; i < lines.size(); i++)
            lines.get(i).draw(fr, x, y + i * lineHeight, color);
    }

    private List<Line> buildLines() {
        List<Line> lines = new ArrayList<>();

        String cps = cpsText();
        if (cps != null)
            lines.add(Line.plain(cps));

        if (ConfigHandler.hudShowStatus) {
            lines.add(Line.status("Left", Clicker.LEFT.isActive(ConfigHandler.leftMode), Clicker.LEFT.isEnabled(), ConfigHandler.leftMode.label));
            lines.add(Line.status("Right", Clicker.RIGHT.isActive(ConfigHandler.rightMode), Clicker.RIGHT.isEnabled(), ConfigHandler.rightMode.label));
        }

        return lines;
    }

    private String cpsText() {
        boolean left = ConfigHandler.hudCpsLeft;
        boolean right = ConfigHandler.hudCpsRight;

        if (left && right)
            return "CPS: " + Cps.left() + " | " + Cps.right();
        if (left)
            return "CPS: " + Cps.left();
        if (right)
            return "CPS: " + Cps.right();
        return null;
    }

    private int edgePad() {
        return ConfigHandler.hudMargin + (ConfigHandler.hudBackground ? ConfigHandler.hudPadding : 0);
    }

    private int anchorX(int textW, int screenW) {
        HudAnchor a = ConfigHandler.hudAnchor;
        if (a == HudAnchor.MANUAL)
            return ConfigHandler.hudX;
        if (a == HudAnchor.TOP_RIGHT || a == HudAnchor.BOTTOM_RIGHT)
            return screenW - textW - edgePad();
        return edgePad();
    }

    private int anchorY(int textH, int screenH) {
        HudAnchor a = ConfigHandler.hudAnchor;
        if (a == HudAnchor.MANUAL)
            return ConfigHandler.hudY;
        if (a == HudAnchor.BOTTOM_LEFT || a == HudAnchor.BOTTOM_RIGHT)
            return screenH - textH - edgePad();
        return edgePad();
    }

    // A HUD line: either plain text, or a status line. Status shows two colored
    // states: whether the clicker is on/armed, and whether it's clicking right now.
    private static final class Line {
        private final String plain;
        private final String name;
        private final boolean on;
        private final boolean clicking;
        private final String mode;

        private Line(String plain, String name, boolean on, boolean clicking, String mode) {
            this.plain = plain;
            this.name = name;
            this.on = on;
            this.clicking = clicking;
            this.mode = mode;
        }

        static Line plain(String text) {
            return new Line(text, null, false, false, null);
        }

        static Line status(String name, boolean on, boolean clicking, String mode) {
            return new Line(null, name, on, clicking, mode);
        }

        int width(FontRenderer fr) {
            if (plain != null)
                return fr.getStringWidth(plain);
            return fr.getStringWidth(name + ": ") + fr.getStringWidth(onText())
                    + fr.getStringWidth(" " + clickText()) + fr.getStringWidth(" (" + mode + ")");
        }

        void draw(FontRenderer fr, int x, int y, int color) {
            if (plain != null) {
                fr.drawStringWithShadow(plain, x, y, color);
                return;
            }
            String prefix = name + ": ";
            fr.drawStringWithShadow(prefix, x, y, color);
            int x2 = x + fr.getStringWidth(prefix);
            fr.drawStringWithShadow(onText(), x2, y, on ? GREEN : RED);
            int x3 = x2 + fr.getStringWidth(onText());
            fr.drawStringWithShadow(" " + clickText(), x3, y, clicking ? GREEN : GRAY);
            int x4 = x3 + fr.getStringWidth(" " + clickText());
            fr.drawStringWithShadow(" (" + mode + ")", x4, y, color);
        }

        private String onText() {
            return on ? "ON" : "OFF";
        }

        private String clickText() {
            return clicking ? "FIRE" : "IDLE";
        }
    }
}
