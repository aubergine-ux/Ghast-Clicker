package com.icysnex.ghosttap.core;

// Where the HUD sits.
public enum HudAnchor {

    TOP_LEFT("TL"),
    TOP_RIGHT("TR"),
    BOTTOM_LEFT("BL"),
    BOTTOM_RIGHT("BR"),
    MANUAL("Manual");

    public final String label;

    HudAnchor(String label) {
        this.label = label;
    }
}
