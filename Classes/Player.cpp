#include "Player.h"
#include "../game.h"

#include <SDL.h>
#include "../template.h"
#include "newmath.h"
#include "CollisionCheck.h"


namespace Character
{
    void Player::restart()
    {
        //restart all parameters 
        for (int i = 0; i < 5; i++)
            potionTimers[i] = 0;
        for (int i = 0; i < explosions.size();)
            explosions[i]->destroy();

        health = healthBase;
        move.speed = speedBase;

        weapon.setDamage(arrowDamageBase);
        weapon.setReloadTime(firerateBase);

        shieldSs.setDirection(0);
        shieldSs.freezeFrame(0, true);
        isShieldCreating = 0;

        locf.x = drawLocf.x = static_cast<float>(middleScreen.x) - 50;
        locf.y = drawLocf.y = static_cast<float>(middleScreen.y) - 50;
        collisionBox.setCollisionBox(static_cast<int>(locf.x) + collisionBox.offset.x,
                                     static_cast<int>(locf.y) + collisionBox.offset.y, 36, 36);
        won = false;
        currentState = 0;
        directionFacing = 0;
        isHoldingGun = false;

        isDead = false;
        changeActionSprite(idle);
        currentSs.setFrame(0);
        for (int i = 0; i < weapon.arrows.size();)
            weapon.deleteArrow(weapon.arrows[i]);
        weapon.changeDirection(0);
        mapManager->restart();
        currentRoom = &mapManager->rooms[mapManager->getStart().x + mapManager->getRoomAm().x * mapManager->getStart().
            y];
        points = 0;
        locf.x = drawLocf.x = static_cast<float>(middleScreen.x) - 50;
        locf.y = drawLocf.y = static_cast<float>(middleScreen.y) - 50;
        equipWeapon(0);
        currentRoom->speed = move.speed;
        currentSs.changeVisiblity(true);
    }

    void Player::init(GameSpace::Surface* newScreen, Map::Room* newRoom, Map::MapManager* newMapManager, const Uint8* newKeystate)
    {
        currentSs.setDirection(0);
        currentSs.setFrameTime(sspaths[1].frameTime);
        updateScreen(newScreen);
        updateRoom(newRoom);
        updateMapManager(newMapManager);
        updateKeystate(newKeystate);

        currentRoom->speed = move.speed;
        collisionBox.setCollisionBox(static_cast<int>(locf.x) + collisionBox.offset.x, static_cast<int>(locf.y) + collisionBox.offset.y, 36, 36);

        weapon.Init(drawLocf, this);
    }

    void Player::checkStatus()
    {
        //change player status
        if (currentState == idle && (move.side[0] || move.side[1] || move.side[2] || move.side[3]))
        {
            if (isHoldingGun)
                currentState = runWithGun;
            else
                currentState = run;

            changeActionSprite(currentState);
        }
        else if ((currentState == run || currentState == runWithGun) && !(move.side[0] || move.side[1] || move.side[2]  || move.side[3]))
        {
            currentState = idle;
            changeActionSprite(currentState);
        }
        else if (isHoldingGun == true && currentState == run)
        {
            currentState = runWithGun;
            changeActionSprite(currentState);
        }
        else if (isHoldingGun == false && currentState == runWithGun && (move.side[0] || move.side[1] || move.side[2] || move.side[3]))
        {
            currentState = run;
            changeActionSprite(currentState);
        }
    }

    newmath::ivec2 Player::getCurrentPos()
    {
        //get middle tile
        int x = (static_cast<int>(locf.x) + sprite->GetWidth() / 2) / 32;
        int y = (static_cast<int>(locf.y) + sprite->GetHeight() / 2) / 32;
        newmath::ivec2 pos = newmath::make_ivec2(x, y);
        return pos;
    }

    void Player::speedBoost()
    {
        move.speed += speedAdd;
        currentRoom->speed = move.speed;
        potionTimers[speed] = 9000;
    }

