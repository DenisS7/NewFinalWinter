#include "Room.h"
#include "../template.h"
#include "../surface.h"
#include <fstream>

#include "newmath.h"
//#include <algorithm>
#include <vector>
#include "../MapManager.h"
#include "Player.h"
#include "EnemyBase.h"
#include "enemy_metalbox.h"
#include "enemy_snowman.h"
#include "enemy_rager.h"
#include "Sprites.h"

namespace Map
{
    void Room::restart()
    {
        //restart parameters
        locf = 0;
        level = 0;

        while (enemiesInRoom.size())
        {
            enemiesInRoom[0]->die();
        }
        enemiesInRoom.clear();
        resetDoors();
        enemies = 0;
        tiles.clear();
        nrdoors = 1;
        for (int i = 0; i < itemsInRoom.size();)
            itemsInRoom[i]->deleteItem();
    }

    void Room::addItem(int type, GameSpace::vec2 locf)
    {
        //add new item to the room
        if (type == gift)
        {
            auto newGift = new Item::Gift(this, gift, screen, locf);
            itemsInRoom.push_back(newGift);
            newGift->init();
        }
        else if (type == potion)
        {
            int potionType = IRand(5);
            auto newPotion = new Item::Potion(this, potion, screen, potionType, locf);
            itemsInRoom.push_back(newPotion);
            newPotion->init();
        }
    }

    void Room::removeItem(Item::ItemBase* item)
    {
        //delete item from room
        auto position = std::find(itemsInRoom.begin(), itemsInRoom.end(), item);
        if (position != itemsInRoom.end())
            itemsInRoom.erase(position);
    }

    void Room::spawnGifts()
    {
        //spawn a random amount of gifts between 1 and 3
        int gifts = IRand(3) + 1;
        for (int i = 0; i < gifts; i++)
        {
            GameSpace::vec2 giftLocf;
            //make sure is inside the walls
            giftLocf.x = static_cast<float>(IRand((size.x - 6) * tilesize)) + 3 * tilesize;
            giftLocf.y = static_cast<float>(IRand((size.y - 6) * tilesize)) + 3 * tilesize;
            addItem(gift, giftLocf);
        }
    }

    void Room::readRoomLayout(const std::vector<int> collisionTiles, const std::vector<int> portalTiles)
    {
        std::ifstream fin("ReadFiles/RoomLayout/Room2.txt");

        fin >> size.x >> size.y;
        roomSize = size.x * size.y;

        for (int i = 0; i < roomSize; i++)
        {
            int x;
            fin >> x;
            //add collisions to walls and portals
            if (std::find(collisionTiles.begin(), collisionTiles.end(), x) != collisionTiles.end())
                tiles.push_back(makeTile(x, collide, 0, true));
            else tiles.push_back(makeTile(x, nonCollide, 0, false));
        }

        fin.close();
    }

    void Room::initiateRoom(const int number, MapManager* newManager, GameSpace::Surface* newScreen)
    {
        roomNumber = number;
        type = Fight;
        level = 2;
       
        manager = newManager;
        player = newManager->player;
        screen = newScreen;
        
        //chance of spawning campfire in the middle of the room
        int campfireChance = IRand(5);
        if (!campfireChance)
        {
            GameSpace::vec2 campfirePos(static_cast<float>(size.x * tilesize) / 2,
                                        static_cast<float>((size.y - 1) * tilesize) / 2);
            auto newCampfire = new Item::Campfire(this, campfire, screen, campfirePos);
            newCampfire->init();
            itemsInRoom.push_back(newCampfire);
        }
        spawnGifts();
        tilesPerRow = Sprites::get().tilemap.GetPitch() / tilesize;
    }

