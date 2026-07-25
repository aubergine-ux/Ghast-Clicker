package com.icysnex.ghosttap.core;

// TOGGLE: key flips on/off.
// HOLD: clicks while key held.
// MOUSE: key arms, then holding the real mouse button autoclicks.
public enum ActivationMode {

    TOGGLE("Toggle"),
    HOLD("Hold"),
    MOUSE("Mouse");

    public final String label;

    ActivationMode(String label) {
        this.label = label;
    }
}
