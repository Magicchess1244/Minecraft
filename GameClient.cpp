#include "GameClient.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <windows.h>
#include <filesystem>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "SDL3_ttf.lib")
#pragma comment(lib, "SDL3_image.lib")

void GameClient::set_seed() {
	int res;
	char buf[16];
	unsigned int server_seed = 0;
	const char* command = "seed";
	std::cout << "seed" << std::endl;
	res = send(this->server, command, sizeof(command), 0);
	if (res <= 0) std::cerr << "Error requesting seed from the server.";
	res = recv(this->server, buf, sizeof(buf), 0);
	if (res <= 0) std::cerr << "Error receiving seed from the server.";
	std::cout << "buf: " << buf << std::endl;
	server_seed = static_cast<unsigned int>(std::stoi(std::string(buf)));
	std::cout << "New client seed: " << server_seed << std::endl;
	srand(server_seed);
	this->seed = server_seed;
	SetGradients();
}
void GameClient::set_color() {
	int res;
	char buf[8];
	unsigned int server_color[3] = {};
	const char* command = (char*)GetColor;

	res = send(this->server, command, sizeof(command), 0);
	if (res <= 0) std::cerr << "Error requesting color from the server.";
	res = recv(this->server, buf, sizeof(buf), 0);
	if (res <= 0) std::cerr << "Error receiving color from the server.";

	for (int i = 0; i < 3; i++) {
		server_color[i] = static_cast<unsigned int>(buf[i]);
	}

	std::cout << "New client color: " << server_color << std::endl;
	this->players[0].color = Color{server_color[0], server_color[1], server_color[2] };
}

//UI function
namespace BitMiner {
	int FindSlot(std::vector<Slot>& Inventory, short Type) {
		int index = 0;
		for (Slot slot : Inventory) {
			if ((slot.Type == Type || slot.Type == 0) && slot.Amount < 64) {
				std::cout << "Found slot" << index <<std::endl;
				return index;
			}
			index++;
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

		if (move_up && ChunckManager::Collition(PlayerPos, { 0, -1 }, Range.x, Range.y, true, false)) {
			PlayerDirection.y = 1;
		}

		for (int i = 0; i < 8; ++i) {
			if (KeyboardState[SDL_SCANCODE_1 + i]) {
				InventorySlots = i;
				//std::cout << i << std::endl;
				break;
			}
		}
	}
	void DrawBG(SDL_Renderer* Renderer, Vector2& PlayerPos, Vector2 Range) {
		int CurrentChunck = (int)floorf((PlayerPos.x) / 16);
		int RelativeX = (int)(PlayerPos.x) % 16;
		int xRange = Range.x - RelativeX;
		Mesh mesh = { 0 };
		ChunckManager::ChunkGenerator(CurrentChunck);

		ChunckManager::DrawChunk(CurrentChunck, RelativeX, (int)PlayerPos.y, xRange, Range.y, Range.x, mesh, true);
		SDL_RenderGeometry(Renderer, nullptr, mesh.Vertices, xRange * Range.y * 4, mesh.Indices, xRange * Range.y * 6);
		//ChunckManager::PrintChunk(CurrentChunck, 0, 0, xRange, 64, Range.x);


		if ((RelativeX - Range.x) < RelativeX)
		{
			ChunckManager::ChunkGenerator(CurrentChunck + 1);
			xRange = RelativeX;
			RelativeX = 0;
			CurrentChunck++;

			ChunckManager::DrawChunk(CurrentChunck, RelativeX, (int)PlayerPos.y, xRange, Range.y, Range.x, mesh, false);
			SDL_RenderGeometry(Renderer, nullptr, mesh.Vertices, xRange * Range.y * 4, mesh.Indices, xRange * Range.y * 6);
			//PrintChunk(CurrentChunck, RelativeX, PlayerPos.y, xRange, yRange, FullRange);
		}

	}
	void DrawPlayer(SDL_Renderer* Renderer, Vector2 Range, std::vector<Player>& PlayerPos)
	{
		//Other player
		for (Player Position : PlayerPos)
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
				SDL_SetRenderDrawColor(Renderer, (Uint8)SDL_clamp(PlayerPos[0].color.r, 0, 255), (Uint8)SDL_clamp(PlayerPos[0].color.g, 0, 255), (Uint8)SDL_clamp(PlayerPos[0].color.b, 0, 255), 255);
				SDL_RenderFillRect(Renderer, &OtherPlayerRect);
			}
		}

