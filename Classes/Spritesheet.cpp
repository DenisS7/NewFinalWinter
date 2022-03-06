#include "Spritesheet.h"
#include "../surface.h"
#include <iostream>
#include "../template.h"


Spritesheet::Spritesheet(char* path, int row, int column, GameSpace::Sprite* newSprite)
{
    image = new GameSpace::Surface{ "assets/Player/player_idle.png" };
    image->SetFile(path);
    rows = row;
    columns = column;
    currentFrame = (currentRow * column) % (row * column);
    sprite = newSprite;
    int ok = 0;
    if (ok == 0)
        ok = 0;
}

Spritesheet::~Spritesheet()
{
    rows = 0;
    columns = 0;
}

void Spritesheet::changeSpritesheet(char* path, int row, int column, int newCurrentRow, GameSpace::Sprite* newSprite)
{
    currentTime = 0;
    rows = row;
    currentRow = newCurrentRow;
    currentFrame = currentRow * column;
    columns = column;
    sprite = newSprite;
    image->SetFile(path);
}

void Spritesheet::setFrameTime(float newFrameTime)
{
    frameTime = newFrameTime;
}

void Spritesheet::setDirection(int newRow)
{
    currentFrame = newRow * columns + currentFrame;
    currentRow = newRow;
}

void Spritesheet::freezeFrame(int frame, bool isFreezed)
{
    if (isFreezed)
    {
        currentFrame = frame;
        freezedColumn = 0;
        currentRow = frame / columns;
    }
    else
    {
        // currentTime = 0.0f;
        freezedColumn = 1;
    }
}



void Spritesheet::calculateNextFrame()
{
    currentFrame = (currentFrame + 1 * freezedColumn) % columns + columns * currentRow;
}

void Spritesheet::changeVisiblity(bool newVisible)
{
    visible = newVisible;
}

void Spritesheet::drawNextSprite(float deltaTime, GameSpace::Surface* screen, GameSpace::vec2 drawLocf)
{
    if (visible)
    {
        currentTime += deltaTime * freezedColumn;
        if (currentTime >= frameTime)
        {
            calculateNextFrame();
            currentTime -= frameTime;
            sprite->SetFrame(currentFrame);
        }
        sprite->Draw(screen, (int)drawLocf.x, (int)drawLocf.y);
    }
}