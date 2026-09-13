#include "rendering/AssetManager.h"
#include <iostream>

namespace district_fury {

void AssetManager::LoadAll() {
    textures["bg_industrial"] = LoadTexture("assets/backgrounds/bg_industrial.jpg");
    textures["rayden_sheet"] = LoadTexture("assets/characters/rayden_sheet.jpg");
    textures["grinder_sheet"] = LoadTexture("assets/enemies/grinder_sheet.jpg");
    textures["vfx_sheet"] = LoadTexture("assets/vfx/vfx_sheet.jpg");
    
    // Set texture filters for smoother scaling if needed, but for pixel art Point is best.
    // DALL-E pixel art is usually not 1:1 pixel perfect so Bilinear can sometimes look better 
    // or worse. Let's use Bilinear for the AI generated images so they scale cleanly.
    for (auto& pair : textures) {
        SetTextureFilter(pair.second, TEXTURE_FILTER_BILINEAR);
    }
}

void AssetManager::UnloadAll() {
    for (auto& pair : textures) {
        UnloadTexture(pair.second);
    }
    textures.clear();
}

Texture2D AssetManager::GetTexture(const std::string& name) {
    if (textures.find(name) != textures.end()) {
        return textures[name];
    }
    return Texture2D{0};
}

}
