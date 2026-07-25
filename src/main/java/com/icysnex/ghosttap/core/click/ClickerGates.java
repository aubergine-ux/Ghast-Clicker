package com.icysnex.ghosttap.core.click;

import com.icysnex.ghosttap.core.Defaults;

import java.util.Arrays;

// Per-clicker conditions deciding whether clicking is allowed right now.
public class ClickerGates {

    public final boolean[] slots = new boolean[9];

    // Allowed held-item categories (all true = any item).
    public boolean weapons;
    public boolean tools;
    public boolean blocks;
    public boolean other;

    public boolean allowBlockBreak;
    public boolean allowInMenu;
    public boolean pauseWhileUsingItem;

    // Allowed game modes.
    public boolean survival;
    public boolean creative;
    public boolean adventure;

    // Combat: only click when an entity is in sight within a random reach.
    public boolean entityOnly;
    public double reachMin;
    public double reachMax;

    // Right-click only: fire only when a block can be placed where aimed.
    public boolean placeableOnly;


    public ClickerGates(Defaults.Profile d) {
        resetTo(d);
    }

    // Copy every gate from a defaults profile (used on construct and on reset).
    public void resetTo(Defaults.Profile d) {
        weapons = d.weapons;
        tools = d.tools;
        blocks = d.blocks;
        other = d.other;
        allowBlockBreak = d.allowBlockBreak;
        allowInMenu = d.allowInMenu;
        pauseWhileUsingItem = d.pauseWhileUsingItem;
        survival = d.survival;
        creative = d.creative;
        adventure = d.adventure;
        entityOnly = d.entityOnly;
        reachMin = d.reachMin;
        reachMax = d.reachMax;
        placeableOnly = d.placeableOnly;
        Arrays.fill(slots, d.slot);
    }
}
