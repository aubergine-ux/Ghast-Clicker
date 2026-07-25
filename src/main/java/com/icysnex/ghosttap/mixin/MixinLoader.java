package com.icysnex.ghosttap.mixin;

import net.minecraft.launchwrapper.Launch;
import net.minecraft.launchwrapper.LaunchClassLoader;
import net.minecraftforge.fml.relauncher.IFMLLoadingPlugin;

import java.lang.reflect.Field;
import java.util.Map;
import java.util.Set;

// This coremod's only job is to let Mixin transform the LWJGL input classes,
// which LaunchWrapper excludes from transformation by default.
//
// It must NOT reference Mixin at all: touching org.spongepowered.asm.* from a
// coremod constructor loads those classes far too early (before any Mixin tweaker
// runs). If that lookup fails it gets cached in LaunchClassLoader's invalidClasses
// set, which then breaks every other mod that legitimately loads Mixin later.
// Mixin is bootstrapped by the MixinTweaker (jar manifest), not here.
public class MixinLoader implements IFMLLoadingPlugin {

    public MixinLoader() {
        System.out.println("[GhostTap] Unlocking LWJGL for Mixin transformation.");
        try {
            Field field = LaunchClassLoader.class.getDeclaredField("classLoaderExceptions");
            field.setAccessible(true);
            @SuppressWarnings("unchecked")
            Set<String> exceptions = (Set<String>) field.get(Launch.classLoader);
            exceptions.remove("org.lwjgl.");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public String[] getASMTransformerClass() {
        return new String[0];
    }

    @Override
    public String getModContainerClass() {
        return null;
    }

    @Override
    public String getSetupClass() {
        return null;
    }

    @Override
    public void injectData(Map<String, Object> data) {
    }

    @Override
    public String getAccessTransformerClass() {
        return null;
    }
}
