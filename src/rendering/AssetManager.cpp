#include "rendering/AssetManager.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace district_fury {
namespace {

std::string ResolveAssetPath(const std::vector<std::string>& candidates) {
    for (const auto& path : candidates) {
        if (FileExists(path.c_str())) return path;
    }
    return candidates.empty() ? std::string{} : candidates.front();
}

// Legacy JPG cleanup is intentionally isolated. Clean DF-006 RGBA assets are
// loaded directly so their authored anti-aliased edges and effect alpha stay intact.
Texture2D LoadLegacySpriteTexture(const std::string& path, int columns, int rows) {
    Image image = LoadImage(path.c_str());
    if (image.data == nullptr) return Texture2D{0};

    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    auto* pixels = static_cast<Color*>(image.data);
    const int width = image.width;
    const int height = image.height;
    if (width <= 0 || height <= 0 || columns <= 0 || rows <= 0) {
        UnloadImage(image);
        return Texture2D{0};
    }

    // Only remove pixels directly connected to each cell's border. This remains
    // a fallback for the old JPG sheets; new assets are already clean RGBA.
    for (int row = 0; row < rows; ++row) {
        const int y0 = (row * height) / rows;
        const int y1 = ((row + 1) * height) / rows;
        for (int col = 0; col < columns; ++col) {
            const int x0 = (col * width) / columns;
            const int x1 = ((col + 1) * width) / columns;
            const int cellWidth = x1 - x0;
            const int cellHeight = y1 - y0;
            if (cellWidth <= 2 || cellHeight <= 2) continue;

            const Color key = pixels[y0 * width + x0];
            std::vector<unsigned char> visited(static_cast<std::size_t>(cellWidth * cellHeight), 0);
            std::vector<int> stack;
            const auto idx = [cellWidth](int x, int y) { return y * cellWidth + x; };

            const auto nearColor = [](const Color& a, const Color& b) {
                return std::abs(static_cast<int>(a.r) - static_cast<int>(b.r)) +
                       std::abs(static_cast<int>(a.g) - static_cast<int>(b.g)) +
                       std::abs(static_cast<int>(a.b) - static_cast<int>(b.b)) <= 55;
            };

            auto visit = [&](int lx, int ly) {
                if (lx < 0 || ly < 0 || lx >= cellWidth || ly >= cellHeight) return;
                const int li = idx(lx, ly);
                if (visited[static_cast<std::size_t>(li)]) return;
                const Color pixel = pixels[(y0 + ly) * width + (x0 + lx)];
                if (pixel.a != 0 && !nearColor(pixel, key)) return;
                visited[static_cast<std::size_t>(li)] = 1;
                stack.push_back(li);
            };

            for (int x = 0; x < cellWidth; ++x) {
                visit(x, 0);
                visit(x, cellHeight - 1);
            }
            for (int y = 0; y < cellHeight; ++y) {
                visit(0, y);
                visit(cellWidth - 1, y);
            }

            while (!stack.empty()) {
                const int li = stack.back();
                stack.pop_back();
                const int lx = li % cellWidth;
                const int ly = li / cellWidth;
                pixels[(y0 + ly) * width + (x0 + lx)].a = 0;
                visit(lx + 1, ly);
                visit(lx - 1, ly);
                visit(lx, ly + 1);
                visit(lx, ly - 1);
            }
        }
    }

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

Texture2D LoadCleanTexture(const std::string& path) {
    if (!FileExists(path.c_str())) return Texture2D{0};
    return LoadTexture(path.c_str());
}

}

void AssetManager::LoadAll() {
    if (!textures.empty()) return;

    const std::string background = ResolveAssetPath({
        "assets/backgrounds/old_steel_yard_clean.jpg",
        "../assets/backgrounds/old_steel_yard_clean.jpg",
        "../../assets/backgrounds/old_steel_yard_clean.jpg",
        "assets/backgrounds/bg_industrial.jpg",
        "../assets/backgrounds/bg_industrial.jpg",
        "../../assets/backgrounds/bg_industrial.jpg"
    });

    const std::string raydenClean = ResolveAssetPath({
        "assets/characters/rayden_clean.png",
        "../assets/characters/rayden_clean.png",
        "../../assets/characters/rayden_clean.png"
    });
    const std::string raydenLegacy = ResolveAssetPath({
        "assets/characters/rayden_sheet.jpg",
        "../assets/characters/rayden_sheet.jpg",
        "../../assets/characters/rayden_sheet.jpg"
    });

    const std::string enemyLegacy = ResolveAssetPath({
        "assets/enemies/grinder_sheet.jpg",
        "../assets/enemies/grinder_sheet.jpg",
        "../../assets/enemies/grinder_sheet.jpg"
    });

    const std::string punk = ResolveAssetPath({
        "assets/enemies/punk_clean.png", "../assets/enemies/punk_clean.png",
        "../../assets/enemies/punk_clean.png"
    });
    const std::string charger = ResolveAssetPath({
        "assets/enemies/charger_clean.png", "../assets/enemies/charger_clean.png",
        "../../assets/enemies/charger_clean.png"
    });
    const std::string brute = ResolveAssetPath({
        "assets/enemies/brute_clean.png", "../assets/enemies/brute_clean.png",
        "../../assets/enemies/brute_clean.png"
    });
    const std::string enforcer = ResolveAssetPath({
        "assets/enemies/enforcer_clean.png", "../assets/enemies/enforcer_clean.png",
        "../../assets/enemies/enforcer_clean.png"
    });
    const std::string vfx = ResolveAssetPath({
        "assets/vfx/vfx_sheet.jpg",
        "../assets/vfx/vfx_sheet.jpg",
        "../../assets/vfx/vfx_sheet.jpg"
    });

    textures["bg_industrial"] = LoadTexture(background.c_str());
    if (textures["bg_industrial"].id != 0) {
        SetTextureFilter(textures["bg_industrial"], TEXTURE_FILTER_BILINEAR);
    }

    auto loadClean = [&](const std::string& key, const std::string& path) {
        const Texture2D texture = LoadCleanTexture(path);
        if (texture.id == 0) return false;
        textures[key] = texture;
        SetTextureFilter(textures[key], TEXTURE_FILTER_POINT);
        return true;
    };

    if (!loadClean("rayden_clean", raydenClean)) {
        textures["rayden_sheet"] = LoadLegacySpriteTexture(raydenLegacy, 5, 3);
        if (textures["rayden_sheet"].id != 0) SetTextureFilter(textures["rayden_sheet"], TEXTURE_FILTER_POINT);
    }

    loadClean("punk_clean", punk);
    loadClean("charger_clean", charger);
    loadClean("brute_clean", brute);
    loadClean("enforcer_clean", enforcer);

    textures["vfx_sheet"] = LoadTexture(vfx.c_str());
    if (textures["vfx_sheet"].id != 0) SetTextureFilter(textures["vfx_sheet"], TEXTURE_FILTER_POINT);
}

void AssetManager::UnloadAll() {
    for (auto& pair : textures) {
        if (pair.second.id != 0) UnloadTexture(pair.second);
    }
    textures.clear();
}

Texture2D AssetManager::GetTexture(const std::string& name) {
    const auto it = textures.find(name);
    return it != textures.end() ? it->second : Texture2D{0};
}

}
