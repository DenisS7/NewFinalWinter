#include "Gift.h"
#include "Potion.h"
#include "Room.h"

namespace Item
{
    void Gift::init()
    {
        drawLocf = locf - currentRoom->getLocation();
    }

    void Gift::open()
    {
        //spawn a potion from the gift
        currentRoom->addItem(potion, locf);
        currentRoom->removeItem(this);
        delete this;
    }

    void Gift::takeDamage(int damage)
    {
        health -= damage;
        if (health <= 0)
            open(), isColidable = false;
    }
}
