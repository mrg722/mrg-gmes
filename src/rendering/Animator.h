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
        timer += dt;
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
        if (texture.id == 0) return;
        const float frameWidth = static_cast<float>(texture.width) / static_cast<float>(cols);
        const float frameHeight = static_cast<float>(texture.height) / static_cast<float>(rows);
        const int col = std::clamp(currentFrame % cols, 0, cols - 1);
        const int row = std::clamp(currentFrame / cols, 0, rows - 1);

        Rectangle source = {
            col * frameWidth,
            row * frameHeight,
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
