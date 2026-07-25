package com.icysnex.ghosttap.config;

import com.icysnex.ghosttap.core.ActivationMode;
import com.icysnex.ghosttap.core.Defaults;
import com.icysnex.ghosttap.core.HudAnchor;
import com.icysnex.ghosttap.core.analytics.Tracker;
import com.icysnex.ghosttap.core.click.Clicker;
import com.icysnex.ghosttap.core.click.ClickerGates;
import net.minecraftforge.common.config.Configuration;
import net.minecraftforge.common.config.Property;

import java.io.File;
import java.util.Map;
import java.util.function.DoubleConsumer;
import java.util.function.DoubleSupplier;

public class ConfigHandler {

    private static final String CAT_LEFT = "left_clicker";
    private static final String CAT_RIGHT = "right_clicker";
    private static final String CAT_KEYS = "keys";
    private static final String CAT_HUD = "hud";

    private static Configuration config;

    // Keybinds (LWJGL key codes) and activation modes.
    public static int openGuiKey = Defaults.KEY_OPEN;
    public static int toggleLeftKey = Defaults.KEY_LEFT;
    public static int toggleRightKey = Defaults.KEY_RIGHT;
    public static ActivationMode leftMode = Defaults.MODE_LEFT;
    public static ActivationMode rightMode = Defaults.MODE_RIGHT;

    // Hide the mod from the server-facing FML handshake mod list.
    public static boolean hideFromServers = Defaults.HIDE_FROM_SERVERS;

    // Animate the config menu's list scrolling.
    public static boolean smoothScroll = Defaults.SMOOTH_SCROLL;

    // On-screen HUD.
    public static boolean hudEnabled = Defaults.HUD_ENABLED;
    public static boolean hudHideInMenu = Defaults.HUD_HIDE_IN_MENU;
    public static boolean hudCpsLeft = Defaults.HUD_CPS_LEFT;
    public static boolean hudCpsRight = Defaults.HUD_CPS_RIGHT;
    public static boolean hudShowStatus = Defaults.HUD_STATUS;
    public static boolean hudBackground = Defaults.HUD_BACKGROUND;
    public static int hudPadding = Defaults.HUD_PADDING;
    public static int hudTextColor = Defaults.HUD_TEXT_COLOR;
    public static int hudBgColor = Defaults.HUD_BG_COLOR;
    public static HudAnchor hudAnchor = Defaults.HUD_ANCHOR;
    public static int hudMargin = Defaults.HUD_MARGIN;
    public static int hudX = Defaults.HUD_X;
    public static int hudY = Defaults.HUD_Y;


    public static void loadConfig(File file) {
        config = new Configuration(file);
        config.load();

        sync(false);
    }

    public static void resetHud() {
        hudEnabled = Defaults.HUD_ENABLED;
        hudHideInMenu = Defaults.HUD_HIDE_IN_MENU;
        hudCpsLeft = Defaults.HUD_CPS_LEFT;
        hudCpsRight = Defaults.HUD_CPS_RIGHT;
        hudShowStatus = Defaults.HUD_STATUS;
        hudBackground = Defaults.HUD_BACKGROUND;
        hudPadding = Defaults.HUD_PADDING;
        hudTextColor = Defaults.HUD_TEXT_COLOR;
        hudBgColor = Defaults.HUD_BG_COLOR;
        hudAnchor = Defaults.HUD_ANCHOR;
        hudMargin = Defaults.HUD_MARGIN;
        hudX = Defaults.HUD_X;
        hudY = Defaults.HUD_Y;
    }

    private static final String HUD_TOKEN = "ghosttap-hud";

    public static String exportHud() {
        Map<String, String> m = ConfigCodec.map();
        ConfigCodec.put(m, "enabled", hudEnabled);
        ConfigCodec.put(m, "hideInMenu", hudHideInMenu);
        ConfigCodec.put(m, "cpsLeft", hudCpsLeft);
        ConfigCodec.put(m, "cpsRight", hudCpsRight);
        ConfigCodec.put(m, "showStatus", hudShowStatus);
        ConfigCodec.put(m, "background", hudBackground);
        ConfigCodec.put(m, "padding", hudPadding);
        ConfigCodec.put(m, "textColor", hudTextColor);
        ConfigCodec.put(m, "bgColor", hudBgColor);
        ConfigCodec.put(m, "anchor", hudAnchor.name());
        ConfigCodec.put(m, "margin", hudMargin);
        ConfigCodec.put(m, "x", hudX);
        ConfigCodec.put(m, "y", hudY);
        return ConfigCodec.encode(HUD_TOKEN, m);
    }

