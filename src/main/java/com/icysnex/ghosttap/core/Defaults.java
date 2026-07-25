package com.icysnex.ghosttap.core;

import org.lwjgl.input.Keyboard;

// Every default value in one place.
public final class Defaults {

    private Defaults() {
    }

    public static final class Profile {
        public double cpsMean = 13.0;
        public double cpsStandardDeviation = 1.5;
        public double cpsMin = 8.0;
        public double cpsMax = 18.0;
        public double cpsMinMaxFallout = 0.8;

        public double spikeChance = 0.04;
        public double spikeMin = 1;
        public double spikeMax = 3;

        public double stutterChance = 0.03;
        public double stutterMin = 4;
        public double stutterMax = 7;

        public double holdMsMean = 38;
        public double holdMsStandardDeviation = 6.5;
        public double holdMsMin = 18;
        public double holdMsMax = 75;

        public double holdMsHeavyChance = 0.015;
        public double holdMsHeavyMin = 15;
        public double holdMsHeavyMax = 35;

        public double rhythmVolatility = 0.5;
        public double rhythmTension = 0.04;

        public double startDelayMs = 0;

        public boolean slot = true;

        public boolean weapons = true;
        public boolean tools = true;
        public boolean blocks = true;
        public boolean other = true;

        public boolean allowBlockBreak = true;
        public boolean allowInMenu = false;
        public boolean pauseWhileUsingItem = true;
        public boolean entityOnly = false;
        public double reachMin = 3.0;
        public double reachMax = 3.5;
        public boolean placeableOnly = false;

        public boolean survival = true;
        public boolean creative = true;
        public boolean adventure = true;

    }


    // Per-clicker defaults: tuning params and gate options.
    public static final Profile LEFT = new Profile();
    public static final Profile RIGHT = new Profile();

    static {
        RIGHT.cpsMean = 15.0;
        RIGHT.cpsMin = 10.0;
        RIGHT.cpsMax = 20.0;
        
        RIGHT.weapons = false;
        RIGHT.tools = true;
        RIGHT.other = false;
        
        RIGHT.allowBlockBreak = false;
    }

    // Keys and modes (per button).
    public static final int KEY_OPEN = Keyboard.KEY_RSHIFT;
    public static final int KEY_LEFT = Keyboard.KEY_N;
    public static final int KEY_RIGHT = Keyboard.KEY_M;
    public static final ActivationMode MODE_LEFT = ActivationMode.MOUSE;
    public static final ActivationMode MODE_RIGHT = ActivationMode.MOUSE;

    // Analytics
    public static final boolean ANALYTICS = false;

    // Strip the mod from the mod list sent to servers during the FML handshake.
    public static final boolean HIDE_FROM_SERVERS = true;

    // Animate the config menu's list scrolling.
    public static final boolean SMOOTH_SCROLL = true;

    // HUD (global).
    public static final boolean HUD_ENABLED = true;
    public static final boolean HUD_HIDE_IN_MENU = true;
    public static final boolean HUD_CPS_LEFT = true;
    public static final boolean HUD_CPS_RIGHT = true;
    public static final boolean HUD_STATUS = false;
    public static final boolean HUD_BACKGROUND = true;
    public static final int HUD_PADDING = 5;
    public static final int HUD_TEXT_COLOR = 0xFFFFFFFF;
    public static final int HUD_BG_COLOR = 0x90000000;
    public static final HudAnchor HUD_ANCHOR = HudAnchor.TOP_LEFT;
    public static final int HUD_MARGIN = 4;
    public static final int HUD_X = 4;
    public static final int HUD_Y = 4;
}
