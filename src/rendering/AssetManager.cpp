#include "rendering/AssetManager.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <string>
#include <utility>
#include <vector>

namespace district_fury {
namespace {

std::string ResolveAssetPath(const std::vector<std::string>& candidates) {
    for (const auto& path : candidates) {
        if (FileExists(path.c_str())) return path;
    }
    return candidates.empty() ? std::string{} : candidates.front();
}

bool NearColor(const Color& a, const Color& b, int threshold) {
    return std::abs(static_cast<int>(a.r) - static_cast<int>(b.r)) +
           std::abs(static_cast<int>(a.g) - static_cast<int>(b.g)) +
           std::abs(static_cast<int>(a.b) - static_cast<int>(b.b)) <= threshold;
}

struct Component {
    int area = 0;
    int minX = 0;
    int minY = 0;
    int maxX = 0;
    int maxY = 0;
    std::vector<int> pixels;
};

int RectGap(int aMin, int aMax, int bMin, int bMax) {
    if (aMax < bMin) return bMin - aMax;
    if (bMax < aMin) return aMin - bMax;
    return 0;
}

Texture2D LoadSpriteTexture(const std::string& path, int columns, int rows) {
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

    // Clean each frame independently. This is retained for legacy JPG sheets and is
    // harmless on the DF-005 transparent atlases. It prevents neighboring poses or
    // background fragments from surviving inside another frame.
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
            auto localIndex = [cellWidth](int x, int y) { return y * cellWidth + x; };
            std::vector<int> stack;

            auto visitBackground = [&](int lx, int ly) {
                if (lx < 0 || ly < 0 || lx >= cellWidth || ly >= cellHeight) return;
                const int li = localIndex(lx, ly);
                if (visited[static_cast<std::size_t>(li)]) return;
                const Color& pixel = pixels[(y0 + ly) * width + (x0 + lx)];
                if (pixel.a != 0 && !NearColor(pixel, key, 55)) return;
                visited[static_cast<std::size_t>(li)] = 1;
                stack.push_back(li);
            };

            for (int x = 0; x < cellWidth; ++x) {
                visitBackground(x, 0);
                visitBackground(x, cellHeight - 1);
            }
            for (int y = 0; y < cellHeight; ++y) {
                visitBackground(0, y);
                visitBackground(cellWidth - 1, y);
            }
            while (!stack.empty()) {
                const int li = stack.back();
                stack.pop_back();
                const int lx = li % cellWidth;
                const int ly = li / cellWidth;
                pixels[(y0 + ly) * width + (x0 + lx)].a = 0;
                visitBackground(lx + 1, ly);
                visitBackground(lx - 1, ly);
                visitBackground(lx, ly + 1);
                visitBackground(lx, ly - 1);
            }

            std::fill(visited.begin(), visited.end(), 0);
            std::vector<Component> components;
            for (int ly = 0; ly < cellHeight; ++ly) {
                for (int lx = 0; lx < cellWidth; ++lx) {
                    const int li = localIndex(lx, ly);
                    if (visited[static_cast<std::size_t>(li)]) continue;
                    const Color& pixel = pixels[(y0 + ly) * width + (x0 + lx)];
                    if (pixel.a == 0) {
                        visited[static_cast<std::size_t>(li)] = 1;
                        continue;
                    }

                    Component component;
                    component.minX = component.maxX = lx;
                    component.minY = component.maxY = ly;
                    stack.clear();
                    stack.push_back(li);
                    visited[static_cast<std::size_t>(li)] = 1;

                    while (!stack.empty()) {
                        const int current = stack.back();
                        stack.pop_back();
                        const int cx = current % cellWidth;
                        const int cy = current / cellWidth;
                        component.pixels.push_back(current);
                        ++component.area;
                        component.minX = std::min(component.minX, cx);
                        component.maxX = std::max(component.maxX, cx);
                        component.minY = std::min(component.minY, cy);
                        component.maxY = std::max(component.maxY, cy);

                        for (int oy = -1; oy <= 1; ++oy) {
                            for (int ox = -1; ox <= 1; ++ox) {
                                if (ox == 0 && oy == 0) continue;
                                const int nx = cx + ox;
                                const int ny = cy + oy;
                                if (nx < 0 || ny < 0 || nx >= cellWidth || ny >= cellHeight) continue;
                                const int ni = localIndex(nx, ny);
                                if (visited[static_cast<std::size_t>(ni)]) continue;
                                if (pixels[(y0 + ny) * width + (x0 + nx)].a == 0) {
                                    visited[static_cast<std::size_t>(ni)] = 1;
                                    continue;
                                }
                                visited[static_cast<std::size_t>(ni)] = 1;
                                stack.push_back(ni);
                            }
                        }
                    }
                    components.push_back(std::move(component));
                }
            }

            if (components.empty()) continue;
            std::sort(components.begin(), components.end(), [](const Component& a, const Component& b) {
                return a.area > b.area;
            });
            const Component& main = components.front();
            const int keepGap = std::max(8, static_cast<int>(std::min(cellWidth, cellHeight) * 0.045f));

            for (int ly = 0; ly < cellHeight; ++ly) {
                for (int lx = 0; lx < cellWidth; ++lx) {
                    pixels[(y0 + ly) * width + (x0 + lx)].a = 0;
                }
            }

            for (const auto& component : components) {
                if (&component != &main) {
                    if (component.area < std::max(24, static_cast<int>(main.area * 0.06f))) continue;
                    const int horizontalGap = RectGap(main.minX, main.maxX, component.minX, component.maxX);
                    const int verticalGap = RectGap(main.minY, main.maxY, component.minY, component.maxY);
                    if (horizontalGap > keepGap || verticalGap > keepGap) continue;
                }
                for (const int li : component.pixels) {
                    const int lx = li % cellWidth;
                    const int ly = li / cellWidth;
                    pixels[(y0 + ly) * width + (x0 + lx)].a = 255;
                }
            }
        }
    }

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

}