    public static boolean importHud(String token) {
        Map<String, String> m = ConfigCodec.decode(HUD_TOKEN, token);
        if (m == null)
            return false;

        hudEnabled = ConfigCodec.flag(m, "enabled", hudEnabled);
        hudHideInMenu = ConfigCodec.flag(m, "hideInMenu", hudHideInMenu);
        hudCpsLeft = ConfigCodec.flag(m, "cpsLeft", hudCpsLeft);
        hudCpsRight = ConfigCodec.flag(m, "cpsRight", hudCpsRight);
        hudShowStatus = ConfigCodec.flag(m, "showStatus", hudShowStatus);
        hudBackground = ConfigCodec.flag(m, "background", hudBackground);
        hudPadding = ConfigCodec.integer(m, "padding", hudPadding);
        hudTextColor = ConfigCodec.integer(m, "textColor", hudTextColor);
        hudBgColor = ConfigCodec.integer(m, "bgColor", hudBgColor);
        hudMargin = ConfigCodec.integer(m, "margin", hudMargin);
        hudX = ConfigCodec.integer(m, "x", hudX);
        hudY = ConfigCodec.integer(m, "y", hudY);
        try {
            if (m.containsKey("anchor"))
                hudAnchor = HudAnchor.valueOf(m.get("anchor"));
        } catch (IllegalArgumentException ignored) {
        }
        
        return true;
    }

    public static void saveConfig() {
        if (config == null)
            return;

        sync(true);

        if (config.hasChanged())
            config.save();
    }


    // save=false loads config into the live fields; save=true writes them back.
    private static void sync(boolean save) {
        syncClicker(CAT_LEFT, Clicker.LEFT, save);
        syncClicker(CAT_RIGHT, Clicker.RIGHT, save);

        syncGates(CAT_LEFT, Clicker.LEFT.gates, save);
        syncGates(CAT_RIGHT, Clicker.RIGHT.gates, save);

        openGuiKey = key(CAT_KEYS, "openGui", openGuiKey, save);
        toggleLeftKey = key(CAT_KEYS, "toggleLeft", toggleLeftKey, save);
        toggleRightKey = key(CAT_KEYS, "toggleRight", toggleRightKey, save);

        leftMode = mode(CAT_KEYS, "leftMode", leftMode, save);
        rightMode = mode(CAT_KEYS, "rightMode", rightMode, save);

        hideFromServers = bool(CAT_KEYS, "hideFromServers", hideFromServers, save);
        smoothScroll = bool(CAT_KEYS, "smoothScroll", smoothScroll, save);

        Property analytics = config.get(CAT_KEYS, "analyticsEnabled", Tracker.enabled, "Record click analytics for tuning/export");
        if (save)
            analytics.set(Tracker.enabled);
        else
            Tracker.enabled = analytics.getBoolean();

        hudEnabled = bool(CAT_HUD, "enabled", hudEnabled, save);
        hudHideInMenu = bool(CAT_HUD, "hideInMenu", hudHideInMenu, save);
        hudCpsLeft = bool(CAT_HUD, "cpsLeft", hudCpsLeft, save);
        hudCpsRight = bool(CAT_HUD, "cpsRight", hudCpsRight, save);
        hudShowStatus = bool(CAT_HUD, "showStatus", hudShowStatus, save);
        hudBackground = bool(CAT_HUD, "background", hudBackground, save);
        hudPadding = integer(CAT_HUD, "padding", hudPadding, save);
        hudTextColor = integer(CAT_HUD, "textColor", hudTextColor, save);
        hudBgColor = integer(CAT_HUD, "bgColor", hudBgColor, save);

        Property anchor = config.get(CAT_HUD, "anchor", hudAnchor.name());
        if (save)
            anchor.set(hudAnchor.name());
        else
            try {
                hudAnchor = HudAnchor.valueOf(anchor.getString());
            } catch (IllegalArgumentException ignored) {
            }

        hudMargin = integer(CAT_HUD, "margin", hudMargin, save);
        hudX = integer(CAT_HUD, "x", hudX, save);
        hudY = integer(CAT_HUD, "y", hudY, save);
    }

