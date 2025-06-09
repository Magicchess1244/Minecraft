#include "ChunkManager.h"
#include "PerlinNoise.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define Max_Player 8


enum Commands {
	UPDATE_PLAYER_POS,
	UPDATE_BLOCKS,
	GET_SEED
};

//Networking functions
void UpdatePlayerPos(int X, int Y, SOCKET socket) 
{
	/*
	const int command = UPDATE_PLAYER_POS;
	int sent = send(serverSocket, reinterpret_cast<const char*>(&command), sizeof(command), 0);

	if (sent == SOCKET_ERROR) {
		std::cerr << "Failed to send pos\n";
	}
	else {
		//std::cout << "Sending pos from server...\n";
	}

	int32_t netSeed = 0;
	char* buffer = reinterpret_cast<char*>(&netSeed);
	int bytesToReceive = sizeof(netSeed);

	int x = send(serverSocket, reinterpret_cast<const char*>(&X), bytesToReceive, 0);
	int y = send(serverSocket, reinterpret_cast<const char*>(&Y), bytesToReceive, 0);
	if (x == 0 || y == 0) {
		std::cerr << "Connection closed by server.\n";
		return;
	}
	else if (x < 0 || y < 0) {
		std::cerr << "recv() failed with error: " << WSAGetLastError() << "\n";
		return;
	}
	*/
}
void UpdateBlock(int Type, int X, int Y, SOCKET socket)
{
	/*
	const int command = UPDATE_BLOCKS;
	int sent = send(serverSocket, reinterpret_cast<const char*>(&command), sizeof(command), 0);

	if (sent == SOCKET_ERROR) {
		std::cerr << "Failed to send block info\n";
	}
	else {
		//std::cout << "Sending pos from server...\n";
	}

	int32_t netSeed = 0;
	char* buffer = reinterpret_cast<char*>(&netSeed);
	int bytesToReceive = sizeof(netSeed);

	int x = send(serverSocket, reinterpret_cast<const char*>(&X), bytesToReceive, 0);
	int y = send(serverSocket, reinterpret_cast<const char*>(&Y), bytesToReceive, 0);
	int type = send(serverSocket, reinterpret_cast<const char*>(&Type), bytesToReceive, 0);
	if (x == 0 || y == 0 || type == 0) {
		std::cerr << "Connection closed by server.\n";
		return;
	}
	else if (x < 0 || y < 0 || type < 0) {
		std::cerr << "recv() failed with error: " << WSAGetLastError() << "\n";
		return;
	}
	*/
}
int GetSeed(SOCKET serverSocket)
{
	const int command = htonl(GET_SEED);
	int sent = send(serverSocket, reinterpret_cast<const char*>(&command), sizeof(command), 0);

	if (sent == SOCKET_ERROR) {
		std::cerr << "Failed to send command\n";
	}
	else {
		std::cout << "Requesting seed from server...\n";
	}

	int32_t netSeed = 0;
	char* buffer = reinterpret_cast<char*>(&netSeed);
	int bytesToReceive = sizeof(netSeed);

	int result = recv(serverSocket, buffer, bytesToReceive, 0);
	if (result == 0) {
		std::cerr << "Connection closed by server.\n";
		return 0;
	}
	else if (result < 0) {
		std::cerr << "recv() failed with error: " << WSAGetLastError() << "\n";
		return 0;
	}

	int32_t receivedSeed = ntohl(netSeed);
	std::cout << "Received seed: " << receivedSeed << std::endl;
	return receivedSeed;
}
void handleClientMessage(SOCKET client, std::vector<SOCKET>& clients, bool server, int seed, bool& Running)
{  
	std::cout << "Running: " << Running << std::endl;
	while (Running) {
		int command;
		int result = recv(client, (char*)&command, sizeof(command), 0);
		command = ntohl(command);
		if (result > 0)
		{
			std::cout << "Received command: " << command << "\n";

			if (server) { // Server  
				if (command == UPDATE_PLAYER_POS) {
					int x_net, y_net;
					int receivedX = recv(client, (char*)&x_net, sizeof(x_net), 0);
					int receivedY = recv(client, (char*)&y_net, sizeof(y_net), 0);
					if (receivedX == sizeof(x_net) && receivedY == sizeof(y_net)) {
						for (SOCKET client : clients)
						{
							UpdatePlayerPos(x_net, y_net, client);
						}
					}
					else {
						std::cerr << "Failed to receive player position\n";
					}
				}
				else if (command == UPDATE_BLOCKS) {
					std::cout << "Received command: " << command << "\n";
					int x_net, y_net, type_net;
					int receivedX = recv(client, (char*)&x_net, sizeof(x_net), 0);
					int receivedY = recv(client, (char*)&y_net, sizeof(y_net), 0);
					int receivedType = recv(client, (char*)&type_net, sizeof(type_net), 0);
					if (receivedX == sizeof(x_net) && receivedY == sizeof(y_net) && receivedType == sizeof(type_net)) {
						short* PlaceBlockType = 0;
						for (SOCKET client : clients)
						{
							UpdateBlock(type_net, x_net, y_net, client);
						}
					}
					else {
						std::cerr << "Failed to receive block position\n";
					}
				}
				else if (command == GET_SEED) {
					std::cout << "Received command: " << command << "\n";
					int netSeed = htonl(seed);
					int sent = send(client, (char*)&netSeed, sizeof(netSeed), 0);
					if (sent == SOCKET_ERROR) {
						std::cerr << "Failed to send seed\n";
					}
				}
			}
			/*
			else {
				if (command == UPDATE_PLAYER_POS) {
					int x_net, y_net;
					int receivedX = recv(client, (char*)&x_net, sizeof(x_net), 0);
					int receivedY = recv(client, (char*)&y_net, sizeof(y_net), 0);
					if (receivedX == sizeof(x_net) && receivedY == sizeof(y_net)) {
						std::lock_guard<std::mutex> lock(myMutex);
						PlayerPos[1].x = ntohl(x_net);
						PlayerPos[1].y = ntohl(y_net);
					}
					else {
						std::cerr << "Failed to receive player position\n";
					}
				}
				else if (command == UPDATE_BLOCKS) {
					std::cout << "Received block update command\n";
					int x_net, y_net, type_net;
					int receivedX = recv(client, (char*)&x_net, sizeof(x_net), 0);
					int receivedY = recv(client, (char*)&y_net, sizeof(y_net), 0);
					int receivedType = recv(client, (char*)&type_net, sizeof(type_net), 0);
					if (receivedX == sizeof(x_net) && receivedY == sizeof(y_net) && receivedType == sizeof(type_net)) {
						short* PlaceBlockType = 0;
						PlaceBlock(ntohl(type_net), { (float)ntohl(x_net), (float)ntohl(y_net) }, yRange, PlayerPos[1], PlaceBlockType);
					}
					else {
						std::cerr << "Failed to receive block position\n";
					}
				}
			}
			*/
		}
	}
}

