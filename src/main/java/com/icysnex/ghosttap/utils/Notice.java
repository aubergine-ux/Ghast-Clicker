package com.icysnex.ghosttap.utils;

public final class Notice {

    private static volatile String message = "";
    private static volatile long until = 0;

    private Notice() {
    }

    public static void show(String text) {
        message = text;
        until = System.currentTimeMillis() + 2500;
    }

    public static String current() {
        return System.currentTimeMillis() < until ? message : null;
    }
}
