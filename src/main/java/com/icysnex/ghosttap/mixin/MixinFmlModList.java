package com.icysnex.ghosttap.mixin;

import com.icysnex.ghosttap.GhostTap;
import com.icysnex.ghosttap.config.ConfigHandler;
import net.minecraftforge.fml.common.network.handshake.FMLHandshakeMessage;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

import java.util.List;
import java.util.Map;

// Drops GhostTap from the mod list the client sends to servers during the FML
// handshake, so it never shows up in a server-side mod check.
@Mixin(value = FMLHandshakeMessage.ModList.class, remap = false)
public abstract class MixinFmlModList {

    @Shadow
    private Map<String, String> modTags;

    @Inject(method = "<init>(Ljava/util/List;)V", at = @At("RETURN"))
    private void ghostTap$hide(List<?> modList, CallbackInfo ci) {
        if (ConfigHandler.hideFromServers)
            modTags.remove(GhostTap.MODID);
    }
}
