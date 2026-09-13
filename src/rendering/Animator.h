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

    Animator()
        : texture{0}, cols(1), rows(1), currentFrame(0), timer(0.0f),
          currentClip{0, 0, 0.1f, true}, isPlaying(false), isFinished(true) {}

    void Init(Texture2D tex, int columns, int rws) {
        texture = tex;
        cols = std::max(1, columns);
        rows = std::max(1, rws);
        currentFrame = 0;
        timer = 0.0f;
        isPlaying = false;
        isFinished = true;
    }

    void Play(AnimationClip clip) {
        currentClip = clip;
        currentClip.startFrame = std::max(0, clip.startFrame);
        currentClip.endFrame = std::max(currentClip.startFrame, clip.endFrame);
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

    void Draw(Vector2 position, float scale, bool flipX, Color tint = WHITE) const {
        if (texture.id == 0 || texture.width <= 0 || texture.height <= 0) return;

        const int safeFrame = std::clamp(currentFrame, 0, cols * rows - 1);
        const int col = safeFrame % cols;
        const int row = safeFrame / cols;

        // Integer cell boundaries avoid fractional atlas coordinates. A one-pixel
        // inset keeps bilinear filtering from sampling a neighbouring frame; the
        // sprite textures themselves are loaded with point filtering as an extra guard.
        const int x0 = (col * texture.width) / cols;
        const int x1 = ((col + 1) * texture.width) / cols;
        const int y0 = (row * texture.height) / rows;
        const int y1 = ((row + 1) * texture.height) / rows;
        const int inset = (x1 - x0 > 4 && y1 - y0 > 4) ? 1 : 0;

        const float frameWidth = static_cast<float>((x1 - x0) - inset * 2);
        const float frameHeight = static_cast<float>((y1 - y0) - inset * 2);
        if (frameWidth <= 1.0f || frameHeight <= 1.0f) return;

        Rectangle source = {
            static_cast<float>(x0 + inset),
            static_cast<float>(y0 + inset),
            flipX ? -frameWidth : frameWidth,
            frameHeight
        };
        Rectangle dest = {
            position.x,
            position.y,
            frameWidth * scale,
            frameHeight * scale
        };
        Vector2 origin = {dest.width * 0.5f, dest.height};
        DrawTexturePro(texture, source, dest, origin, 0.0f, tint);
    }
};

}