    void Player::damageBoost()
    {
        weapon.setDamage(arrowDamageBase + damageAdd);
        potionTimers[damage] = 5000;
    }

    void Player::firerateBoost()
    {
        weapon.setReloadTime(firerateBase - firerateAdd);
        potionTimers[firerate] = 5000;
    }

    void Player::createShield()
    {
        potionTimers[shield] = 1;
        shieldSs.changeVisiblity(true);
        shieldSs.freezeFrame(0, false);
        isShieldCreating = 1;
    }

    void Player::checkPotions(float deltaTime)
    {
        //countdown timers of the potions applied
        if (potionTimers[speed])
        {
            potionTimers[speed] -= deltaTime;
            if (potionTimers[speed] <= 0)
            {
                potionTimers[speed] = 0;
                if (isHoldingGun)
                {
                    move.speed = speedBase;
                    currentRoom->speed = speedBase;
                }
                else
                {
                    move.speed = speedBase + 0.05f;
                    currentRoom->speed = speedBase + 0.05f;
                }
            }
        }
        if (potionTimers[firerate])
        {
            potionTimers[firerate] -= deltaTime;
            if (potionTimers[firerate] <= 0)
            {
                potionTimers[firerate] = 0;
                weapon.setReloadTime(firerateBase);
            }
        }
        if (potionTimers[damage])
        {
            potionTimers[damage] -= deltaTime;
            if (potionTimers[damage] <= 0)
            {
                potionTimers[damage] = 0;
                weapon.setDamage(arrowDamageBase);
            }
        }
    }

    void Player::die()
    {
        equipWeapon(0);
        isDead = true;
        changeActionSprite(dead);
    }

    void Player::modifyPoints(int newPoints)
    {
        points += newPoints;
    }

    void Player::takeDamage(int damage)
    {
        if (!potionTimers[shield] || damage <= 0) // there's not shield
        {
            health -= damage;
            if (health <= 0 && !isDead)
                die();
            health = newmath::clamp(health, 0, 100);
        }
        else // play shield destroyed animation and remove shield
        {
            shieldSs.freezeFrame(11, false);
            shieldSs.setFrame(11);
            shieldSs.setDirection(1);
            isShieldCreating = 1;
            potionTimers[shield] = 0;
        }
    }

    void Player::checkDirection(int n)
    {
        //change the direction of the player sprite
        if (directionFacing != n)
        {
            if ((move.side[n] && !move.side[directionFacing]) || weapon.isShooting) //if the player is shooting the shooting direction has priority and not the keyboard input
            {
                weapon.changeDirection(n);
                currentSs.setDirection(n);
                directionFacing = n;
            }
        }
    }

    void Player::updateMapManager(Map::MapManager* newMapManager)
    {
        mapManager = newMapManager;
    }

    void Player::updateScreen(GameSpace::Surface* newScreen)
    {
        screen = newScreen;
        middleScreen.x = screen->GetWidth() / 2 - sprite->GetWidth() / 2;
        middleScreen.y = screen->GetHeight() / 2 - sprite->GetHeight() / 2;

        locf.x = drawLocf.x = static_cast<float>(middleScreen.x) - 50;
        locf.y = drawLocf.y = static_cast<float>(middleScreen.y) - 50;


        collisionBox.setCollisionBox(static_cast<int>(locf.x) + collisionBox.offset.x, static_cast<int>(locf.y) + collisionBox.offset.y, 36, 36);
    }

    void Player::updateRoom(Map::Room* newRoom)
    {
        currentRoom = newRoom;
    }

    void Player::updateKeystate(const Uint8* keystate)
    {
        this->keystate = keystate;
    }

    void Player::input(float deltaTime)
    {
        if (!isDead)
        {
            moveDown(keystate[SDL_SCANCODE_S], deltaTime);
            moveLeft(keystate[SDL_SCANCODE_A], deltaTime);
            moveUp(keystate[SDL_SCANCODE_W], deltaTime);
            moveRight(keystate[SDL_SCANCODE_D], deltaTime);

            weapon.reload(deltaTime);
        }
    }

