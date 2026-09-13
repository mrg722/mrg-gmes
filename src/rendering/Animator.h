#pragma once
#include "raylib.h"
#include <algorithm>

namespace district_fury {

struct AnimationClip {
    int startFrame;
    int endFrame;
    float frameDuration;
    bool loop;
};

class Animator {
public:
    Texture2D texture;
    int cols;
    int rows;
    int currentFrame;
    float timer;
    AnimationClip currentClip;
    bool isPlaying;
    bool isFinished;
    bool normalizedAtlas;

    Animator()
        : texture{0}, cols(1), rows(1), currentFrame(0), timer(0.0f),
          currentClip{0, 0, 0.1f, true}, isPlaying(false), isFinished(true),
          normalizedAtlas(false) {}

    void Init(Texture2D tex, int columns, int rws, bool normalized = false) {
        texture = tex;
        cols = std::max(1, columns);
        rows = std::max(1, rws);
        currentFrame = 0;
        timer = 0.0f;
        isPlaying = false;
        isFinished = true;
        normalizedAtlas = normalized;
    }

    void Play(AnimationClip clip) {
        const int frameCount = std::max(1, cols * rows);
        currentClip = clip;
        currentClip.startFrame = std::clamp(clip.startFrame, 0, frameCount - 1);
        currentClip.endFrame = std::clamp(std::max(currentClip.startFrame, clip.endFrame), 0, frameCount - 1);
        currentClip.frameDuration = std::max(0.016f, clip.frameDuration);
        currentFrame = currentClip.startFrame;
        timer = 0.0f;
        isPlaying = true;
        isFinished = false;
    }

    void Update(float dt) {
        if (!isPlaying || isFinished) return;
        timer += std::max(0.0f, dt);
        while (timer >= currentClip.frameDuration) {
            timer -= currentClip.frameDuration;
            ++currentFrame;
            if (currentFrame > currentClip.endFrame) {
                if (currentClip.loop) {
                    currentFrame = currentClip.startFrame;
                } else {
                    currentFrame = currentClip.endFrame;
                    isFinished = true;
                    isPlaying = false;
                    break;
                }
            }
        }
    }

    void Draw(Vector2 feetPosition, float scale, bool flipX, Color tint = WHITE) const {
        if (texture.id == 0 || texture.width <= 0 || texture.height <= 0) return;

        const int frameCount = std::max(1, cols * rows);
        const int safeFrame = std::clamp(currentFrame, 0, frameCount - 1);
        const int col = safeFrame % cols;
        const int row = safeFrame / cols;

        const int x0 = (col * texture.width) / cols;
        const int x1 = ((col + 1) * texture.width) / cols;
        const int y0 = (row * texture.height) / rows;
        const int y1 = ((row + 1) * texture.height) / rows;
        const float frameWidth = static_cast<float>(x1 - x0);
        const float frameHeight = static_cast<float>(y1 - y0);
        if (frameWidth <= 1.0f || frameHeight <= 1.0f) return;

        Rectangle source = {
            static_cast<float>(x0),
            static_cast<float>(y0),
            flipX ? -frameWidth : frameWidth,
            frameHeight
        };

        // Clean DF-006 atlases are exported to fixed cells and treated as authored
        // artwork. The character's world position is its foot anchor, not its center.
        if (normalizedAtlas) {
            const float width = frameWidth * scale;
            const float height = frameHeight * scale;
            const Rectangle dest = {
                feetPosition.x - width * 0.5f,
                feetPosition.y - height,
                width,
                height
            };
            DrawTexturePro(texture, source, dest, {0.0f, 0.0f}, 0.0f, tint);
            return;
        }

        // Legacy fallback: inset one pixel to reduce neighbouring-frame sampling.
        const int inset = (x1 - x0 > 4 && y1 - y0 > 4) ? 1 : 0;
        const float safeWidth = frameWidth - inset * 2.0f;
        const float safeHeight = frameHeight - inset * 2.0f;
        const Rectangle safeSource = {
            static_cast<float>(x0 + inset),
            static_cast<float>(y0 + inset),
            flipX ? -safeWidth : safeWidth,
            safeHeight
        };
        const Rectangle dest = {
            feetPosition.x - safeWidth * scale * 0.5f,
            feetPosition.y - safeHeight * scale,
            safeWidth * scale,
            safeHeight * scale
        };
        DrawTexturePro(texture, safeSource, dest, {0.0f, 0.0f}, 0.0f, tint);
    }
};

}