    void Room::initiateEnemies()
    {
        //nr of enemies in room
        enemies = 13;
        for (int i = 0; i < enemies; i++)
        {
            //random type of enemy  0 - metalbox, 1 - snowman, 2 - rager
            int enemyType = IRand(3);
            if (enemyType == 0)
            {
                auto newEnemy = new Character::enemy_metalbox(this, 0);
                newEnemy->init();
                enemiesInRoom.push_back(newEnemy);
            }
            else if (enemyType == 1)
            {
                auto newEnemy = new Character::enemy_snowman(this, 1);
                newEnemy->init();
                enemiesInRoom.push_back(newEnemy);
            }
            else if (enemyType == 2)
            {
                auto newEnemy = new Character::enemy_rager(this, 2);
                newEnemy->init();
                enemiesInRoom.push_back(newEnemy);
            }
        }
    }

    void Room::removeEnemyFromTile(const Character::EnemyBase* enemy, int tileNr)
    {
        //remove the enemy to the tile's containing enemies
        tileNr = newmath::clamp(tileNr, 0, roomSize - 1);
        auto position = tiles[tileNr].enemiesOnTile.end();
        if (std::count(tiles[tileNr].enemiesOnTile.begin(), tiles[tileNr].enemiesOnTile.end(), enemy))
            position = std::find(tiles[tileNr].enemiesOnTile.begin(), tiles[tileNr].enemiesOnTile.end(), enemy);
        if (position != tiles[tileNr].enemiesOnTile.end())
            tiles[tileNr].enemiesOnTile.erase(position);
    }

    void Room::addEnemyToTile(Character::EnemyBase* enemy, int tileNr)
    {
        //add the enemy to the tile's containing enemies
        if (!std::count(tiles[tileNr].enemiesOnTile.begin(), tiles[tileNr].enemiesOnTile.end(), enemy))
            tiles[tileNr].enemiesOnTile.push_back(enemy);
    }

    void Room::enemyOnTiles(Character::EnemyBase* enemy)
    {
        //update tiles that contain the enemy
        int x = static_cast<int>(enemy->locf.x) / tilesize;
        int y = static_cast<int>(enemy->locf.y) / tilesize;
        newmath::ivec2 initTile = newmath::make_ivec2(x, y);

        x = (static_cast<int>(enemy->locf.x) + enemy->sprite->GetWidth()) / tilesize;
        y = (static_cast<int>(enemy->locf.y) + enemy->sprite->GetHeight()) / tilesize;
        newmath::ivec2 finTile = newmath::make_ivec2(x, y);

        newmath::ivec2 dif = initTile - enemy->initOcupTile;
        if (dif.x < 0)
        {
            for (int x = 0; x > dif.x; x--)
            {
                for (int y = enemy->initOcupTile.y; y <= enemy->finOcupTile.y && y < size.y; y++)
                {
                    const int removex = enemy->finOcupTile.x - x;
                    const int addx = enemy->initOcupTile.x - x;

                    newmath::clamp(removex, 0, size.x - 1);
                    newmath::clamp(addx, 0, size.x - 1);

                    const int tileRemNr = removex + y * size.x;
                    const int tileAddNr = addx + y * size.x;

                    removeEnemyFromTile(enemy, tileRemNr);
                    addEnemyToTile(enemy, tileAddNr);
                }
            }
        }
        else if (dif.x > 0)
        {
            for (int x = 0; x < dif.x; x++)
            {
                for (int y = enemy->initOcupTile.y; y <= enemy->finOcupTile.y && y < size.y; y++)
                {
                    const int removex = enemy->initOcupTile.x + x;
                    const int addx = enemy->finOcupTile.x + x;

                    newmath::clamp(removex, 0, size.x - 1);
                    newmath::clamp(addx, 0, size.x - 1);

                    const int tileRemNr = removex + y * size.x;
                    const int tileAddNr = addx + y * size.x;

                    removeEnemyFromTile(enemy, tileRemNr);
                    addEnemyToTile(enemy, tileAddNr);
                }
            }
        }

        if (dif.y < 0)
        {
            for (int y = 0; y > dif.y; y--)
            {
                for (int x = enemy->initOcupTile.x; x <= enemy->finOcupTile.x && x < size.x; x++)
                {
                    const int removey = enemy->finOcupTile.y - y;
                    const int addy = enemy->initOcupTile.y - y;

                    newmath::clamp(removey, 0, size.y - 1);
                    newmath::clamp(addy, 0, size.y - 1);

                    const int tileRemNr = x + removey * size.x;
                    const int tileAddNr = x + addy * size.x;

                    removeEnemyFromTile(enemy, tileRemNr);
                    addEnemyToTile(enemy, tileAddNr);
                }
            }
        }
        else if (dif.y > 0)
        {
            for (int y = 0; y < dif.y; y++)
            {
                for (int x = enemy->initOcupTile.x; x <= enemy->finOcupTile.x && x < size.x; x++)
                {
                    const int removey = enemy->initOcupTile.y + y;
                    const int addy = enemy->finOcupTile.y + y;

                    newmath::clamp(removey, 0, size.y - 1);
                    newmath::clamp(addy, 0, size.y - 1);

                    const int tileRemNr = x + removey * size.x;
                    const int tileAddNr = x + addy * size.x;

                    removeEnemyFromTile(enemy, tileRemNr);
                    addEnemyToTile(enemy, tileAddNr);
                }
            }
        }

        enemy->initOcupTile = initTile;
        enemy->finOcupTile = finTile;
    }

