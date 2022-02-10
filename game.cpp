#include "game.h"
#include "Classes/Room.h"
#include <iostream>

#include "MapManager.h"
#include "Classes/Player.h"

#include "Classes/Spritesheet.h"
#include "Classes/Other.h"



namespace GameSpace
{
	// -----------------------------------------------------------
	// Initialize the application
	// -----------------------------------------------------------

	Map::MapManager manager;
	Character::Player player;
	const Uint8* keystate = SDL_GetKeyboardState(NULL);
	//Spritesheet test("assets/Player/player_run.png", 4, 8);
	//Sprite test(new Surface("assets/Player/player_run.png"), 8);


	void Game::Init()
	{

		vec2 s;
		//test2->LoadImage("assets/Player/player_run.png");
		//test.Init("assets/Player/player_run.png", 4, 8);
		time_t t;
		srand((unsigned)time(&t));
		manager.Initiate();
		manager.GenerateFirstRoom();

		player.Init(screen, &manager.rooms[manager.start.x + manager.start.y * manager.roomAm.x], &manager);

		manager.setPlayer(&player);
		
		/*std::cout << "PR " << "GO " << "D " << "L " << "U " << "R " << std::endl;

		for (int i = 0; i < manager.actualRooms; i++)
		{
			std::cout << manager.parentRoom[manager.generatedOrder[i]] << " " << manager.generatedOrder[i] << " ";
			for (int j = 0; j < 4; j++)
				std::cout << manager.rooms[manager.generatedOrder[i]].doors[j] << " ";
			std::cout << std::endl;
		}*/

		player.equipWeapon(5);

		//for (int i = 0; i < manager.roomAm.x * manager.roomAm.y; i++, std::cout << std::endl)
			//for (int j = 0; j < manager.roomAm.x * manager.roomAm.y; j++)
				//std::cout << manager.graph[i][j] << " ";

		//manager.rooms[46].DrawTile(0, 1, screen, 132, 96, 0);

	}

	void Game::Input()
	{
		player.moveDown(keystate[SDL_SCANCODE_DOWN]);
		player.moveLeft(keystate[SDL_SCANCODE_LEFT]);
		player.moveUp(keystate[SDL_SCANCODE_UP]);
		player.moveRight(keystate[SDL_SCANCODE_RIGHT]);
	}

	void Game::KeyDown(int key)
	{
		int x;
		if (player.isHoldingGun)
			x = 0;
		else x = 5;
		switch (key)
		{
		case SDL_SCANCODE_E:
			player.equipWeapon(x);
			break;
		default:
			break;
		}
	}



	// -----------------------------------------------------------
	// Close down application
	// -----------------------------------------------------------
	void Game::Shutdown()
	{
	}



	// -----------------------------------------------------------
	// Main application tick function
	// -----------------------------------------------------------



	void Game::Tick(float deltaTime)
	{
		//keystate = SDL_GetKeyboardState(NULL);
		Input();



		
		manager.rooms[player.currentRoom->roomNumber].DrawMap(screen);
		player.Update(deltaTime);
	}
};