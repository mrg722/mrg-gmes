#include "rendering/AssetManager.h"
#include <algorithm>
#include <queue>
#include <string>
#include <vector>

namespace district_fury {
namespace {
std::string ResolveAssetPath(const std::vector<std::string>& candidates) {
    for (const auto& path : candidates) if (FileExists(path.c_str())) return path;
    return candidates.empty() ? std::string{} : candidates.front();
}

Texture2D LoadRequiredTexture(const char* key, const std::vector<std::string>& candidates, TextureFilter filter) {
    const std::string path = ResolveAssetPath(candidates);
    if (path.empty() || !FileExists(path.c_str())) {
        TraceLog(LOG_WARNING, "District Fury asset missing: %s", key);
        return Texture2D{0};
    }
    Texture2D texture = LoadTexture(path.c_str());
    if (texture.id == 0) {
        TraceLog(LOG_WARNING, "District Fury asset failed to load: %s (%s)", key, path.c_str());
        return Texture2D{0};
    }
    SetTextureFilter(texture, filter);
    return texture;
}

bool NearWhite(const Color& c) {
    const int mx = std::max({static_cast<int>(c.r), static_cast<int>(c.g), static_cast<int>(c.b)});
    const int mn = std::min({static_cast<int>(c.r), static_cast<int>(c.g), static_cast<int>(c.b)});
    return c.a > 0 && mn >= 225 && (mx - mn) <= 24;
}

void ClearCellBackground(Image& image, int cellX, int cellY, int cellWidth, int cellHeight) {
    std::vector<unsigned char> visited(static_cast<std::size_t>(cellWidth * cellHeight), 0);
    std::queue<int> pending;

    const auto enqueue = [&](int x, int y) {
        if (x < 0 || x >= cellWidth || y < 0 || y >= cellHeight) return;
        const int index = y * cellWidth + x;
        if (visited[static_cast<std::size_t>(index)] != 0) return;
        if (!NearWhite(GetImageColor(image, cellX + x, cellY + y))) return;
        visited[static_cast<std::size_t>(index)] = 1;
        pending.push(index);
    };

    for (int x = 0; x < cellWidth; ++x) {
        enqueue(x, 0);
        enqueue(x, cellHeight - 1);
    }
    for (int y = 0; y < cellHeight; ++y) {
        enqueue(0, y);
        enqueue(cellWidth - 1, y);
    }

    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};
    while (!pending.empty()) {
        const int index = pending.front();
        pending.pop();
        const int x = index % cellWidth;
        const int y = index / cellWidth;
        ImageDrawPixel(&image, cellX + x, cellY + y, {0, 0, 0, 0});
        for (int i = 0; i < 4; ++i) enqueue(x + dx[i], y + dy[i]);
    }

    for (int pass = 0; pass < 2; ++pass) {
        std::vector<int> erase;
        erase.reserve(static_cast<std::size_t>(cellWidth * cellHeight / 20));
        for (int y = 0; y < cellHeight; ++y) {
            for (int x = 0; x < cellWidth; ++x) {
                const Color c = GetImageColor(image, cellX + x, cellY + y);
                if (!NearWhite(c)) continue;
                bool touchesTransparent = false;
                for (int i = 0; i < 4; ++i) {
                    const int nx = x + dx[i];
                    const int ny = y + dy[i];
                    if (nx < 0 || nx >= cellWidth || ny < 0 || ny >= cellHeight) continue;
                    if (GetImageColor(image, cellX + nx, cellY + ny).a == 0) {
                        touchesTransparent = true;
                        break;
                    }
                }
                if (touchesTransparent) erase.push_back(y * cellWidth + x);
            }
        }
        for (const int index : erase) {
            const int x = index % cellWidth;
            const int y = index / cellWidth;
            const Color c = GetImageColor(image, cellX + x, cellY + y);
            ImageDrawPixel(&image, cellX + x, cellY + y, {c.r, c.g, c.b, 0});
        }
    }
}

Texture2D LoadEnemyTexture(const char* key, const std::vector<std::string>& candidates) {
    const std::string path = ResolveAssetPath(candidates);
    if (path.empty() || !FileExists(path.c_str())) {
        TraceLog(LOG_WARNING, "District Fury enemy asset missing: %s", key);
        return Texture2D{0};
    }

    Image image = LoadImage(path.c_str());
    if (image.data == nullptr) {
        TraceLog(LOG_WARNING, "District Fury enemy asset failed to load: %s (%s)", key, path.c_str());
        return Texture2D{0};
    }
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    if (image.data == nullptr) {
        UnloadImage(image);
        return Texture2D{0};
    }

    if (image.width == 512 && image.height == 384) {
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 4; ++col) {
                ClearCellBackground(image, col * 128, row * 128, 128, 128);
            }
        }
    }

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    if (texture.id == 0) return Texture2D{0};
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    return texture;
}
}

