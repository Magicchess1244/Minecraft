#include "GameClient.h"

constexpr double mouseSensitivity = 0.1f;
constexpr double playerSpeed = 1.0f;

static std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end;

	while ((end = s.find(delimiter, start)) != std::string::npos) {
		tokens.push_back(s.substr(start, end - start));
		start = end + delimiter.length();
	}
	tokens.push_back(s.substr(start));
	return tokens;
}
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
}
void GameClient::set_color() {
	int res;
	char buf[11];
	unsigned int server_color[3] = {};
	const char* command = "getColor";

	std::cout << "getColor" << std::endl;

	res = send(this->server, command, sizeof(command), 0);
	if (res <= 0) std::cerr << "Error requesting color from the server.";
	res = recv(this->server, buf, sizeof(buf), 0);
	if (res <= 0) std::cerr << "Error receiving color from the server.";
	else std::cout << "recived" << std::endl;

	std::cout << "color" << std::endl;
	int i = 0;
	for (std::string w : split(buf, ",")) {
		server_color[i++] = std::stoi(w);
	}

	std::cout << "New client color: " << server_color[0] << "," << server_color[BlockSize] <<"," << server_color[2] << std::endl;

	std::cout << "niggas online: " << this->players.size() << std::endl;
	if (this->players.size() > 0) {
		this->players[0].color = Color{ server_color[0], server_color[1], server_color[2] };
	}
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
	void DrawPlayer(SDL_Renderer* Renderer, Vector3 Range, std::vector<Player>& PlayerPos)
	{
		//Other player
		for (const Player &Position : PlayerPos)
		{
			int dx = (int)(Position.Position.x - PlayerPos[0].Position.x);
			int dy = (int)(Position.Position.y - PlayerPos[0].Position.y);

			bool InsideX = std::abs(dx) <= (Range.x / 2);
			if (InsideX) {
				int RelativeX = (int)((Range.x / 2 - 1 + dx) * BlockSize);
				int RelativeY = (int)((Range.y / 2 - 2 - dy) * BlockSize);

				SDL_FRect OtherPlayerRect = {
					(double)RelativeX,
					(double)RelativeY,
					(double)BlockSize,
					(double)BlockSize * 2
				};
				SDL_SetRenderDrawColor(Renderer, (Uint8)SDL_clamp(PlayerPos[0].color.r, 0, 255), (Uint8)SDL_clamp(PlayerPos[0].color.g, 0, 255), (Uint8)SDL_clamp(PlayerPos[0].color.b, 0, 255), 255);
				SDL_RenderFillRect(Renderer, &OtherPlayerRect);
			}
		}

		// Your player
		SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].color.r, 0, 255), SDL_clamp(PlayerPos[0].color.g, 0, 255), SDL_clamp(PlayerPos[0].color.b, 0, 255), 255);
		SDL_FRect PlayerRect = {
			(double)(Range.x / 2 - 1) * BlockSize,
			(double)(Range.y / 2 - 2) * BlockSize,
			(double)BlockSize,
			(double)BlockSize * 2
		};
		SDL_RenderFillRect(Renderer, &PlayerRect);

		SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].color.r + 90, 0, 255), SDL_clamp(PlayerPos[0].color.g + 90, 0, 255), SDL_clamp(PlayerPos[0].color.b + 90, 0, 255), 255);
		SDL_FRect InsidePlayerRect = {
			(double)(Range.x / 2 - 1) * BlockSize + (BlockSize * 0.1f),
			(double)(Range.y / 2 - 2) * BlockSize + (BlockSize * 0.1f),
			(double)(BlockSize * 0.8f),
			(double)(BlockSize * 0.9f) * 2
		};
		SDL_RenderFillRect(Renderer, &InsidePlayerRect);
	}
	int SetUpRender(SDL_Window** Window, SDL_Renderer** Renderer)
	{
	   if (SDL_Init(SDL_INIT_VIDEO)) { // Fixing SDL_Init condition
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
	void Render(RendererParameters Params, std::vector<Slot>& inventory, int inventorySlot, std::vector<Player>& players)
	{
			while (SDL_PollEvent(&Params.event)) {
				if (Params.event.type == SDL_EVENT_QUIT) {
					Params.Running = false;
					break;
				}
				else if (Params.event.type == SDL_EVENT_WINDOW_RESIZED) {
					Params.screenSize.x = Params.event.window.data1;
					Params.screenSize.y = Params.event.window.data2;

					//ChunckManager::Size(width, height, Range.y, Range.x);
				}

				if (Params.event.type == SDL_EVENT_KEY_DOWN)
				{
					SDL_Keycode KeyPressed = Params.event.key.scancode;

					if (KeyPressed == SDL_SCANCODE_ESCAPE) {
						Params.Running = false;
						break;
					}
					else if (KeyPressed == SDL_SCANCODE_F11) {
						Params.FullScreen = !Params.FullScreen;
						SDL_SetWindowFullscreen(Params.window, Params.FullScreen);
					}
				}
				else if (Params.event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
					if (Params.event.button.button == SDL_BUTTON_LEFT) {
						short Type = 0;
						if (false)//ChunckManager::PlaceBlock(0, { event.button.x, event.button.y }, Range.y, players[0].Position, std::ref(Type)))
						{
							short Slot = FindSlot(inventory, Type);
							inventory[Slot].Amount++;
							inventory[Slot].Type = Type;
							//std::cout << "Block broken: " << Type << " Slot: " << Slot << " Amount: " << inventory[Slot].Amount << " Slot type: " << inventory[Slot].Type << std::endl;
							ChunckManager::SimulateWater((int)((Params.event.button.x / BlockSize) + players[0].Position.x) / 16);

						}
					}
					else if (Params.event.button.button == SDL_BUTTON_RIGHT && inventory[inventorySlot].Amount > 0) {
						short Type = NULL;
						if (false)//ChunckManager::PlaceBlock(inventory[inventorySlot].Type, { event.button.x, event.button.y }, Range.y, players[0].Position, Type))
						{
							//std::cout << Type << std::endl;
							//UpdateBlock(Inventory[InventorySlot].Type, (int)Event.button.x, (int)Event.button.y, serverSocket);
							inventory[inventorySlot].Amount--;
							ChunckManager::SimulateWater((int)((Params.event.button.x / BlockSize) + players[0].Position.x) / 16);

							if (inventory[inventorySlot].Amount == 0)
							{
								inventory[inventorySlot].Type = 0;
							}
						}
					}
				}
			}
			SDL_SetRenderDrawColor(Params.renderer, 0, 178, 255, 255);
			SDL_RenderClear(Params.renderer);


			PlayerMovement(players[0], inventorySlot);

			/*
			Mesh mesh{};
			mesh.faces = 0;

			ChunckManager::Face(std::ref(mesh), players[0].Position, players[0].Rotation, { 0, 0, 0 }, Verts[1], 1, { (double)width, (double)height });
			ChunckManager::Face(std::ref(mesh), players[0].Position, players[0].Rotation, { 0, 0, 0 }, Verts[3], 2, { (double)width, (double)height });
			ChunckManager::Face(std::ref(mesh), players[0].Position, players[0].Rotation, { 0, 0, 0}, Verts[0], 3, {(double)width, (double)height});

			SDL_RenderGeometry(renderer, nullptr, mesh.Vertices.data(), mesh.faces * 4, mesh.Indices.data(), mesh.faces * 6);
			*/
			Player player = players[0];
			Params.rendererObj.DrawTerrain(player);
			
			//DrawBG(renderer, players[0],{ (double)width, (double)height, 0}, texture);
			//ChunckManager::ShowInventor(renderer, width, height, std::ref(inventory), inventorySlot, font);
			
			//DrawPlayer(renderer, Range, std::ref(players));
			SDL_RenderPresent(Params.renderer);
			SDL_Delay(1000 / 10);
	}
	void Input(Vector3& PlayerDirection, bool OnGround, int& InventorySlots, Vector3& PlayerRot) {
		const bool* KeyboardState = SDL_GetKeyboardState(NULL);
		const bool move_foward = (KeyboardState[SDL_SCANCODE_W] || KeyboardState[SDL_SCANCODE_UP]);
		const bool move_backward = (KeyboardState[SDL_SCANCODE_S] || KeyboardState[SDL_SCANCODE_DOWN]);
		const bool move_up = KeyboardState[SDL_SCANCODE_SPACE];
		const bool move_down = KeyboardState[SDL_SCANCODE_LSHIFT];
		const bool move_left = KeyboardState[SDL_SCANCODE_A] || KeyboardState[SDL_SCANCODE_LEFT];
		const bool move_right = KeyboardState[SDL_SCANCODE_D] || KeyboardState[SDL_SCANCODE_RIGHT];

		PlayerDirection.x = 0;
		PlayerDirection.y = 0;
		PlayerDirection.z = 0;

		// Get mouse movement
		float mouseX, mouseY;
		SDL_GetRelativeMouseState(&mouseX, &mouseY);

		// Apply mouse movement to rotation
		// Mouse X controls yaw (Y-axis rotation)
		// Mouse Y controls pitch (X-axis rotation)
		PlayerRot.y = -mouseX;  // Yaw (left/right)
		PlayerRot.x = -mouseY;  // Pitch (up/down)
		PlayerRot.z = 0;       // Roll (not used for FPS camera)

		if (move_left || move_right) {
			PlayerDirection.x = move_left ? -1 : 1;
		}
		if (move_down || move_up) {
			PlayerDirection.y = move_down ? -1 : 1;
		}
		if (move_backward || move_foward) {
			PlayerDirection.z = move_backward ? -1 : 1;
		}

		for (int i = 0; i < 8; ++i) {
			if (KeyboardState[SDL_SCANCODE_1 + i]) {
				InventorySlots = i;
				break;
			}
		}
	}
	void PlayerMovement(Player& player, int& inventorySlot) {
		Vector3 playerDirection = { 0, 0, 0 };
		Vector3 RotationDir = { 0, 0, 0 };

		Input(playerDirection, true, inventorySlot, RotationDir);


		if (RotationDir.x != 0 || RotationDir.y != 0) {

			player.Rotation.y += RotationDir.y * mouseSensitivity;
			player.Rotation.x += RotationDir.x * mouseSensitivity;
			player.Rotation.x = SDL_clamp(player.Rotation.x, -89.0f, 89.0f);
			if (player.Rotation.y > 360.0f) player.Rotation.y -= 360.0f;
			if (player.Rotation.y < 0.0f) player.Rotation.y += 360.0f;
		}

		// Handle movement
		if (playerDirection.x != 0 || playerDirection.y != 0 || playerDirection.z != 0) {
			// Move in Y first (e.g. jumping or flying)
			//std::cout << "Player Direction: " << playerDirection.x << ", " << playerDirection.y << ", " << playerDirection.z << std::endl;
			player.Position.y += playerDirection.y;
			playerDirection.y = 0;

			ChunckManager::Normalize(std::ref(playerDirection));
			// Rotate the horizontal movement by player rotation (Y-axis)
			double cosY = cosf(AngleToRadians(player.Rotation.y));
			double sinY = sinf(AngleToRadians(player.Rotation.y));

			Vector3 rotatedDirection = {
				playerDirection.x * cosY - playerDirection.z * sinY,
				0,
				playerDirection.x * sinY + playerDirection.z * cosY
			};

			// Apply movement
			player.Position.x += rotatedDirection.x * playerSpeed;
			player.Position.z += rotatedDirection.z * playerSpeed;
		}


	}

	void GameLoop(bool& running, GameClient& game)
	{
		game.add_player({ {100, 66, 100}, {0.0f, 0.0f, 0.0f}, {255, 0, 0}  });
		auto p = game.get_players();
		//ChunckManager::ChunkGenerator(p[0].Position);

		//game.MakeClient();
		//game.set_seed();
		//game.set_color();
		SetGradients();

		int width = 600;
		int height = 400;

		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;

		bool fullScreen = false;
		Vector3 playerDirection = { 0, 0 };

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

		SDL_SetWindowRelativeMouseMode(window, true);

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
		
		SDL_Surface* surface = SDL_LoadBMP("C:\\Users\\pumu\\source\\repos\\2Dminecraft\\x64\\Release\\Textures.bmp");
		if (!surface) {
			std::cerr << "SDL_LoadBMP failed: " << SDL_GetError() << std::endl;
		}

		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_DestroySurface(surface);
		if (!texture) {
			std::cerr << "SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl;
		}

		//ChunckManager::Size(width, height, Range.y, Range.x);
		Vector3 ScreenSize = { (double)width, (double)height, 0 };
		Renderer RendererObject;
		RendererObject.Setup(window, renderer, font, texture, ScreenSize);
		RendererParameters Params = {
			event,
			window,
			renderer,
			font,
			texture,
			RendererObject,
			std::ref(ScreenSize),
			std::ref(running),
			std::ref(fullScreen)
		};

		while (running) {
			Render( Params, std::ref(inventory), std::ref(inventorySlot), std::ref(p));
		}

		std::cout << "Exiting game..." << std::endl;

		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		TTF_Quit();
		SDL_Quit();

		return;
	}
}