    void Player::mouseLoc(int x, int y)
    {
        mousePosition.x = static_cast<float>(x);
        mousePosition.y = static_cast<float>(y);
        if (weapon.isShooting)
        {
            const float distx = static_cast<float>(screen->GetWidth()) * (static_cast<float>(y) / screen->GetHeight());
            const float disty = static_cast<float>(screen->GetHeight()) * (static_cast<float>(x) / screen->GetWidth());

            const float minx = GameSpace::Min(distx, screen->GetWidth() - distx);
            const float maxx = screen->GetWidth() - minx;


            const float miny = GameSpace::Min(disty, screen->GetHeight() - disty);
            const float maxy = screen->GetHeight() - miny;

            //screen is made of 4 parts (delimited from the corners to the middle lines)
            if (x >= minx && x <= maxx)
            {
                if (y < screen->GetHeight() / 2)
                    checkDirection(2);
                else checkDirection(0);
            }
            else if (y >= miny && y <= maxy)
            {
                if (x < screen->GetWidth() / 2)
                    checkDirection(1);
                else checkDirection(3);
            }
        }
    }

    void Player::moveDown(bool down, float deltaTime)
    {
        move.side[0] = down;
        if (down)
        {
            addMovement(0, 1, deltaTime);
            if (!weapon.isShooting)
                checkDirection(0);
        }
        checkStatus();
    }

    void Player::moveLeft(bool left, float deltaTime)
    {
        move.side[1] = left;
        if (left)
        {
            addMovement(-1, 0, deltaTime);
            if (!weapon.isShooting)
                checkDirection(1);
        }
        checkStatus();
    }

    void Player::moveUp(bool up, float deltaTime)
    {
        move.side[2] = up;
        if (up)
        {
            addMovement(0, -1, deltaTime);
            if (!weapon.isShooting)
                checkDirection(2);
        }
        checkStatus();
    }

    void Player::moveRight(bool right, float deltaTime)
    {
        move.side[3] = right;
        if (right)
        {
            addMovement(1, 0, deltaTime);
            if (!weapon.isShooting)
                checkDirection(3);
        }

        checkStatus();
    }

    void Player::changeActionSprite(int x)
    {
        Sprites::get().player[x]->SetFrame(directionFacing * sspaths[x].columns);
        *sprite = *Sprites::get().player[x];
        currentSs.changeSpritesheet(sspaths[x].rows, sspaths[x].columns, directionFacing, sprite);
        currentSs.setFrameTime(sspaths[x].frameTime);
    }

    void Player::equipWeapon(int type) //type = 0 -> no weapon    5 - crossbow   6 - snowball  7 - snowman head
    {
        if (!isDead)
        {
            if (!type)
            {
                move.speed += 0.05f;
                currentRoom->speed += 0.05f;

                isHoldingGun = false;

                weapon.changeVisibility(false);

                checkStatus();
            }
            else if (type && !isHoldingGun)
            {
                move.speed -= 0.05f;
                currentRoom->speed -= 0.05f;
                isHoldingGun = true;

                weapon.changeVisibility(true);

                checkStatus();
            }
        }
    }

    void Player::shootProjectile(int type)
    {
        if (!isDead)
        {
            if (type)
            {
                equipWeapon(crossbow);
                weapon.shootArrows();
            }
            else
            {
                if (weapon.reloading < weapon.reloadTime / 2)
                    weapon.reloading = weapon.reloadTime / 2;
                weapon.stopShooting();
            }
        }
    }

    void Player::deleteExplosion(Weapon::IceExplosion* attack)
    {
        auto position = std::find(explosions.begin(), explosions.end(), attack);
        if (position != explosions.end())
            explosions.erase(position);
    }