void AssetManager::LoadAll() {
    if (!textures.empty()) return;
    textures["bg_industrial"] = LoadRequiredTexture("old_steel_yard_clean", {"assets/backgrounds/old_steel_yard_clean.png", "../assets/backgrounds/old_steel_yard_clean.png", "../../assets/backgrounds/old_steel_yard_clean.png"}, TEXTURE_FILTER_BILINEAR);
    textures["bg_steel_deep"] = LoadRequiredTexture("old_steel_yard_deep_clean", {"assets/backgrounds/old_steel_yard_deep_clean.png", "../assets/backgrounds/old_steel_yard_deep_clean.png", "../../assets/backgrounds/old_steel_yard_deep_clean.png"}, TEXTURE_FILTER_BILINEAR);
    textures["bg_mercado_antiguo"] = LoadRequiredTexture("mercado_antiguo_clean", {"assets/backgrounds/mercado_antiguo_clean.png", "../assets/backgrounds/mercado_antiguo_clean.png", "../../assets/backgrounds/mercado_antiguo_clean.png"}, TEXTURE_FILTER_POINT);
    textures["bg_zona_quimica"] = LoadRequiredTexture("zona_quimica_clean", {"assets/backgrounds/zona_quimica_clean.png", "../assets/backgrounds/zona_quimica_clean.png", "../../assets/backgrounds/zona_quimica_clean.png"}, TEXTURE_FILTER_POINT);
    textures["rayden_clean"] = LoadRequiredTexture("rayden_clean", {"assets/characters/rayden_clean.png", "../assets/characters/rayden_clean.png", "../../assets/characters/rayden_clean.png"}, TEXTURE_FILTER_POINT);
    textures["punk_clean"] = LoadEnemyTexture("punk_clean", {"assets/enemies/punk_clean.png", "../assets/enemies/punk_clean.png", "../../assets/enemies/punk_clean.png"});
    textures["charger_clean"] = LoadEnemyTexture("charger_clean", {"assets/enemies/charger_clean.png", "../assets/enemies/charger_clean.png", "../../assets/enemies/charger_clean.png"});
    textures["brute_clean"] = LoadEnemyTexture("brute_clean", {"assets/enemies/brute_clean.png", "../assets/enemies/brute_clean.png", "../../assets/enemies/brute_clean.png"});
    textures["enforcer_clean"] = LoadEnemyTexture("enforcer_clean", {"assets/enemies/enforcer_clean.png", "../assets/enemies/enforcer_clean.png", "../../assets/enemies/enforcer_clean.png"});
    textures["chemical_soldier_clean"] = LoadEnemyTexture("chemical_soldier_clean", {"assets/enemies/chemical_soldier_clean.png", "../assets/enemies/chemical_soldier_clean.png", "../../assets/enemies/chemical_soldier_clean.png"});
    textures["urban_ninja_clean"] = LoadEnemyTexture("urban_ninja_clean", {"assets/enemies/urban_ninja_clean.png", "../assets/enemies/urban_ninja_clean.png", "../../assets/enemies/urban_ninja_clean.png"});
    textures["mutant_clean"] = LoadEnemyTexture("mutant_clean", {"assets/enemies/mutant_clean.png", "../assets/enemies/mutant_clean.png", "../../assets/enemies/mutant_clean.png"});
    textures["armored_guard_clean"] = LoadEnemyTexture("armored_guard_clean", {"assets/enemies/armored_guard_clean.png", "../assets/enemies/armored_guard_clean.png", "../../assets/enemies/armored_guard_clean.png"});
}

void AssetManager::UnloadAll() {
    for (auto& pair : textures) if (pair.second.id != 0) UnloadTexture(pair.second);
    textures.clear();
}

Texture2D AssetManager::GetTexture(const std::string& name) {
    const auto it = textures.find(name);
    return it != textures.end() ? it->second : Texture2D{0};
}
}