    void Room::openPortals()
    {
        //turn on the campfire
        if (itemsInRoom.size())
            if (itemsInRoom[0]->getType() == campfire)
            {
                auto campfireP = dynamic_cast<Item::Campfire*>(itemsInRoom[0]);
                campfireP->turnOn();
            }
        //change the portal tile from closed to open
        changeDoorLayout(true);

        //make the portal leading to the exit green
        if (manager->isLastRoom(roomNumber) == 1)
        {
            type = portalActive;
            int door[2] = {manager->finalDoor[0], manager->finalDoor[1]};

            if (roomNumber - manager->getFinish() == -manager->getRoomAm().x) // final room is down
            {
                tiles[size.x / 2 - 1 + (size.y - 1) * size.x].drawIndex = door[0];
                tiles[size.x / 2 - 1 + (size.y - 2) * size.x].drawIndex = door[1];

                tiles[size.x / 2 - 1 + (size.y - 1) * size.x].type = type;
                tiles[size.x / 2 - 1 + (size.y - 1) * size.x].colidable = true;

                tiles[size.x / 2 - 1 + (size.y - 1) * size.x].rotate = tiles[size.x / 2 - 1 + (size.y - 2) * size.x].
                    rotate = 2;
            }
            else if (roomNumber - manager->getFinish() == 1) //final room is left
            {
                tiles[(size.y / 2) * size.x].drawIndex = door[0];
                tiles[(size.y / 2) * size.x + 1].drawIndex = door[1];

                tiles[(size.y / 2) * size.x].type = type;
                tiles[(size.y / 2) * size.x].colidable = true;

                tiles[(size.y / 2) * size.x].rotate = tiles[(size.y / 2) * size.x + 1].rotate = 1;
            }
            else if (roomNumber - manager->getFinish() == manager->getRoomAm().x) //final room is up
            {
                tiles[size.x / 2 - 1 + size.x].drawIndex = door[0];
                tiles[size.x / 2 - 1 + 2 * size.x].drawIndex = door[1];

                tiles[size.x / 2 - 1 + size.x].type = type;
                tiles[size.x / 2 - 1 + size.x].colidable = true;
            }
            else if (roomNumber - manager->getFinish() == -1) //final room is right
            {
                tiles[(size.y / 2) * size.x + size.x - 1].drawIndex = door[0];
                tiles[(size.y / 2) * size.x + size.x - 2].drawIndex = door[1];

                tiles[(size.y / 2) * size.x + size.x - 1].type = type;
                tiles[(size.y / 2) * size.x + size.x - 1].colidable = true;

                tiles[(size.y / 2) * size.x + size.x - 1].rotate = tiles[(size.y / 2) * size.x + size.x - 2].rotate = 3;
            }
        }
    }

    void Room::hideEnemy(Character::EnemyBase* enemy)
    {
        //enemy died but still has projectiles in the world (only applicable to snowman)
        enemies--;
        player->modifyPoints(enemy->getData().points);
        if (enemies == 0)
        {
            player->modifyPoints(100);
            openPortals();
        }
    }

