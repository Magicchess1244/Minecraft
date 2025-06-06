#include "ChunkManager.h"
#include "PerlinNoise.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ccomplex>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

std::mutex myMutex;

std::vector<SOCKET> clients;
SOCKET hostSocket;
SOCKET serverSocket;
u_long mode = 0;

fd_set readSet;

enum Commands {
	UPDATE_PLAYER_POS,
	UPDATE_BLOCKS,
	GET_SEED
};

int Side = 1;

const int FullRange = 16;
const int yRange = 10;
const char* SERVER_IP = "127.0.0.1";
int seed = 0;
bool Running = true;

Vector2 PlayerPos[8] = { {800, 1000}, {800, 1000}, {800, 1000}, {800, 1000}, {800, 1000}, {800, 1000},{800, 1000}, {800, 1000} };

//Networking functions
void UpdatePlayerPos(int X, int Y, SOCKET socket) 
{
	const char* command = (char*)UPDATE_PLAYER_POS;
	int sent = send(socket, command, (int)strlen(command), 0);
	if (sent == SOCKET_ERROR) {
		std::cerr << "Failed to send command: UPDATE_PLAYER_POS \n";
	}
	
	int x = htonl(X);
	int sentX = send(socket, (char*)&x, sizeof(x), 0);

	int y = htonl(Y);
	int sentY = send(socket, (char*)&y, sizeof(y), 0);
}
void UpdateBlock(int Type, int X, int Y, SOCKET socket)
{
	const char* command = (char*)UPDATE_BLOCKS;
	int sent = send(serverSocket, command, (int)strlen(command), 0);
	if (sent == SOCKET_ERROR) {
		std::cerr << "Failed to send command\n";
	}

	int x = htonl(X);
	int sentX = send(serverSocket, (char*)&x, sizeof(x), 0);

	int y = htonl(Y);
	int sentY = send(serverSocket, (char*)&y, sizeof(y), 0);

	int type = htonl(Type);
	int sentType = send(serverSocket, (char*)&type, sizeof(type), 0);
}
void GetSeed()
{
	const char* command = (char*)GET_SEED;
	int sent = send(serverSocket, command, (int)strlen(command), 0);
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
		return;
	}
	else if (result < 0) {
		std::cerr << "recv() failed with error: " << WSAGetLastError() << "\n";
		return;
	}

	int32_t receivedSeed = ntohl(netSeed); // Convert from network byte order to host byte order
	std::cout << "Received seed: " << receivedSeed << std::endl;
	seed = receivedSeed;
}
void handleClientMessage(SOCKET client, char* InfoRecived)  
{  
	int command = (int)InfoRecived;

	if (Side == 0) { // Server  
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
}

int MakeServer()
{
	WSAData wsaData;
	if (WSAStartup(WINSOCK_VERSION, &wsaData) != 0) {
		std::cerr << "Failed to initialize WinSock\n";
		return 1;
	}

	hostSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (hostSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed\n";
		WSACleanup();
		return 1;
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(PORT);

	if (bind(hostSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Bind failed\n";
		closesocket(hostSocket);
		WSACleanup();
		return 1;
	}

	if (listen(hostSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "Listen failed\n";
		closesocket(hostSocket);
		WSACleanup();
		return 1;
	}

	std::cout << "Server listening on port " << PORT << "...\n";
	return 0;
}
int MakeClient()
{
	WSADATA wsaData;
	WSAStartup(WINSOCK_VERSION, &wsaData);
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);

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

	return 0;
}
int CloseServer()
{
	if (Side == 0)
	{
		closesocket(hostSocket);
	}
	for (int i = 0; i < clients.size(); i++)
	{
		closesocket(clients[i]);
	}
	closesocket(serverSocket);
	WSACleanup();
	std::cout << "Server closed.\n";
	return 0;
}
int SetUpServer()
{
	int choice;
	std::cout << "Choose an option:\n1. Start Server\n2. Start Client\n";
	std::cin >> choice;
	if (choice == 1) {
		Side = 0;
		return (MakeServer() & MakeClient());
	}
	else if (choice == 2) {
		Side = 1;
		return MakeClient();
	}
	else {
		std::cerr << "Invalid choice.\n";
		return 1;
	}
}
void AcceptClients() {
	if (Side == 0) {
		while (Running && clients.size() < 8) {
			sockaddr_in client;
			int clientSize = sizeof(sockaddr_in);

			SOCKET clientSocket = accept(hostSocket, (sockaddr*)&client, &clientSize);
			if (clientSocket == INVALID_SOCKET) {
				std::cerr << "accept() failed with error: " << WSAGetLastError() << std::endl;
				continue;
			}

			std::lock_guard<std::mutex> lock(myMutex); // Protect the clients vector
			clients.push_back(clientSocket);
			std::cout << clients.size() << " clients connected." << std::endl;
			std::cout << "Client connected! Socket: " << clientSocket << std::endl;
		}

	}
}
void MultiPlayer() {

	while (Running) {
		FD_ZERO(&readSet);
		FD_SET(serverSocket, &readSet);

		for (SOCKET client : clients) {
			FD_SET(client, &readSet);
		}

		timeval timeout = { 0, 100000 }; // 100 ms timeout
		int result = select(0, &readSet, NULL, NULL, &timeout);

		if (result > 0) {
			for (auto it = clients.begin(); it != clients.end(); ++it) {
				SOCKET client = *it;
				if (FD_ISSET(client, &readSet)) {
					char buffer[BUFFER_SIZE];
					int bytesReceived = recv(client, buffer, sizeof(buffer), 0);
					if (bytesReceived <= 0) {
						closesocket(client);
						it = clients.erase(it);
						--it;
					}
					else {
						handleClientMessage(client, buffer);
					}
				}
			}
		}
	}
}

//UI function
int FindSlot(Slot* Inventory, short Type) {
	for (int i = 0; i < 8; i++) {
		if ((Inventory[i].Type == Type || Inventory[i].Type == 0) && Inventory[i].Amount < 64) {
			Inventory[i].Type = Type;
			return i;
		}
	}
	std::cout << "Inventory full, cannot add item of type: " << Type << std::endl;
	return -1; // Inventory full
}
void Input(Vector2* PlayerDirection, bool OnGround, int* InventorySlots, Vector2* PlayerPos) {  
	const bool* KeyboardState = SDL_GetKeyboardState(NULL);  

	PlayerDirection->x = 0;  
	
	if (KeyboardState[SDL_SCANCODE_A] || KeyboardState[SDL_SCANCODE_LEFT]) {  
		PlayerDirection->x = -1;  
	}  
	if (KeyboardState[SDL_SCANCODE_D] || KeyboardState[SDL_SCANCODE_RIGHT]) {  
		PlayerDirection->x = 1;  
	}  

	if ((KeyboardState[SDL_SCANCODE_W] || KeyboardState[SDL_SCANCODE_UP] || KeyboardState[SDL_SCANCODE_SPACE]) && Collition(PlayerPos, { 0, -1 }, FullRange, yRange, true, false)) {
		PlayerDirection->y = 1;  
	}  

	if (KeyboardState[SDL_SCANCODE_1]) {  
		*InventorySlots = 0;  
	} else if (KeyboardState[SDL_SCANCODE_2]) {  
		*InventorySlots = 1;  
	} else if (KeyboardState[SDL_SCANCODE_3]) {
		*InventorySlots = 2;
	} else if (KeyboardState[SDL_SCANCODE_4]) {
		*InventorySlots = 3;
	} else if (KeyboardState[SDL_SCANCODE_5]) {
		*InventorySlots = 4;
	} else if (KeyboardState[SDL_SCANCODE_6]) {
		*InventorySlots = 5;
	} else if (KeyboardState[SDL_SCANCODE_7]) {
		*InventorySlots = 6;
	} else if (KeyboardState[SDL_SCANCODE_8]) {
		*InventorySlots = 7;
	}
}
void DrawBG(SDL_Renderer* Renderer, Vector2* PlayerPos){
	int CurrentChunck = (int)floorf((PlayerPos->x) / 16);
	int RelativeX = (int)(PlayerPos->x) % 16;
	int xRange = FullRange - RelativeX;
	Mesh mesh = { 0 };
	ChunkGenerator(CurrentChunck);
	if (PlayerPos->y == 1000) {
		PlayerPos->y = (float)GetHeight(CurrentChunck);
		//std::cout << "PlayerPos: " << PlayerPos.x << ", " << PlayerPos.y << std::endl;
	}

	//std::cout << "PlayerPos: " << PlayerPos.x << ", " << PlayerPos.y << std::endl;
	//std::cout << "CurrentChunck: " << CurrentChunck << std::endl;

	DrawChunk(CurrentChunck, RelativeX, (int)PlayerPos->y, xRange, yRange, FullRange, &mesh, true);
	SDL_RenderGeometry(Renderer, nullptr, mesh.Vertices, xRange * yRange * 4, mesh.Indices, xRange * yRange * 6);
	//PrintChunk(CurrentChunck, 0, 0, xRange, 64, FullRange);


	if (RelativeX > 0)
	{
		ChunkGenerator(CurrentChunck + 1);
		xRange = RelativeX;
		RelativeX = 0;
		CurrentChunck++;

		DrawChunk(CurrentChunck, RelativeX, (int)PlayerPos->y, xRange, yRange, FullRange, &mesh, false);
		SDL_RenderGeometry(Renderer, nullptr, mesh.Vertices, xRange * yRange * 4, mesh.Indices, xRange * yRange * 6);
		//PrintChunk(CurrentChunck, RelativeX, PlayerPos.y, xRange, yRange, FullRange);
	}

}
void DrawPlayer(SDL_Renderer* Renderer) {
	std::lock_guard<std::mutex> lock(myMutex);
	int dx = (int)(PlayerPos[1].x - PlayerPos[0].x);
	int dy = (int)(PlayerPos[1].y - PlayerPos[0].y);

	bool InsideX = std::abs(dx) <= (FullRange / 2);
	if (InsideX) {
		int RelativeX = (FullRange / 2 - 1 + dx) * BlockSize;
		int RelativeY = (yRange / 2 - 2 - dy) * BlockSize;

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
		(float)(FullRange / 2 - 1) * BlockSize,
		(float)(yRange / 2 - 2) * BlockSize,
		(float)BlockSize,
		(float)BlockSize * 2
	};
	SDL_RenderFillRect(Renderer, &PlayerRect);
}
int Ui()
{
	int width = 600;
	int height = 400;

	Size(width, height, yRange, FullRange);

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
	Slot Inventory[8] = { {60, 1}, {5 , 5}, 0, 0, 0, 0, 0, 0 };
	int InventorySlot = 0;

	while (Running){
		myMutex.lock();
		UpdatePlayerPos((int)PlayerPos[0].x, (int)PlayerPos[0].y, serverSocket);

		PlayerDirection.x = 0;
		bool OnGround = Collition(&PlayerPos[0], { 0, -1 }, FullRange, yRange, false, false);
		myMutex.unlock();

		if (!OnGround) {
			PlayerDirection.y -= 0.5f;
		} else {
			PlayerDirection.y = 0;
		}

		while (SDL_PollEvent(&Event)) {
			if (Event.type == SDL_EVENT_QUIT) {
				myMutex.lock();
				Running = false;
				myMutex.unlock();
				break;
			} else if (Event.type == SDL_EVENT_WINDOW_RESIZED) {
				width = Event.window.data1;
				height = Event.window.data2;

				Size(width, height, yRange, FullRange);
			}

			if (Event.type == SDL_EVENT_KEY_DOWN)
			{
				SDL_Keycode KeyPressed = Event.key.scancode;

				if (KeyPressed == SDL_SCANCODE_ESCAPE) {
					myMutex.lock();
					Running = false;
					myMutex.unlock();
					break;
				}
				else if (KeyPressed == SDL_SCANCODE_F11) {
					FullScreen = !FullScreen;
					SDL_SetWindowFullscreen(Window, FullScreen);
				}
			}
			else if (Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
				if (Event.button.button == SDL_BUTTON_LEFT) {
					myMutex.lock();
					short Type = 0;
					PlaceBlock(0, { Event.button.x, Event.button.y }, yRange, PlayerPos[0], &Type);
					UpdateBlock(0, (int)Event.button.x, (int)Event.button.y, serverSocket);
					myMutex.unlock();
					if (Type != 0)
					{
						Inventory[FindSlot(Inventory, Type)].Amount++;
						SimulateWater((int)((Event.button.x / BlockSize) + PlayerPos[0].x) / 16);

					}
				}
				else if (Event.button.button == SDL_BUTTON_RIGHT && Inventory[InventorySlot].Amount > 0) {
					short Type = NULL;
					myMutex.lock();
					if (PlaceBlock(Inventory[InventorySlot].Type, { Event.button.x, Event.button.y }, yRange, PlayerPos[0], &Type))
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
					myMutex.unlock();
				}
			}
		}

		Input(&PlayerDirection, OnGround, &InventorySlot, &PlayerPos[0]);

		PlayerDirection.x = SDL_clamp(PlayerDirection.x, -1, 1);
		PlayerDirection.y = SDL_clamp(PlayerDirection.y, -1, 1);
		myMutex.lock();

		if (PlayerDirection.x != 0  && !Collition(&PlayerPos[0], { PlayerDirection.x, 0 }, FullRange, yRange, false, false)){
			PlayerPos[0].x += PlayerDirection.x;
		}
		if (PlayerDirection.y != 0 && !Collition(&PlayerPos[0], { 0, PlayerDirection.y }, FullRange, yRange, false, false)) {
			PlayerPos[0].y += PlayerDirection.y;
		}


		SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
		SDL_RenderClear(Renderer);
		

		DrawBG(Renderer, &PlayerPos[0]);
		ShowInventor(Renderer, width, height, Inventory, InventorySlot);

		PlayerPos[0].x = SDL_clamp(PlayerPos[0].x, 9, 1600 - (int)(FullRange / 2));
		PlayerPos[0].y = SDL_clamp(PlayerPos[0].y, -4, 64 - yRange);
		
		myMutex.unlock();

		DrawPlayer(Renderer);

		SDL_RenderPresent(Renderer);
		SDL_Delay(1000 / 10);
	}
	SDL_DestroyRenderer(Renderer);
	SDL_DestroyWindow(Window);
	SDL_Quit();
	CloseServer();

	return 0;
}
int GameSetUp(bool GenerateSeed) 
{
	if (GenerateSeed)
	{
		srand((unsigned int)time(0));

		seed = rand();
	}
	else
	{
		GetSeed();
	}

	std::cout << "Seed: " << seed << std::endl;

	srand(seed);
	SetGradients();

	return 0;
}

int main(int Argc, char* Argv[])
{
	if (SetUpServer() != 0)
	{
		std::cout << "Error setting up server." << std::endl;
	}

	std::thread multiplayerThread(MultiPlayer);
	std::thread acceptThread(AcceptClients);

	GameSetUp(Side == 0);

	std::thread UiThread(Ui);

	multiplayerThread.join();
	acceptThread.join();
	UiThread.join();

	if (!Running) {
		std::cout << "Exiting game..." << std::endl;
		multiplayerThread.detach();
		multiplayerThread.~thread();

		acceptThread.detach();
		acceptThread.~thread();

		UiThread.detach();
		UiThread.~thread();
	}

	return 0;
}