		// Your player
		SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].color.r, 0, 255), SDL_clamp(PlayerPos[0].color.g, 0, 255), SDL_clamp(PlayerPos[0].color.b, 0, 255), 255);
		SDL_FRect PlayerRect = {
			(float)(Range.x / 2 - 1) * BlockSize,
			(float)(Range.y / 2 - 2) * BlockSize,
			(float)BlockSize,
			(float)BlockSize * 2
		};
		SDL_RenderFillRect(Renderer, &PlayerRect);

		SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].color.r + 90, 0, 255), SDL_clamp(PlayerPos[0].color.g + 90, 0, 255), SDL_clamp(PlayerPos[0].color.b + 90, 0, 255), 255);
		SDL_FRect InsidePlayerRect = {
			(float)(Range.x / 2 - 1) * BlockSize + (BlockSize * 0.1f),
			(float)(Range.y / 2 - 2) * BlockSize + (BlockSize * 0.1f),
			(float)(BlockSize * 0.8f),
			(float)(BlockSize * 0.9f) * 2
		};
		SDL_RenderFillRect(Renderer, &InsidePlayerRect);
	}

	int SetUpRender(SDL_Window** Window, SDL_Renderer** Renderer)
	{
	   if (SDL_Init(SDL_INIT_VIDEO) < 0) { // Fixing SDL_Init condition
		   std::cout << "Error initializing SDL: " << SDL_GetError();
		   return 1;
	   } else {
		   std::cout << "SDL initialized successfully." << std::endl;
	   }

	   *Window = SDL_CreateWindow("Bit Miner", 600, 400, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	   if (*Window == nullptr) { // Fixing pointer dereference
		   std::cout << "Error creating window: " << SDL_GetError();
		   SDL_Quit();
		   return 1;
	   }

	   *Renderer = SDL_CreateRenderer(*Window, NULL);
	   if (*Renderer == nullptr) { // Fixing pointer dereference
		   std::cout << "Error creating renderer: " << SDL_GetError();
		   SDL_DestroyWindow(*Window);
		   SDL_Quit();
		   return 1;
	   }
	   if (!SDL_Init(SDL_INIT_VIDEO)) {
		   std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
		   return -1;
	   }

	   // Initialize SDL_ttf
	   if (!TTF_Init()) {
		   std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
		   SDL_Quit();
		   return -1;
	   }
	   return 0; // Ensure function returns success
	}
	void Render(SDL_Event event, SDL_Renderer* renderer, SDL_Window* window, Vector2 Range, int& width, int& height, std::vector<Slot>& inventory, int inventorySlot, std::vector<Player>& players, bool& Running, bool& FullScreen, TTF_Font* font)
	{
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_EVENT_QUIT) {
					Running = false;
					break;
				}
				else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
					width = event.window.data1;
					height = event.window.data2;

					ChunckManager::Size(width, height, Range.y, Range.x);
				}

				if (event.type == SDL_EVENT_KEY_DOWN)
				{
					SDL_Keycode KeyPressed = event.key.scancode;

					if (KeyPressed == SDL_SCANCODE_ESCAPE) {
						Running = false;
						break;
					}
					else if (KeyPressed == SDL_SCANCODE_F11) {
						FullScreen = !FullScreen;
						SDL_SetWindowFullscreen(window, FullScreen);
					}
				}
				else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
					if (event.button.button == SDL_BUTTON_LEFT) {
						short Type = 0;
						if (ChunckManager::PlaceBlock(0, { event.button.x, event.button.y }, Range.y, players[0].Position, std::ref(Type)))
						{
							short Slot = FindSlot(inventory, Type);
							inventory[Slot].Amount++;
							inventory[Slot].Type = Type;
							//std::cout << "Block broken: " << Type << " Slot: " << Slot << " Amount: " << inventory[Slot].Amount << " Slot type: " << inventory[Slot].Type << std::endl;
							ChunckManager::SimulateWater((int)((event.button.x / BlockSize) + players[0].Position.x) / 16);

						}
					}
					else if (event.button.button == SDL_BUTTON_RIGHT && inventory[inventorySlot].Amount > 0) {
						short Type = NULL;
						if (ChunckManager::PlaceBlock(inventory[inventorySlot].Type, { event.button.x, event.button.y }, Range.y, players[0].Position, Type))
						{
							//std::cout << Type << std::endl;
							//UpdateBlock(Inventory[InventorySlot].Type, (int)Event.button.x, (int)Event.button.y, serverSocket);
							inventory[inventorySlot].Amount--;
							ChunckManager::SimulateWater((int)((event.button.x / BlockSize) + players[0].Position.x) / 16);

							if (inventory[inventorySlot].Amount == 0)
							{
								inventory[inventorySlot].Type = 0;
							}
						}
					}
				}
			}
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);


			DrawBG(renderer, players[0].Position, Range);
			ChunckManager::ShowInventor(renderer, width, height, std::ref(inventory), inventorySlot, font);
			
			DrawPlayer(renderer, Range, std::ref(players));
			SDL_RenderPresent(renderer);
			SDL_Delay(1000 / 10);
	}
	void PlayerMovement(Vector2& playerDirection, Vector2& range, Player& player, int& inventorySlot)  { 
		
		playerDirection.x = 0;  
		bool OnGround = ChunckManager::Collition(player.Position, { 0, -1 }, range.x, range.y, false, false);

		if (!OnGround) {  
			playerDirection.y -= 0.5f;  
		} else {  
			playerDirection.y = 0;  
		}  

		Input(std::ref(playerDirection), OnGround, std::ref(inventorySlot), std::ref(player.Position), range);  

		playerDirection.x = SDL_clamp(playerDirection.x, -1, 1);  
		playerDirection.y = SDL_clamp(playerDirection.y, -1, 1);  

		if (playerDirection.x != 0 && !ChunckManager::Collition(player.Position, { playerDirection.x, 0 }, range.x, range.y, false, false)) {
			player.Position.x += playerDirection.x;  
		}  
		if (playerDirection.y != 0 && !ChunckManager::Collition(player.Position, { 0, playerDirection.y }, range.x, range.y, false, false)) {
			player.Position.y += playerDirection.y;  
		}  

		player.Position.x = SDL_clamp(player.Position.x, 9, 1600 - (int)(range.x / 2));  
		player.Position.y = SDL_clamp(player.Position.y, -4, 64 - range.y);  
	}

	void GameLoop(bool& running, GameClient& game)
	{
		
		game.MakeClient();
		game.set_seed();

		Vector2 Range = { 16, 10 };
		int width = 600;
		int height = 400;

		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;

		bool fullScreen = false;
		Vector2 playerDirection = { 0, 0 };

		SDL_Event event{};
		std::vector<Slot> inventory;
		
		for (int i = 0; i < 8; i++)
		{
			inventory.push_back({ 0, 0 });
		}

		inventory[0] = { 60, 1 };
		inventory[1] = { 5, 5 };

		int inventorySlot = 0;

		SetUpRender(&window, &renderer);

		char buffer[MAX_PATH];
		GetModuleFileNameA(NULL, buffer, MAX_PATH);

		TTF_Font* font = TTF_OpenFont("Quantico-Bold.ttf", 24);

		/*
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_FRect rect = { 0,0,100,100 };
		SDL_RenderFillRect(renderer, &rect);

		SDL_Color White = { 200, 200, 200 };
		SDL_Surface* surface = TTF_RenderText_Blended(font, "HelloWorld SDL3 TTF",sizeof("HelloWorld SDL3 TTF"), White);
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface); 
		SDL_DestroySurface(surface);
		SDL_FRect dstRect{ 100, 100, 200, 80 };
		SDL_RenderTexture(renderer, texture, NULL, &dstRect);
		SDL_DestroyTexture(texture);

		SDL_RenderPresent(renderer);
		*/


		SDL_Surface* surface = IMG_Load("Texture.png");
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_DestroySurface(surface);

		SDL_Vertex vertices[4];
		vertices[0] = { {100, 100}, SDL_FColor{255, 255, 255, 255}, {0.0f, 0.0f} };
		vertices[1] = { {200, 100}, SDL_FColor{255, 255, 255, 255}, {1.0f, 0.0f} };
		vertices[2] = { {200, 200}, SDL_FColor{255, 255, 255, 255}, {1.0f, 1.0f} };
		vertices[3] = { {100, 200}, SDL_FColor{255, 255, 255, 255}, {0.0f, 1.0f} };

		int indices[] = { 0, 1, 2, 0, 2, 3 };

		SDL_RenderClear(renderer);
		SDL_RenderGeometry(renderer, texture, vertices, 4, indices, 6);
		SDL_RenderPresent(renderer);

		ChunckManager::Size(width, height, Range.y, Range.x);
		
		game.add_player({ {800.0, 64.0}, {255, 0, 0} });
		auto p = game.get_players();

		while (running) {
			PlayerMovement(std::ref(playerDirection), std::ref(Range), std::ref(p[0]), std::ref(inventorySlot));
			Render(event, renderer, window, Range, std::ref(width), std::ref(height), std::ref(inventory), std::ref(inventorySlot), std::ref(p), std::ref(running), std::ref(fullScreen), font);
		}

		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		TTF_Quit();


		return;
	}
}