    void Room::removeEnemy(Character::EnemyBase* enemy)
    {
        //enemy died but still had projectiles in the world (only applicable to snowman)
        //deletes it
        auto position = std::find(enemiesInRoom.begin(), enemiesInRoom.end(), enemy);
        if (position != enemiesInRoom.end())
            enemiesInRoom.erase(position);
    }

    void Room::deleteEnemy(Character::EnemyBase* enemy)
    {
        //delete the enemy and check if it was the last one
        removeEnemy(enemy);
        enemies--;
        player->modifyPoints(enemy->getData().points);
        if (enemies == 0)
        {
            player->modifyPoints(100);
            openPortals();
        }
    }

    void Room::moveMap(int x, int y, float deltaTime)
    {
        locf.x += speed * x * deltaTime;
        locf.y += speed * y * deltaTime;

        locf.x = newmath::clampf(locf.x, 0.0, static_cast<float>(size.x) * tilesize - ScreenWidth);
        locf.y = newmath::clampf(locf.y, 0.0, static_cast<float>(size.y) * tilesize - ScreenHeight);
    }

    void Room::changeDoorLayout(bool isOpen)
    {
        //add portals or open portals
        int door[2] = {40, 67};
        int type = portalInactive;
        isRoomOpen = isOpen;
        if (isOpen)
        {
            door[0] = manager->openDoor[0];
            door[1] = manager->openDoor[1];
            type = portalActive;
        }
        else
        {
            door[0] = manager->closedDoor[0];
            door[1] = manager->closedDoor[1];
            type = portalInactive;
        }

        if (doors[0]) //down
        {
            tiles[size.x / 2 - 1 + (size.y - 1) * size.x].drawIndex = door[0];
            tiles[size.x / 2 - 1 + (size.y - 2) * size.x].drawIndex = door[1];

            tiles[size.x / 2 - 1 + (size.y - 1) * size.x].type = type;
            tiles[size.x / 2 - 1 + (size.y - 1) * size.x].colidable = true;

            tiles[size.x / 2 - 1 + (size.y - 1) * size.x].rotate = tiles[size.x / 2 - 1 + (size.y - 2) * size.x].rotate
                = 2;
        }

        if (doors[2]) // up
        {
            tiles[size.x / 2 - 1 + size.x].drawIndex = door[0];
            tiles[size.x / 2 - 1 + 2 * size.x].drawIndex = door[1];

            tiles[size.x / 2 - 1 + size.x].type = type;
            tiles[size.x / 2 - 1 + size.x].colidable = true;
        }

        if (doors[1]) //left
        {
            tiles[(size.y / 2) * size.x].drawIndex = door[0];
            tiles[(size.y / 2) * size.x + 1].drawIndex = door[1];

            tiles[(size.y / 2) * size.x].type = type;
            tiles[(size.y / 2) * size.x].colidable = true;

            tiles[(size.y / 2) * size.x].rotate = tiles[(size.y / 2) * size.x + 1].rotate = 1;
        }

        if (doors[3]) //right
        {
            tiles[(size.y / 2) * size.x + size.x - 1].drawIndex = door[0];
            tiles[(size.y / 2) * size.x + size.x - 2].drawIndex = door[1];

            tiles[(size.y / 2) * size.x + size.x - 1].type = type;
            tiles[(size.y / 2) * size.x + size.x - 1].colidable = true;

            tiles[(size.y / 2) * size.x + size.x - 1].rotate = tiles[(size.y / 2) * size.x + size.x - 2].rotate = 3;
        }
    }

    void Room::calculateDoors(int startDoor, bool CanClose)
    {
        if (!doors[startDoor])
        {
            nrdoors++;
            doors[startDoor] = true;
        }
        nrdoors += IRand(5 - nrdoors);
        if (nrdoors == 1 && CanClose == false)
            nrdoors = 2;
        for (int i = 0; i < nrdoors; i++)
        {
            int k = 0;
            int n = IRand(4);
            // try to add a new door if there's one in the current position
            while (doors[n] && k < 4)
                n = (n + 1) % 4, k++;
            doors[n] = true;
        }
    }