    void Player::iceExplosion(int x, int y)
    {
        if (!isDead)
        {
            if (explosionTimeElapsed >= explosionTimer) //timer between each explosion
            {
                explosionTimeElapsed = 0;
                auto newAttack = new Weapon::IceExplosion{ x, y, newmath::isPositivef(potionTimers[damage]) * damageAdd, this };
                newAttack->init();
                explosions.push_back(newAttack);
            }
        }
    }

    void Player::addMovement(int x, int y, float deltaTime)
    {
        int xMap = 0, yMap = 0;
        locf.x += move.speed * deltaTime * x;
        locf.y += move.speed * deltaTime * y;

        collisionBox.setCollisionBox(static_cast<int>(locf.x) + collisionBox.offset.x, static_cast<int>(locf.y) + collisionBox.offset.y, 36, 36);

        int nextTile = CollisionCheck::isPlayerOverlapping(this, currentRoom);

        if (nextTile == nonCollide) //no collision
        {
            if ((x && newmath::inRangef(drawLocf.x, static_cast<float>(middleScreen.x) - 10, static_cast<float>(middleScreen.x) + 10)) || ((y && newmath::inRangef(drawLocf.y, static_cast<float>(middleScreen.y) - 10, static_cast<float>(middleScreen.y) + 10))))
                currentRoom->moveMap(x, y, deltaTime);
            drawLocf.x = locf.x - currentRoom->getLocation().x;
            drawLocf.y = locf.y - currentRoom->getLocation().y;
        }
        else if (nextTile == collide || nextTile == portalInactive)
        {
            locf.x -= move.speed * deltaTime * x;
            locf.y -= move.speed * deltaTime * y;

            collisionBox.setCollisionBox(static_cast<int>(locf.x) + collisionBox.offset.x, static_cast<int>(locf.y) + collisionBox.offset.y, 36, 36);
        }
        else if (nextTile == portalActive) // "teleport" player to the next room (kinda messy)
        {
            GameSpace::vec2 newRoomLocation;
            Map::Room* pastRoom = currentRoom;
            if (x > 0 && locf.x > (currentRoom->getSize().x - 4) * currentRoom->tilesize)
            {
                currentRoom = mapManager->switchRoom(currentRoom->getRoomNumber() % mapManager->getRoomAm().x + 1, currentRoom->getRoomNumber() / mapManager->getRoomAm().x);
                newRoomLocation = GameSpace::vec2(0, static_cast<float>((currentRoom->getSize().y / 2) * currentRoom->tilesize + currentRoom->tilesize / 2 - screen->GetHeight() / 2));
                currentRoom->setLocation(newRoomLocation);

                currentRoom->updateEnemies();

                locf.x = static_cast<float>(currentRoom->tilesize - sprite->GetWidth() / 2 + currentRoom->tilesize / 2) + 10;
                locf.y = static_cast<float>((currentRoom->getSize().y / 2) * currentRoom->tilesize - currentRoom->tilesize / 2);

                collisionBox.setCollisionBox(static_cast<int>(locf.x) + collisionBox.offset.x, static_cast<int>(locf.y) + collisionBox.offset.y, 36, 36);

                drawLocf.x = locf.x - currentRoom->getLocation().x;
                drawLocf.y = locf.y - currentRoom->getLocation().y;


                for (int i = 0; i < weapon.arrows.size(); i++)
                    weapon.arrows[i]->deleteArrow();
                weapon.arrows.clear();
            }
            else if (x < 0 && locf.x < 4 * currentRoom->tilesize)
            {
                currentRoom = mapManager->switchRoom(currentRoom->getRoomNumber() % mapManager->getRoomAm().x - 1, currentRoom->getRoomNumber() / mapManager->getRoomAm().x);
                newRoomLocation = GameSpace::vec2(static_cast<float>((currentRoom->getSize().x - screen->GetWidth() / currentRoom->tilesize) * currentRoom->tilesize), static_cast<float>((currentRoom->getSize().y / 2) * currentRoom->tilesize + currentRoom->tilesize / 2 - screen->GetHeight() / 2));
                currentRoom->setLocation(newRoomLocation);

                currentRoom->updateEnemies();

                locf.x = static_cast<float>((currentRoom->getSize().x - 1) * currentRoom->tilesize - sprite->GetWidth() / 2 - currentRoom->tilesize / 2);
                locf.y = static_cast<float>((currentRoom->getSize().y / 2) * currentRoom->tilesize - currentRoom->tilesize / 2);

                collisionBox.setCollisionBox(static_cast<int>(locf.x) + collisionBox.offset.x, static_cast<int>(locf.y) + collisionBox.offset.y, 36, 36);

                drawLocf.x = locf.x - currentRoom->getLocation().x;
                drawLocf.y = locf.y - currentRoom->getLocation().y;

                for (int i = 0; i < weapon.arrows.size(); i++)
                    weapon.arrows[i]->deleteArrow();
                weapon.arrows.clear();
            }
            else if (y > 0 && locf.y > (currentRoom->getSize().y - 4) * currentRoom->tilesize)
            {
                currentRoom = mapManager->switchRoom(currentRoom->getRoomNumber() % mapManager->getRoomAm().x, currentRoom->getRoomNumber() / mapManager->getRoomAm().x + 1);
                newRoomLocation = GameSpace::vec2(static_cast<float>((currentRoom->getSize().x / 2 - (currentRoom->getSize().x + 1) % 2) * currentRoom->tilesize + currentRoom->tilesize / 2 - screen->GetWidth() / 2), 0);
                currentRoom->setLocation(newRoomLocation);

                currentRoom->updateEnemies();

                locf.x = static_cast<float>((currentRoom->getSize().x / 2 - (currentRoom->getSize().x + 1) % 2) *
                    currentRoom->tilesize -
                    currentRoom->tilesize / 2);
                locf.y = static_cast<float>(2 * currentRoom->tilesize - sprite->GetHeight() / 2 + currentRoom->tilesize / 2) + 10;

                collisionBox.setCollisionBox(static_cast<int>(locf.x) + collisionBox.offset.x, static_cast<int>(locf.y) + collisionBox.offset.y, 36, 36);

                drawLocf.x = locf.x - currentRoom->getLocation().x;
                drawLocf.y = locf.y - currentRoom->getLocation().y;

                for (int i = 0; i < weapon.arrows.size(); i++)
                    weapon.arrows[i]->deleteArrow();
                weapon.arrows.clear();
            }
            else if (y < 0 && locf.y < 4 * currentRoom->tilesize)
            {
                currentRoom = mapManager->switchRoom(currentRoom->getRoomNumber() % mapManager->getRoomAm().x, currentRoom->getRoomNumber() / mapManager->getRoomAm().x - 1);
                newRoomLocation = GameSpace::vec2(static_cast<float>((currentRoom->getSize().x / 2 - (currentRoom->getSize().x + 1) % 2) * currentRoom->tilesize + currentRoom->tilesize / 2 - screen->GetWidth() / 2), static_cast<float>((currentRoom->getSize().y - screen->GetHeight() / currentRoom->tilesize) * currentRoom->tilesize));
                currentRoom->setLocation(newRoomLocation);

                currentRoom->updateEnemies();

                locf.x = static_cast<float>((currentRoom->getSize().x / 2 - (currentRoom->getSize().x + 1) % 2) * currentRoom->tilesize - currentRoom->tilesize / 2);
                locf.y = static_cast<float>((currentRoom->getSize().y - 1) * currentRoom->tilesize - sprite->GetHeight() / 2 - currentRoom-> tilesize / 2);

                collisionBox.setCollisionBox(static_cast<int>(locf.x) + collisionBox.offset.x, static_cast<int>(locf.y) + collisionBox.offset.y, 36, 36);

                drawLocf.x = locf.x - currentRoom->getLocation().x;
                drawLocf.y = locf.y - currentRoom->getLocation().y;

                for (int i = 0; i < weapon.arrows.size(); i++)
                    weapon.arrows[i]->deleteArrow();
                weapon.arrows.clear();
            }
            if (currentRoom->getRoomNumber() == mapManager->getFinish()) // if thew new room is the final one, apply win condition
            {
                currentSs.changeVisiblity(false);
                weapon.changeVisibility(false);
                won = true;
                currentRoom = pastRoom;
            }
            currentRoom->speed = move.speed;
        }
    }

