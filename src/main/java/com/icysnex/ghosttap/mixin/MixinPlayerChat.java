package com.icysnex.ghosttap.mixin;

import com.icysnex.ghosttap.commands.SilentChat;
import net.minecraft.client.entity.EntityPlayerSP;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

// Intercepts outgoing chat so GhostTap's silent command is never sent.
@Mixin(EntityPlayerSP.class)
public abstract class MixinPlayerChat {

    @Inject(method = "sendChatMessage(Ljava/lang/String;)V", at = @At("HEAD"), cancellable = true)
    private void ghostTap$onChat(String message, CallbackInfo ci) {
        if (SilentChat.handle(message))
            ci.cancel();
    }
}