    void Room::resetDoors()
    {
        nrdoors = 1;
        for (int i = 0; i < 4; i++)
            doors[i] = false;
    }

    int Room::doorNumber()
    {
        int k = 0;
        for (int i = 0; i < 4; i++)
            if (doors[i])
                k++;
        return k;
    }

    int Room::checkCollision(int x, int y)
    {
        //check tile collision
        int xTile = x;
        int yTile = y;
        if (tiles[xTile + yTile * size.x].type == collide)
            return collide;
        if (tiles[xTile + yTile * size.x].type == portalActive)
            return portalActive;
        return 0;
    }

    void Room::drawRotatedTile(int tx, int ty, int dx, int dy, int rotate)
    {
        //draw rotated tile (only applicable to portals currently)
        newmath::ivec2 rot, loop, add;
        rot.x = rot.y = 0;
        loop.x = loop.y = 0;
        add.x = add.y = 0;
        int xRot = 0, yRot = 0;
        int xLoop = 1, yLoop = 0;
        int xAdd = 0, yAdd = 1;
        if (rotate == 1)
        {
            rot.x = 31;
            loop.x = 0;
            loop.y = 1;
            add.x = -1;
            add.y = 0;
        }
        else if (rotate == 2)
        {
            rot.x = rot.y = 31;
            loop.x = -1;
            loop.y = 0;
            add.x = 0;
            add.y = -1;
        }
        else if (rotate == 3)
        {
            rot.y = 31;
            loop.x = 0;
            loop.y = -1;
            add.x = 1;
            add.y = 0;
        }

        GameSpace::Pixel* src = Sprites::get().tilemap.GetBuffer() + tx * tilesize + ty * tilesize * Sprites::get().
            tilemap.GetPitch();
        GameSpace::Pixel* dst = screen->GetBuffer() + dx - offset.x + (dy - offset.y) * screen->GetPitch();

        int yStartOffset = dy - offset.y;

        while (yStartOffset < 0)
        {
            yStartOffset++;
            rot.y += add.y;
            rot.x += add.x;
            dst += screen->GetPitch();
        }

        for (int i = 0; i < tilesize && yStartOffset < screen->GetHeight(); i++, yStartOffset++)
        {
            newmath::ivec2 rem;
            rem.x = rot.x;
            rem.y = rot.y;
            for (int j = 0; j < tilesize; j++, rem.x += loop.x, rem.y += loop.y)
                if (j + dx >= offset.x && j + dx < screen->GetPitch() + offset.x)
                    dst[j] = src[rem.x + rem.y * Sprites::get().tilemap.GetPitch()];
            rot.x += add.x;
            rot.y += add.y;

            dst += screen->GetPitch();
        }
    }

    void Room::drawTile(int tx, int ty, int dx, int dy)
    {
        GameSpace::Pixel* src = Sprites::get().tilemap.GetBuffer() + tx * tilesize + ty * tilesize * Sprites::get().
            tilemap.GetPitch();
        GameSpace::Pixel* dst = screen->GetBuffer() + dx - offset.x + (dy - offset.y) * screen->GetPitch();

        int yStartOffset = dy - offset.y;

        while (yStartOffset < 0)
        {
            yStartOffset++;
            src += Sprites::get().tilemap.GetPitch();
            dst += screen->GetPitch();
        }

        for (int i = 0; i < tilesize && yStartOffset < screen->GetHeight(); i++, yStartOffset++)
        {
            for (int j = 0; j < tilesize; j++)
                if (j + dx >= offset.x && j + dx < screen->GetPitch() + offset.x)
                    dst[j] = src[j];
            src += Sprites::get().tilemap.GetPitch(), dst += screen->GetPitch();
        }
    }

