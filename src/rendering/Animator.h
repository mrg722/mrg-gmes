#pragma once
#include "raylib.h"
#include <vector>

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

    Animator() : cols(1), rows(1), currentFrame(0), timer(0.0f), isPlaying(false), isFinished(true) {}
    
    void Init(Texture2D tex, int columns, int rws) {
        texture = tex;
        cols = columns;
        rows = rws;
    }
    
    void Play(AnimationClip clip) {
        currentClip = clip;
        currentFrame = clip.startFrame;
        timer = 0.0f;
        isPlaying = true;
        isFinished = false;
    }
    
    void Update(float dt) {
        if (!isPlaying || isFinished) return;
        
        timer += dt;
        if (timer >= currentClip.frameDuration) {
            timer = 0.0f;
            currentFrame++;
            if (currentFrame > currentClip.endFrame) {
                if (currentClip.loop) {
                    currentFrame = currentClip.startFrame;
                } else {
                    currentFrame = currentClip.endFrame;
                    isFinished = true;
                }
            }
        }
    }
    
    void Draw(Vector2 position, float scale, bool flipX, Color tint = WHITE) const {
        if (texture.id == 0) return;
        
        float frameWidth = (float)texture.width / cols;
        float frameHeight = (float)texture.height / rows;
        
        int col = currentFrame % cols;
        int row = currentFrame / cols;
        
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
        
        Vector2 origin = { dest.width / 2.0f, dest.height }; // Bottom center
        
        DrawTexturePro(texture, source, dest, origin, 0.0f, tint);
    }
};

}
