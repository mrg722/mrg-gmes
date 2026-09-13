#include "rendering/AssetManager.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <string>
#include <vector>

namespace district_fury {
namespace {

std::string ResolveAssetPath(const std::string& relativePath) {
    const std::vector<std::string> candidates = {
        relativePath,
        "../" + relativePath,
        "../../" + relativePath,
        "../../../" + relativePath
    };
    for (const auto& path : candidates) {
        if (FileExists(path.c_str())) return path;
    }
    return relativePath;
}

bool NearColor(const Color& a, const Color& b, int threshold) {
    return std::abs(static_cast<int>(a.r) - static_cast<int>(b.r)) +
           std::abs(static_cast<int>(a.g) - static_cast<int>(b.g)) +
           std::abs(static_cast<int>(a.b) - static_cast<int>(b.b)) <= threshold;
}

Texture2D LoadSpriteTexture(const std::string& path) {
    Image image = LoadImage(path.c_str());
    if (image.data == nullptr) return Texture2D{0};

    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    auto* pixels = static_cast<Color*>(image.data);
    const int width = image.width;
    const int height = image.height;
    if (width <= 0 || height <= 0) {
        UnloadImage(image);
        return Texture2D{0};
    }

    // Remove only the connected background around the border. This preserves black
    // clothing/details inside the character while making the generated sheet usable
    // as a transparent sprite texture.
    const Color key = pixels[0];
    constexpr int kColorThreshold = 45;
    std::vector<unsigned char> visited(static_cast<std::size_t>(width * height), 0);
    std::queue<int> queue;

    auto enqueueIfBackground = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= width || y >= height) return;
        const int index = y * width + x;
        if (visited[static_cast<std::size_t>(index)]) return;
        if (!NearColor(pixels[index], key, kColorThreshold)) return;
        visited[static_cast<std::size_t>(index)] = 1;
        queue.push(index);
    };

    for (int x = 0; x < width; ++x) {
        enqueueIfBackground(x, 0);
        enqueueIfBackground(x, height - 1);
    }
    for (int y = 0; y < height; ++y) {
        enqueueIfBackground(0, y);
        enqueueIfBackground(width - 1, y);
    }

    while (!queue.empty()) {
        const int index = queue.front();
        queue.pop();
        pixels[index].a = 0;
        const int x = index % width;
        const int y = index / width;
        enqueueIfBackground(x + 1, y);
        enqueueIfBackground(x - 1, y);
        enqueueIfBackground(x, y + 1);
        enqueueIfBackground(x, y - 1);
    }

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

}

void AssetManager::LoadAll() {
    if (!textures.empty()) return;

    const std::string background = ResolveAssetPath("assets/backgrounds/bg_industrial.jpg");
    const std::string rayden = ResolveAssetPath("assets/characters/rayden_sheet.jpg");
    const std::string grinder = ResolveAssetPath("assets/enemies/grinder_sheet.jpg");
    const std::string vfx = ResolveAssetPath("assets/vfx/vfx_sheet.jpg");

    textures["bg_industrial"] = LoadTexture(background.c_str());
    textures["rayden_sheet"] = LoadSpriteTexture(rayden);
    textures["grinder_sheet"] = LoadSpriteTexture(grinder);
    textures["vfx_sheet"] = LoadSpriteTexture(vfx);

    if (textures["bg_industrial"].id != 0) SetTextureFilter(textures["bg_industrial"], TEXTURE_FILTER_BILINEAR);
    if (textures["rayden_sheet"].id != 0) SetTextureFilter(textures["rayden_sheet"], TEXTURE_FILTER_BILINEAR);
    if (textures["grinder_sheet"].id != 0) SetTextureFilter(textures["grinder_sheet"], TEXTURE_FILTER_BILINEAR);
    if (textures["vfx_sheet"].id != 0) SetTextureFilter(textures["vfx_sheet"], TEXTURE_FILTER_BILINEAR);
}

void AssetManager::UnloadAll() {
    for (auto& pair : textures) {
        if (pair.second.id != 0) UnloadTexture(pair.second);
    }
    textures.clear();
}

Texture2D AssetManager::GetTexture(const std::string& name) {
    const auto it = textures.find(name);
    if (it != textures.end()) return it->second;
    return Texture2D{0};
}

}
