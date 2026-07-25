package com.icysnex.ghosttap.config;

import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;

// Shared encoding for the clipboard config tokens:
// - A header line plus key=value lines, base64'd.
// - Import tolerates plain (un-encoded) text for old exports.
public final class ConfigCodec {

    private ConfigCodec() {
    }

    public static String encode(String header, Map<String, String> data) {
        StringBuilder sb = new StringBuilder(header).append('\n');
        
        for (Map.Entry<String, String> e : data.entrySet())
            sb
                .append(e.getKey())
                .append('=')
                .append(e.getValue())
                .append('\n');
        
        return Base64
            .getEncoder()
            .encodeToString(sb
                .toString()
                .getBytes(StandardCharsets.UTF_8));
    }

    // Returns the parsed key/value pairs, or null if the token isn't valid.
    public static Map<String, String> decode(String header, String token) {
        if (token == null)
            return null;

        String data;
        try {
            data = new String(Base64.getDecoder().decode(token.trim()), StandardCharsets.UTF_8);
        } catch (IllegalArgumentException e) {
            data = token;
        }
        if (!data.trim().startsWith(header))
            return null;

        Map<String, String> values = new HashMap<>();
        for (String line : data.split("\\r?\\n")) {
            int eq = line.indexOf('=');
            if (eq > 0)
                values.put(line.substring(0, eq).trim(), line.substring(eq + 1).trim());
        }
        return values;
    }

    public static LinkedHashMap<String, String> map() {
        return new LinkedHashMap<>();
    }

    public static void put(Map<String, String> m, String key, boolean value) {
        m.put(key, value ? "1" : "0");
    }

    public static void put(Map<String, String> m, String key, double value) {
        m.put(key, String.valueOf(value));
    }

    public static void put(Map<String, String> m, String key, String value) {
        m.put(key, value);
    }

    public static boolean flag(Map<String, String> m, String key, boolean def) {
        String v = m.get(key);
        if (v == null)
            return def;
        return v.equals("1") || v.equalsIgnoreCase("true");
    }

    public static int integer(Map<String, String> m, String key, int def) {
        return parse(m.get(key), def);
    }

    public static double number(Map<String, String> m, String key, double def) {
        String v = m.get(key);
        if (v == null)
            return def;
        try {
            return Double.parseDouble(v);
        } catch (NumberFormatException e) {
            return def;
        }
    }

    private static int parse(String v, int def) {
        if (v == null)
            return def;
        try {
            return Integer.parseInt(v);
        } catch (NumberFormatException e) {
            return def;
        }
    }
}
