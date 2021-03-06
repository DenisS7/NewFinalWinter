#pragma once
#include "Projectile.h"
#include "newmath.h"
#include "CollisionComponent.h"
#include "../Classes/Room.h"


namespace Character
{
    class enemy_snowman;
}

namespace Weapon
{
    class Snowball :
        private Projectile
    {
    private:
        GameSpace::vec2 destination, initial, moveDirection;
        Character::enemy_snowman* owner;
    public:
        Snowball(Map::Room* newCurrentRoom, GameSpace::vec2 newDestination, GameSpace::vec2 newInitial,
                 Character::enemy_snowman* newOwner) :
            Projectile(newInitial, newCurrentRoom)
        {
            owner = newOwner;
            timeToExplode = 1500;
            destination = newDestination;
            moveDirection = GameSpace::vec2::normalize(destination - locf);
            collision.offset = newmath::make_ivec2(22, 22);
            collision.collisionBox = newmath::make_Rect(static_cast<int>(locf.x) + 22, static_cast<int>(locf.y) + 22,
                                                        20, 20);
            speedf = 0.2f;
            currentSs.freezeFrame(0, true);

            if (abs(moveDirection.x) <= abs(moveDirection.y))
            {
                if (moveDirection.y <= 0)
                    *sprite = *Sprites::get().snowball[0];
                else *sprite = *Sprites::get().snowball[2];
            }
            else
            {
                if (moveDirection.x <= 0)
                    *sprite = *Sprites::get().snowball[3];
                else
                    *sprite = *Sprites::get().snowball[1];
            }
        }

        ~Snowball() override
        {
        }

        void Init(Character::enemy_snowman* newOwner);
        void deleteProjectile() override;
        void explode();

        void Move(float deltaTime) override;
    };
}