SOCKET MakeServer()
{
	WSAData wsaData;
	if (WSAStartup(WINSOCK_VERSION, &wsaData) != 0) {
		std::cerr << "Failed to initialize WinSock\n";
		return INVALID_SOCKET;
	}

	SOCKET hostSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (hostSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed\n";
		WSACleanup();
		return INVALID_SOCKET;
	}

	int PORT = 8080;

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(PORT);

	if (bind(hostSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Bind failed\n";
		closesocket(hostSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	if (listen(hostSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "Listen failed\n";
		closesocket(hostSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	std::cout << "Server listening on port " << PORT << "...\n";
	return hostSocket;
}
SOCKET MakeClient()
{
	WSADATA wsaData;
	WSAStartup(WINSOCK_VERSION, &wsaData);
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	int PORT = 8080;
	const char* SERVER_IP = "127.0.0.1";

	sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	inet_pton(AF_INET, SERVER_IP, &server.sin_addr);

	connect(serverSocket, (sockaddr*)&server, sizeof(server));
	if (serverSocket == INVALID_SOCKET) {
		std::cout << "Failed to connect to the server.\n";
	}
	else
	{
		std::cout << "Connected to the server.\n";
	}


	return serverSocket;
}
int CloseServer(std::vector<SOCKET>& sockets)
{
	for (SOCKET socket : sockets)
	{
		closesocket(socket);
	}
	WSACleanup();
	std::cout << "Server closed.\n";
	return 0;
}
void SetUpServer(int& Side, std::vector<SOCKET>& sockets, SOCKET& hostSocket)
{
	int choice;
	std::cout << "Choose an option:\n1. Start Server\n2. Start Client\n";
	std::cin >> choice;
	if (choice == 1) {
		Side = 0;
		hostSocket = MakeServer();
		sockets.push_back(MakeClient());
	}
	else if (choice == 2) {
		Side = 1;
		sockets.push_back(MakeClient());
	}
	else {
		std::cerr << "Invalid choice.\n";
	}
	std::cout << "num clients: " << sockets.size() << std::endl;
	std::cout << "clients: " << sockets[0] << std::endl;
}
void AcceptClients(SOCKET hostSocket, std::vector<SOCKET>& sockets,bool& Running, int seed) {
	while (sockets.size() < Max_Player) {
		sockaddr_in client;
		int clientSize = sizeof(sockaddr_in);

		SOCKET clientSocket = accept(hostSocket, (sockaddr*)&client, &clientSize);
		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "accept() failed with error: " << WSAGetLastError() << std::endl;
			continue;
		}

		sockets.push_back(clientSocket);
		std::cout << sockets.size() << " clients connected." << std::endl;
		std::cout << "Client connected! Socket: " << clientSocket << std::endl;

		std::thread(handleClientMessage, clientSocket, std::ref(sockets), true, seed, std::ref(Running)).detach();
	}
}

//UI function
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

	PlayerDirection.x = 0;  
	
	if (KeyboardState[SDL_SCANCODE_A] || KeyboardState[SDL_SCANCODE_LEFT]) {  
		PlayerDirection.x = -1;  
	}  
	if (KeyboardState[SDL_SCANCODE_D] || KeyboardState[SDL_SCANCODE_RIGHT]) {  
		PlayerDirection.x = 1;  
	}  

	if ((KeyboardState[SDL_SCANCODE_W] || KeyboardState[SDL_SCANCODE_UP] || KeyboardState[SDL_SCANCODE_SPACE]) && Collition(PlayerPos, { 0, -1 }, (int)Range.x, Range.y, true, false)) {
		PlayerDirection.y = 1;  
	}  

	if (KeyboardState[SDL_SCANCODE_1]) {  
		InventorySlots = 0;  
	} else if (KeyboardState[SDL_SCANCODE_2]) {  
		InventorySlots = 1;  
	} else if (KeyboardState[SDL_SCANCODE_3]) {
		InventorySlots = 2;
	} else if (KeyboardState[SDL_SCANCODE_4]) {
		InventorySlots = 3;
	} else if (KeyboardState[SDL_SCANCODE_5]) {
		InventorySlots = 4;
	} else if (KeyboardState[SDL_SCANCODE_6]) {
		InventorySlots = 5;
	} else if (KeyboardState[SDL_SCANCODE_7]) {
		InventorySlots = 6;
	} else if (KeyboardState[SDL_SCANCODE_8]) {
		InventorySlots = 7;
	}
}
void DrawBG(SDL_Renderer* Renderer, Vector2& PlayerPos, Vector2 Range){

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
void DrawPlayer(SDL_Renderer* Renderer, Vector2 Range, std::vector<Vector2>& PlayerPos) {

	int dx = (int)(PlayerPos[1].x - PlayerPos[0].x);
	int dy = (int)(PlayerPos[1].y - PlayerPos[0].y);

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
		SDL_SetRenderDrawColor(Renderer, 0, 255, 0, 255);
		SDL_RenderFillRect(Renderer, &OtherPlayerRect);
	}

	SDL_SetRenderDrawColor(Renderer, 255, 0, 0, 255);
	SDL_FRect PlayerRect = {
		(float)(Range.x / 2 - 1) * BlockSize,
		(float)(Range.y / 2 - 2) * BlockSize,
		(float)BlockSize,
		(float)BlockSize * 2
	};
	SDL_RenderFillRect(Renderer, &PlayerRect);
}
int Ui(SOCKET serverSocket, Vector2 Range, bool& Running)
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
	std::vector<Vector2> PlayerPos;
	PlayerPos.resize(Max_Player);
	for (Vector2 Pos : PlayerPos) {
		Pos = { 800, 1000 };
	}

	SDL_Event Event;
	std::vector<Slot> Inventory;
	Inventory.resize(8);
	Inventory.push_back({ 60, 1 });
	Inventory.push_back({ 5, 5 });

	int InventorySlot = 0;

	while (Running){
		UpdatePlayerPos((int)PlayerPos[0].x, (int)PlayerPos[0].y, serverSocket);

		PlayerDirection.x = 0;
		bool OnGround = Collition(PlayerPos[0], { 0, -1 }, Range.x, Range.y, false, false);

		if (!OnGround) {
			PlayerDirection.y -= 0.5f;
		} else {
			PlayerDirection.y = 0;
		}

		while (SDL_PollEvent(&Event)) {
			if (Event.type == SDL_EVENT_QUIT) {
				Running = false;
				break;
			} else if (Event.type == SDL_EVENT_WINDOW_RESIZED) {
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
					PlaceBlock(0, { Event.button.x, Event.button.y }, Range.y, PlayerPos[0], Type);
					UpdateBlock(0, (int)Event.button.x, (int)Event.button.y, serverSocket);
					if (Type != 0)
					{
						Inventory[FindSlot(Inventory, Type)].Amount++;
						SimulateWater((int)((Event.button.x / BlockSize) + PlayerPos[0].x) / 16);

					}
				}
				else if (Event.button.button == SDL_BUTTON_RIGHT && Inventory[InventorySlot].Amount > 0) {
					short Type = NULL;
					if (PlaceBlock(Inventory[InventorySlot].Type, { Event.button.x, Event.button.y }, Range.y, PlayerPos[0], Type))
					{
						//std::cout << Type << std::endl;
						UpdateBlock(Inventory[InventorySlot].Type, (int)Event.button.x, (int)Event.button.y, serverSocket);
						Inventory[InventorySlot].Amount--;
						SimulateWater((int)((Event.button.x / BlockSize) + PlayerPos[0].x) / 16);

						if (Inventory[InventorySlot].Amount == 0)
						{
							Inventory[InventorySlot].Type = 0;
						}
					}
				}
			}
		}

		Input(PlayerDirection, OnGround, InventorySlot, PlayerPos[0], Range);

		PlayerDirection.x = SDL_clamp(PlayerDirection.x, -1, 1);
		PlayerDirection.y = SDL_clamp(PlayerDirection.y, -1, 1);

		if (PlayerDirection.x != 0  && !Collition(PlayerPos[0], { PlayerDirection.x, 0 }, Range.x, Range.y, false, false)){
			PlayerPos[0].x += PlayerDirection.x;
		}
		if (PlayerDirection.y != 0 && !Collition(PlayerPos[0], { 0, PlayerDirection.y }, Range.x, Range.y, false, false)) {
			PlayerPos[0].y += PlayerDirection.y;
		}


		SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
		SDL_RenderClear(Renderer);
		

		DrawBG(Renderer, PlayerPos[0], Range);
		ShowInventor(Renderer, width, height, Inventory, InventorySlot);

		PlayerPos[0].x = SDL_clamp(PlayerPos[0].x, 9, 1600 - (int)(Range.x / 2));
		PlayerPos[0].y = SDL_clamp(PlayerPos[0].y, -4, 64 - Range.y);
		

		DrawPlayer(Renderer, Range, PlayerPos);

		SDL_RenderPresent(Renderer);
		SDL_Delay(1000 / 10);
	}
	SDL_DestroyRenderer(Renderer);
	SDL_DestroyWindow(Window);
	SDL_Quit();

	return 0;
}
int GameSetUp(bool GenerateSeed, SOCKET serverSocket) 
{
	int seed = 0;
	if (GenerateSeed)
	{
		srand((unsigned int)time(0));

		seed = rand();
	}
	else
	{
		seed = GetSeed(serverSocket);
	}

	std::cout << "Seed: " << seed << std::endl;

	srand(seed);
	SetGradients();

	return seed;
}

int main(int Argc, char* Argv[])
{
	bool Running = true;

	int Side = 0;
	std::vector<SOCKET> sockets;
	sockets.resize(Max_Player);
	sockets.clear();
	SOCKET hostSockets;

	SetUpServer(Side, std::ref(sockets), std::ref(hostSockets));
	int seed = GameSetUp(Side == 0, sockets[0]);

	std::thread acceptThread;
	if (Side == 0)
	{
		acceptThread = std::thread(AcceptClients, hostSockets, std::ref(sockets), std::ref(Running), seed);
		acceptThread.detach();
	}
	std::thread(handleClientMessage, sockets[0], std::ref(sockets),false, seed, std::ref(Running)).detach();

	Vector2 Range = { 16, 10 };
	std::thread UiThread(Ui, sockets[0], Range, std::ref(Running));
	UiThread.detach();
	while (Running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	std::cout << "Exiting game..." << std::endl;
	CloseServer(sockets);

	return 0;
}