void AssetManager::LoadAll() {
    if (!textures.empty()) return;

    const std::string background = ResolveAssetPath({
        "assets/backgrounds/old_steel_yard_clean.png",
        "assets/backgrounds/bg_industrial.jpg"
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
    const std::string grinderClean = ResolveAssetPath({
        "assets/enemies/grinder_clean.png",
        "../assets/enemies/grinder_clean.png",
        "../../assets/enemies/grinder_clean.png"
    });
    const std::string grinderLegacy = ResolveAssetPath({
        "assets/enemies/grinder_sheet.jpg",
        "../assets/enemies/grinder_sheet.jpg",
        "../../assets/enemies/grinder_sheet.jpg"
    });
    const std::string vfx = ResolveAssetPath({
        "assets/vfx/vfx_sheet.jpg",
        "../assets/vfx/vfx_sheet.jpg",
        "../../assets/vfx/vfx_sheet.jpg"
    });

    textures["bg_industrial"] = LoadTexture(background.c_str());

    if (FileExists(raydenClean.c_str())) {
        textures["rayden_clean"] = LoadSpriteTexture(raydenClean, 4, 4);
        if (textures["rayden_clean"].id != 0) SetTextureFilter(textures["rayden_clean"], TEXTURE_FILTER_POINT);
    } else {
        textures["rayden_sheet"] = LoadSpriteTexture(raydenLegacy, 5, 3);
        if (textures["rayden_sheet"].id != 0) SetTextureFilter(textures["rayden_sheet"], TEXTURE_FILTER_POINT);
    }

    if (FileExists(grinderClean.c_str())) {
        textures["grinder_clean"] = LoadSpriteTexture(grinderClean, 4, 3);
        if (textures["grinder_clean"].id != 0) SetTextureFilter(textures["grinder_clean"], TEXTURE_FILTER_POINT);
    } else {
        textures["grinder_sheet"] = LoadSpriteTexture(grinderLegacy, 4, 3);
        if (textures["grinder_sheet"].id != 0) SetTextureFilter(textures["grinder_sheet"], TEXTURE_FILTER_POINT);
    }

    textures["vfx_sheet"] = LoadTexture(vfx.c_str());
    if (textures["bg_industrial"].id != 0) SetTextureFilter(textures["bg_industrial"], TEXTURE_FILTER_BILINEAR);
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
    if (it != textures.end()) return it->second;
    return Texture2D{0};
}

}