    void Player::drawUI()
    {
        healthbar.drawHealthbar(health, screen);
        score.printScore(screen, screen->GetPitch() - screen->GetPitch() / 40, screen->GetHeight() / 45, points);
    }

    void Player::drawPausePlayer(float deltaTime)
    {
        drawUI();
        //order of sprites
        if (directionFacing == 0)
        {
            currentSs.drawNextSprite(0, screen, drawLocf);
            if (weapon.visible)
                weapon.drawWeapon(0);
            for (int i = 0; i < weapon.arrows.size(); i++)
                weapon.arrows[i]->drawProjectile(screen, 0);
            for (int i = 0; i < explosions.size(); i++)
                explosions[i]->update(0);
        }
        else
        {
            if (weapon.visible)
                weapon.drawWeapon(0);
            for (int i = 0; i < weapon.arrows.size(); i++)
                weapon.arrows[i]->drawProjectile(screen, 0);
            currentSs.drawNextSprite(0, screen, drawLocf);
            for (int i = 0; i < explosions.size(); i++)
                explosions[i]->update(0);
        }
    }

    void Player::update(float deltaTime)
    {
        checkPotions(deltaTime);
        CollisionCheck::campfireHeal(this, currentRoom);
        explosionTimeElapsed = newmath::clampf(explosionTimeElapsed + deltaTime, 0, explosionTimer + 1);
        drawUI();
        if (directionFacing == 0)
        {
            currentSs.drawNextSprite(deltaTime, screen, drawLocf);
            if (potionTimers[shield] || isShieldCreating) //update shield
            {
                shieldSs.drawNextSprite(deltaTime, screen, drawLocf);
                if (shieldSs.getCurrentFrame() == 10)
                {
                    shieldSs.freezeFrame(10, true);
                    isShieldCreating = 2;
                }
                else if (shieldSs.getCurrentFrame() == 21)
                {
                    shieldSs.setDirection(0);
                    shieldSs.freezeFrame(0, true);
                    isShieldCreating = 0;
                    potionTimers[shield] = 0;
                }
            }
            if (weapon.visible)
                weapon.update(deltaTime);
            for (int i = 0; i < weapon.arrows.size(); i++)
                weapon.arrows[i]->UpdatePosition(deltaTime);
            for (int i = 0; i < explosions.size(); i++)
                explosions[i]->update(deltaTime);
        }
        else
        {
            if (weapon.visible)
                weapon.update(deltaTime);
            for (int i = 0; i < weapon.arrows.size(); i++)
                weapon.arrows[i]->UpdatePosition(deltaTime);
            currentSs.drawNextSprite(deltaTime, screen, drawLocf);
            if (potionTimers[shield] || isShieldCreating) //update shield
            {
                shieldSs.drawNextSprite(deltaTime, screen, drawLocf);
                if (shieldSs.getCurrentFrame() == 10)
                {
                    shieldSs.freezeFrame(10, true);
                    isShieldCreating = 2;
                }
                else if (shieldSs.getCurrentFrame() == 21)
                {
                    shieldSs.setDirection(0);
                    shieldSs.freezeFrame(0, true);
                    isShieldCreating = 0;
                    potionTimers[shield] = 0;
                }
            }
            for (int i = 0; i < explosions.size(); i++)
                explosions[i]->update(deltaTime);
        }
    }
}
