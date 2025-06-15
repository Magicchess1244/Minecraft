#include "GameClient.h"
#include "ChunkManager.h"
#include "PerlinNoise.h"

void GameClient::set_seed() {
    int res;
    char buf[8];
    unsigned int server_seed = 0;

    res = send(this->server, "get_seed", sizeof("get_seed"), 0);
    if (res <= 0) std::cerr << "Error requesting seed from the server.";
    res = recv(this->server, buf, sizeof(buf), 0);
    if (res <= 0) std::cerr << "Error receiving seed from the server.";
    server_seed = static_cast<unsigned int>(std::stoi(std::string(buf)));
    std::cout << "New client seed: " << server_seed << std::endl;
    srand(server_seed);
    this->seed = server_seed;
	SetGradients();
}

//UI function
namespace UI {
	int FindSlot(std::vector<Slot>& Inventory, short Type) {
		int index = 0;
		for (Slot slot : Inventory) {
			index++;
			if ((slot.Type == Type || slot.Type == 0) && slot.Amount < 64) {
				slot.Type = Type;
				return index;
			}
		}
		std::cout << "Inventory full, cannot add item of type: " << Type << std::endl;
	}
	void Input(Vector2& PlayerDirection, bool OnGround, int& InventorySlots, Vector2& PlayerPos, Vector2 Range) {
		const bool* KeyboardState = SDL_GetKeyboardState(NULL);
		const bool move_up = (KeyboardState[SDL_SCANCODE_W] ||
			KeyboardState[SDL_SCANCODE_UP] ||
			KeyboardState[SDL_SCANCODE_SPACE]);
		const bool move_left = KeyboardState[SDL_SCANCODE_A] || KeyboardState[SDL_SCANCODE_LEFT];
		const bool move_right = KeyboardState[SDL_SCANCODE_D] || KeyboardState[SDL_SCANCODE_RIGHT];

		PlayerDirection.x = 0;

		if (move_left || move_right) {
			PlayerDirection.x = move_left ? -1 : 1;
		}

		if (move_up && Collition(PlayerPos, { 0, -1 }, Range.x, Range.y, true, false)) {
			PlayerDirection.y = 1;
		}

		for (int i = 0; i < 8; ++i) {
			if (KeyboardState[SDL_SCANCODE_1 + i]) {
				InventorySlots = i;
				break;
			}
		}
	}
	void DrawBG(SDL_Renderer* Renderer, Vector2& PlayerPos, Vector2 Range) {

		int CurrentChunck = (int)floorf((PlayerPos.x) / 16);
		int RelativeX = (int)(PlayerPos.x) % 16;
		int xRange = Range.x - RelativeX;
		Mesh mesh = { 0 };
		ChunkGenerator(CurrentChunck);
		if (PlayerPos.y == 1000) {
			PlayerPos.y = (float)GetHeight(CurrentChunck);
			//std::cout << "PlayerPos: " << PlayerPos.x << ", " << PlayerPos.y << std::endl;
		}

		//std::cout << "PlayerPos: " << PlayerPos.x << ", " << PlayerPos.y << std::endl;
		//std::cout << "CurrentChunck: " << CurrentChunck << std::endl;

		DrawChunk(CurrentChunck, RelativeX, (int)PlayerPos.y, xRange, Range.y, Range.x, mesh, true);
		SDL_RenderGeometry(Renderer, nullptr, mesh.Vertices, xRange * Range.y * 4, mesh.Indices, xRange * Range.y * 6);
		//PrintChunk(CurrentChunck, 0, 0, xRange, 64, FullRange);


		if (RelativeX > 0)
		{
			ChunkGenerator(CurrentChunck + 1);
			xRange = RelativeX;
			RelativeX = 0;
			CurrentChunck++;

			DrawChunk(CurrentChunck, RelativeX, (int)PlayerPos.y, xRange, Range.y, Range.x, mesh, false);
			SDL_RenderGeometry(Renderer, nullptr, mesh.Vertices, xRange * Range.y * 4, mesh.Indices, xRange * Range.y * 6);
			//PrintChunk(CurrentChunck, RelativeX, PlayerPos.y, xRange, yRange, FullRange);
		}

	}
	void DrawPlayer(SDL_Renderer* Renderer, Vector2 Range, std::vector<Players>& PlayerPos)
	{
		//Other player
		for (Players Position : PlayerPos)
		{
			int dx = (int)(Position.Position.x - PlayerPos[0].Position.x);
			int dy = (int)(Position.Position.y - PlayerPos[0].Position.y);

			bool InsideX = std::abs(dx) <= (Range.x / 2);
			if (InsideX) {
				int RelativeX = (Range.x / 2 - 1 + dx) * BlockSize;
				int RelativeY = (Range.y / 2 - 2 - dy) * BlockSize;

				SDL_FRect OtherPlayerRect = {
					(float)RelativeX,
					(float)RelativeY,
					(float)BlockSize,
					(float)BlockSize * 2
				};
				SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].Color.r, 0, 255), SDL_clamp(PlayerPos[0].Color.g, 0, 255), SDL_clamp(PlayerPos[0].Color.b, 0, 255), PlayerPos[0].Color.a);
				SDL_RenderFillRect(Renderer, &OtherPlayerRect);
			}
		}

		// Your player
		SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].Color.r, 0, 255), SDL_clamp(PlayerPos[0].Color.g, 0, 255), SDL_clamp(PlayerPos[0].Color.b, 0, 255), PlayerPos[0].Color.a);
		SDL_FRect PlayerRect = {
			(float)(Range.x / 2 - 1) * BlockSize,
			(float)(Range.y / 2 - 2) * BlockSize,
			(float)BlockSize,
			(float)BlockSize * 2
		};
		SDL_RenderFillRect(Renderer, &PlayerRect);

		SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].Color.r + 90, 0, 255), SDL_clamp(PlayerPos[0].Color.g + 90, 0, 255), SDL_clamp(PlayerPos[0].Color.b + 90, 0, 255), PlayerPos[0].Color.a);
		SDL_FRect InsidePlayerRect = {
			(float)(Range.x / 2 - 1) * BlockSize + (BlockSize * 0.1f),
			(float)(Range.y / 2 - 2) * BlockSize + (BlockSize * 0.1f),
			(float)(BlockSize * 0.8f),
			(float)(BlockSize * 0.9f) * 2
		};
		SDL_RenderFillRect(Renderer, &InsidePlayerRect);
	}
	int Ui(SOCKET serverSocket, Vector2 Range, bool& Running, std::vector<Players>& PlayerPos)
	{
		int width = 600;
		int height = 400;

		Size(width, height, Range.y, Range.x);

		if (!SDL_Init(SDL_INIT_VIDEO)) {
			std::cout << "Error initializing SDL: " << SDL_GetError();
			return 1;
		}
		else {
			std::cout << "SDL initialized successfully." << std::endl;
		}

		SDL_Window* Window = SDL_CreateWindow("Bit Miner", width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);// | SDL_WINDOW_MAXIMIZED);
		if (Window == NULL) {
			std::cout << "Error creating window: " << SDL_GetError();
			SDL_Quit();
			return 1;
		}

		SDL_Renderer* Renderer = SDL_CreateRenderer(Window, NULL);
		if (Renderer == NULL) {
			std::cout << "Error creating renderer: " << SDL_GetError();
			SDL_DestroyWindow(Window);
			SDL_Quit();
			return 1;
		}

		bool FullScreen = false;
		Vector2 PlayerDirection = { 0, 0 };


		SDL_Event Event;
		std::vector<Slot> Inventory;
		Inventory.resize(8);
		Inventory.clear();
		for (int i = 0; i < 8; i++)
		{
			Inventory.push_back({ 0, 0 });
		}
		Inventory[0] = { 60, 1 };
		Inventory[1] = { 5, 5 };

		int InventorySlot = 0;

		while (Running) {

			PlayerDirection.x = 0;
			bool OnGround = Collition(PlayerPos[0].Position, { 0, -1 }, Range.x, Range.y, false, false);

			if (!OnGround) {
				PlayerDirection.y -= 0.5f;
			}
			else {
				PlayerDirection.y = 0;
			}

			while (SDL_PollEvent(&Event)) {
				if (Event.type == SDL_EVENT_QUIT) {
					Running = false;
					break;
				}
				else if (Event.type == SDL_EVENT_WINDOW_RESIZED) {
					width = Event.window.data1;
					height = Event.window.data2;

					Size(width, height, Range.y, Range.x);
				}

				if (Event.type == SDL_EVENT_KEY_DOWN)
				{
					SDL_Keycode KeyPressed = Event.key.scancode;

					if (KeyPressed == SDL_SCANCODE_ESCAPE) {
						Running = false;
						break;
					}
					else if (KeyPressed == SDL_SCANCODE_F11) {
						FullScreen = !FullScreen;
						SDL_SetWindowFullscreen(Window, FullScreen);
					}
				}
				else if (Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
					if (Event.button.button == SDL_BUTTON_LEFT) {
						short Type = 0;
						PlaceBlock(0, { Event.button.x, Event.button.y }, Range.y, PlayerPos[0].Position, Type);
						UpdateBlock(0, (int)Event.button.x, (int)Event.button.y, serverSocket);
						if (Type != 0)
						{
							Inventory[FindSlot(Inventory, Type)].Amount++;
							SimulateWater((int)((Event.button.x / BlockSize) + PlayerPos[0].Position.x) / 16);

						}
					}
					else if (Event.button.button == SDL_BUTTON_RIGHT && Inventory[InventorySlot].Amount > 0) {
						short Type = NULL;
						if (PlaceBlock(Inventory[InventorySlot].Type, { Event.button.x, Event.button.y }, Range.y, PlayerPos[0].Position, Type))
						{
							//std::cout << Type << std::endl;
							UpdateBlock(Inventory[InventorySlot].Type, (int)Event.button.x, (int)Event.button.y, serverSocket);
							Inventory[InventorySlot].Amount--;
							SimulateWater((int)((Event.button.x / BlockSize) + PlayerPos[0].Position.x) / 16);

							if (Inventory[InventorySlot].Amount == 0)
							{
								Inventory[InventorySlot].Type = 0;
							}
						}
					}
				}
			}

			Input(std::ref(PlayerDirection), OnGround, std::ref(InventorySlot), std::ref(PlayerPos[0].Position), Range);

			PlayerDirection.x = SDL_clamp(PlayerDirection.x, -1, 1);
			PlayerDirection.y = SDL_clamp(PlayerDirection.y, -1, 1);

			if (PlayerDirection.x != 0 && !Collition(PlayerPos[0].Position, { PlayerDirection.x, 0 }, Range.x, Range.y, false, false)) {
				PlayerPos[0].Position.x += PlayerDirection.x;
				UpdatePlayerPos(serverSocket, PlayerPos[0].Color, PlayerPos[0].Position);
			}
			if (PlayerDirection.y != 0 && !Collition(PlayerPos[0].Position, { 0, PlayerDirection.y }, Range.x, Range.y, false, false)) {
				PlayerPos[0].Position.y += PlayerDirection.y;
				UpdatePlayerPos(serverSocket, PlayerPos[0].Color, PlayerPos[0].Position);
				std::cout << "PlayerPos: " << PlayerDirection.x << ", " << PlayerDirection.y << std::endl;
			}


			SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
			SDL_RenderClear(Renderer);


			DrawBG(Renderer, PlayerPos[0].Position, Range);
			ShowInventor(Renderer, width, height, Inventory, InventorySlot);

			PlayerPos[0].Position.x = SDL_clamp(PlayerPos[0].Position.x, 9, 1600 - (int)(Range.x / 2));
			PlayerPos[0].Position.y = SDL_clamp(PlayerPos[0].Position.y, -4, 64 - Range.y);


			DrawPlayer(Renderer, Range, std::ref(PlayerPos));

			SDL_RenderPresent(Renderer);
			SDL_Delay(1000 / 10);
		}

		SDL_DestroyRenderer(Renderer);
		SDL_DestroyWindow(Window);
		SDL_Quit();

		return 0;
	}
}