    void Room::drawSpriteTile(int tx, int ty, int dx, int dy)
    {
        //not used, but will be for decorations of the room in the future (trees, rocks etc.)
        GameSpace::Pixel* src = Sprites::get().tilemap.GetBuffer() + tx * tilesize + ty * tilesize * Sprites::get().
            tilemap.GetPitch();
        GameSpace::Pixel* dst = screen->GetBuffer() + dx - offset.x + (dy - offset.y) * screen->GetPitch();

        int yStartOffset = dy - offset.y;

        while (yStartOffset < 0)
        {
            yStartOffset++;
            src += Sprites::get().tilemap.GetPitch();
            dst += screen->GetPitch();
        }

        for (int i = 0; i < tilesize && yStartOffset < screen->GetHeight(); i++, yStartOffset++)
        {
            for (int j = 0; j < tilesize; j++)
                if (j + dx >= offset.x && j + dx < screen->GetPitch() + offset.x && src[j])
                    dst[j] = src[j];
            src += Sprites::get().tilemap.GetPitch(), dst += screen->GetPitch();
        }
    }

    void Room::drawMap()
    {
        offset.x = static_cast<int>(locf.x) % tilesize;
        offset.y = static_cast<int>(locf.y) % tilesize;

        newmath::ivec2 start;

        start.x = static_cast<int>(locf.x) / tilesize;
        start.y = static_cast<int>(locf.y) / tilesize;

        if (start.x < 0)
            start.x = 0;

        if (start.y < 0)
            start.y = 0;

        for (int y = start.y; y <= screen->GetHeight() / tilesize + start.y && y < size.y; y++)
        {
            for (int x = start.x; x <= screen->GetWidth() / tilesize + start.x && x < size.x; x++)
            {
                int tx = (tiles[y * size.x + x].drawIndex - 1) % tilesPerRow;
                int ty = (tiles[y * size.x + x].drawIndex - 1) / tilesPerRow;

                int xDrawLoc = (x - start.x) * tilesize;
                int yDrawLoc = (y - start.y) * tilesize;

                if (!tiles[y * size.x + x].rotate)
                {
                    drawTile(tx, ty, xDrawLoc, yDrawLoc);
                }
                else
                {
                    drawRotatedTile(tx, ty, xDrawLoc, yDrawLoc, tiles[y * size.x + x].rotate);
                }
            }
        }
    }

    void Room::updateEnemies()
    {
        for (int i = 0; i < enemiesInRoom.size(); i++)
        {
            //A star algorithm to the player
            enemiesInRoom[i]->findPath (enemiesInRoom[i]->getCurrentPos(newmath::make_ivec2(enemiesInRoom[i]->sprite->GetWidth() / 2, enemiesInRoom[i]->sprite->GetHeight() / 2)), player->getCurrentPos(), this);
            enemiesInRoom[i]->changeDrawLoc();
        }
    }

    void Room::updateTiles()
    {
        //update enemies positions and tiles containing them
        for (int i = 0; i < enemiesInRoom.size(); i++)
            enemyOnTiles(enemiesInRoom[i]);
    }

    void Room::drawEnemies(float deltaTime)
    {
        for (int i = 0; i < enemiesInRoom.size(); i++)
        {
            if (enemiesInRoom[i])
                enemiesInRoom[i]->update(deltaTime);
        }
    }

    void Room::drawItems(float deltaTime)
    {
        for (int i = 0; i < itemsInRoom.size(); i++)
            itemsInRoom[i]->update(deltaTime);
    }

    void Room::updateMap(float deltaTime)
    {
        for (int i = 0; i < manager->getRoomAm().x * manager->getRoomAm().y; i++)
        {
            if (manager->rooms[i].getIsRoomOpen() && i != roomNumber)
            {
                for (int j = 0; j < manager->rooms[i].getEnemies().size(); j++)
                {
                    if (manager->rooms[i].getEnemies()[j]->type == 1)
                    {
                        auto snowman = dynamic_cast<Character::enemy_snowman*>(manager->rooms[i].getEnemies()[j]);
                        for (int e = 0; e < snowman->getSnowballs().size(); e++)
                        {
                            snowman->getSnowballs()[e]->Move(deltaTime);
                        }
                    }
                }
            }
        }
        drawMap();
        drawItems(deltaTime);
        drawEnemies(deltaTime);
        updateTiles();
    }
}
