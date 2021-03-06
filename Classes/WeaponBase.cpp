#include "WeaponBase.h"
#include <algorithm>
#include "Player.h"

#include "../template.h"
#include "Sprites.h"

namespace Weapon
{
    void WeaponBase::Init(GameSpace::vec2 newDrawLocf, Character::Player* newPlayer)
    {
        player = newPlayer;
        wpaths[0].path = "assets/Weapons/crossbow_down.png";
        wpaths[1].path = "assets/Weapons/crossbow_left.png";
        wpaths[2].path = "assets/Weapons/crossbow_up.png";
        wpaths[3].path = "assets/Weapons/crossbow_right.png";

        wpaths[4].path = "assets/Weapons/arrow_down.png";
        wpaths[5].path = "assets/Weapons/arrow_left.png";
        wpaths[6].path = "assets/Weapons/arrow_up.png";
        wpaths[7].path = "assets/Weapons/arrow_right.png";

        wpaths[4].frameTime = wpaths[5].frameTime = wpaths[6].frameTime = wpaths[7].frameTime = 30.0f;

        weaponType = 5;

        drawLocf.x = newDrawLocf.x;
        drawLocf.y = newDrawLocf.y;
    }

    void WeaponBase::reload(float deltaTime)
    {
        if (reloading < reloadTime)
            reloading += deltaTime;
    }

    void WeaponBase::changeDirection(int direction)
    {
        directionFacing = direction;
        sprite.SetFile(wpaths[direction].path, 1, 0);
    }

    void WeaponBase::changeVisibility(bool newVisible)
    {
        visible = newVisible;
    }

    void WeaponBase::shootArrows()
    {
        isShooting = true;
    }

    void WeaponBase::stopShooting()
    {
        isShooting = false;
    }

    void WeaponBase::deleteArrow(Arrow* endArrow)
    {
        /* https://stackoverflow.com/questions/3385229/c-erase-vector-element-by-value-rather-than-by-position */
        auto position = std::find(arrows.begin(), arrows.end(), endArrow);
        if (position != arrows.end())
            arrows.erase(position);
    }

    void WeaponBase::drawWeapon(float deltaTime)
    {
        sprite.Draw(player->screen, static_cast<int>(player->getDrawLocation().x),
                    static_cast<int>(player->getDrawLocation().y));
    }


    void WeaponBase::update(float deltaTime)
    {
        if (isShooting)
        {
            if (reloading >= reloadTime)
            {
                reloading = 0;
                auto newArrow = new Arrow(player->getLocation(), player->currentRoom, player->getDirectionFacing(),
                                          player->getMousePosition(), 
                                          this,
                                          wpaths[player->getDirectionFacing() + 4].path,
                                          arrowCol[player->getDirectionFacing()], damage
                                         );
                arrows.push_back(newArrow);
            }
        }
        drawWeapon(deltaTime);
    }
}