    private static void syncClicker(String cat, Clicker c, boolean save) {
        bind(cat, "cpsMean", () -> c.cpsMean, v -> c.cpsMean = v, save);
        bind(cat, "cpsStandardDeviation", () -> c.cpsStandardDeviation, v -> c.cpsStandardDeviation = v, save);
        bind(cat, "cpsMin", () -> c.cpsMin, v -> c.cpsMin = v, save);
        bind(cat, "cpsMax", () -> c.cpsMax, v -> c.cpsMax = v, save);
        bind(cat, "cpsMinMaxFallout", () -> c.cpsMinMaxFallout, v -> c.cpsMinMaxFallout = v, save);

        bind(cat, "spikeChance", () -> c.spikeChance, v -> c.spikeChance = v, save);
        bind(cat, "spikeMin", () -> c.spikeMin, v -> c.spikeMin = v, save);
        bind(cat, "spikeMax", () -> c.spikeMax, v -> c.spikeMax = v, save);

        bind(cat, "stutterChance", () -> c.stutterChance, v -> c.stutterChance = v, save);
        bind(cat, "stutterMin", () -> c.stutterMin, v -> c.stutterMin = v, save);
        bind(cat, "stutterMax", () -> c.stutterMax, v -> c.stutterMax = v, save);

        bind(cat, "holdMsMean", () -> c.holdMsMean, v -> c.holdMsMean = v, save);
        bind(cat, "holdMsStandardDeviation", () -> c.holdMsStandardDeviation, v -> c.holdMsStandardDeviation = v, save);
        bind(cat, "holdMsMin", () -> c.holdMsMin, v -> c.holdMsMin = v, save);
        bind(cat, "holdMsMax", () -> c.holdMsMax, v -> c.holdMsMax = v, save);

        bind(cat, "holdMsHeavyChance", () -> c.holdMsHeavyChance, v -> c.holdMsHeavyChance = v, save);
        bind(cat, "holdMsHeavyMin", () -> c.holdMsHeavyMin, v -> c.holdMsHeavyMin = v, save);
        bind(cat, "holdMsHeavyMax", () -> c.holdMsHeavyMax, v -> c.holdMsHeavyMax = v, save);

        bind(cat, "rhythmVolatility", () -> c.rhythmVolatility, v -> c.rhythmVolatility = v, save);
        bind(cat, "rhythmTension", () -> c.rhythmTension, v -> c.rhythmTension = v, save);
    }

    private static void syncGates(String cat, ClickerGates g, boolean save) {
        g.allowBlockBreak = bool(cat, "allowBlockBreak", g.allowBlockBreak, save);
        g.allowInMenu = bool(cat, "allowInMenu", g.allowInMenu, save);
        g.pauseWhileUsingItem = bool(cat, "pauseWhileUsingItem", g.pauseWhileUsingItem, save);

        g.weapons = bool(cat, "catWeapons", g.weapons, save);
        g.tools = bool(cat, "catTools", g.tools, save);
        g.blocks = bool(cat, "catBlocks", g.blocks, save);
        g.other = bool(cat, "catOther", g.other, save);

        g.survival = bool(cat, "gmSurvival", g.survival, save);
        g.creative = bool(cat, "gmCreative", g.creative, save);
        g.adventure = bool(cat, "gmAdventure", g.adventure, save);

        g.entityOnly = bool(cat, "entityOnly", g.entityOnly, save);
        bind(cat, "reachMin", () -> g.reachMin, v -> g.reachMin = v, save);
        bind(cat, "reachMax", () -> g.reachMax, v -> g.reachMax = v, save);
        g.placeableOnly = bool(cat, "placeableOnly", g.placeableOnly, save);

        for (int i = 0; i < g.slots.length; i++)
            g.slots[i] = bool(cat, "slot" + (i + 1), g.slots[i], save);
    }

    private static boolean bool(String cat, String key, boolean current, boolean save) {
        Property p = config.get(cat, key, current);
        if (save) {
            p.set(current);
            return current;
        }
        return p.getBoolean();
    }

    private static int integer(String cat, String key, int current, boolean save) {
        Property p = config.get(cat, key, current);
        if (save) {
            p.set(current);
            return current;
        }
        return p.getInt();
    }

    private static void bind(String cat, String key, DoubleSupplier get, DoubleConsumer set, boolean save) {
        Property p = config.get(cat, key, get.getAsDouble());
        if (save)
            p.set(get.getAsDouble());
        else
            set.accept(p.getDouble());
    }

    private static int key(String cat, String name, int current, boolean save) {
        Property p = config.get(cat, name, current, "LWJGL key code");
        if (save) {
            p.set(current);
            return current;
        }
        return p.getInt();
    }

    private static ActivationMode mode(String cat, String name, ActivationMode current, boolean save) {
        Property p = config.get(cat, name, current.name(), "Activation mode: TOGGLE, HOLD or MOUSE");
        if (save) {
            p.set(current.name());
            return current;
        }
        
        try {
            return ActivationMode.valueOf(p.getString());
        } catch (IllegalArgumentException e) {
            return current;
        }
    }
}
