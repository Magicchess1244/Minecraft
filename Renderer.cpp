#include "Renderer.h"

const float FOV = 0.01f;//(double)tanf((45.0f / 2.0f) * (PI / 180.0f));
const Vector3 Verts[6][4] = {
	{// Front (-Z)
		{ -0.5, -0.5, -0.5 },
		{  0.5, -0.5, -0.5 },
		{ -0.5,  0.5, -0.5 },
		{  0.5,  0.5, -0.5 } },
	{// Back (+Z)
		{  0.5, -0.5, 0.5 },
		{ -0.5, -0.5, 0.5 },
		{  0.5,  0.5, 0.5 },
		{ -0.5,  0.5, 0.5 } },
	{// Right (+X)
		{ 0.5, -0.5, -0.5 },
		{ 0.5, -0.5,  0.5 },
		{ 0.5,  0.5, -0.5 },
		{ 0.5,  0.5,  0.5 } },
	{// Left (-X)
		{ -0.5, -0.5,  0.5 },
		{ -0.5, -0.5, -0.5 },
		{ -0.5,  0.5,  0.5 },
		{ -0.5,  0.5, -0.5 } },
	{// Top (+Y)
		{ -0.5, 0.5, -0.5 },
		{  0.5, 0.5, -0.5 },
		{ -0.5, 0.5,  0.5 },
		{  0.5, 0.5,  0.5 } },
	{// Bottom (-Y)
		{ -0.5, -0.5, -0.5 },
		{  0.5, -0.5, -0.5 },
		{ -0.5, -0.5,  0.5 },
		{  0.5, -0.5,  0.5 }
	}
};
const Vector3 Direction[6] = {
	{ 0, 0, -1 }, // Front
	{ 0, 0, 1 },  // Back
	{ 1, 0, 0 },  // Right
	{ -1, 0, 0 }, // Left
	{ 0, 1, 0 },  // Top
	{ 0, -1, 0 }   // Bottom
};
const Color Colors[3] = {
	{0,0,0}, // Front / Back
	{5,5,5}, // Right / Left
	{10,10,10}, // Top / Bottom
};

SDL_FPoint Renderer::getUV(int tileIndex, int cornerX, int cornerY) {
	const int tileSize = 16;
	const int atlasSize = 64;
	const double pixelNudge = 0.25f; // or 0.25f if needed

	int tilesPerRow = atlasSize / tileSize;
	int tileX = tileIndex % tilesPerRow;
	int tileY = tileIndex / tilesPerRow;

	double u = (tileX * tileSize + cornerX * (tileSize - pixelNudge * 2) + pixelNudge) / (double)atlasSize;
	double v = (tileY * tileSize + cornerY * (tileSize - pixelNudge * 2) + pixelNudge) / (double)atlasSize;

	SDL_FPoint point = { (float)u, (float)v };
	return point;
}
Vector3 Renderer::rotate(const Vector3 pos, Vector3 Angle)
{
	Vector3 angleRadians = Angle.AngleToRadians();
	double cx = cos(angleRadians.x), sx = sin(angleRadians.x);
	double cy = cos(angleRadians.y), sy = sin(angleRadians.y);
	double cz = cos(angleRadians.z), sz = sin(angleRadians.z);

	Vector3 result = { 0 };

	result.x = pos.x * (cy * cz)
		+ pos.y * (-cy * sz)
		+ pos.z * (sy);

	result.y = pos.x * (sx * sy * cz + cx * sz)
		+ pos.y * (-sx * sy * sz + cx * cz)
		+ pos.z * (-sx * cy);

	result.z = pos.x * (-cx * sy * cz + sx * sz)
		+ pos.y * (cx * sy * sz + sx * cz)
		+ pos.z * (cx * cy);

	return result;
}
void Renderer::DrawFace(Mesh& mesh, Player& player, Vector3 blocks, int color, int Side)
{
	const Vector3* verts = Verts[Side];

	//std::cout << "\n \n";
	for (int i = 0; i < 4; i++) {
		Vector3 relToScreen = ((verts[i] + blocks) - player.Position) * this->BlockPixelSize;

		Vector3 localPos = this->rotate(relToScreen, player.Rotation);

		//std::cout << " 3D:\t Px: " << Px << " Py: " << Py << " Pz: " << Pz << std::endl;

		double screenX = localPos.x;
		double screenY = localPos.y;

		if (localPos.z <= 0.01f) localPos.z = 0.001f;

		screenX = (localPos.x / (localPos.z * FOV)) + ScreenSize.x / 2.0f;
		screenY = (localPos.y / (localPos.z * FOV)) + ScreenSize.y / 2.0f;
		screenY = ScreenSize.y - screenY;

		//std::cout << " 2D:\t Px: " << screenX << " Py: " << screenY << std::endl;

		Color faceColor = (this->chunkManager.GetBlock(color).Color + Colors[(int)(Side / 2)]);

		SDL_Vertex vertex = {
			{ screenX, screenY }, faceColor.ToSDL()
		};

		mesh.Vertices.push_back(vertex);
	}

	int vIndex = mesh.faces * 4;

	mesh.Indices.push_back(vIndex + 0);
	mesh.Indices.push_back(vIndex + 1);
	mesh.Indices.push_back(vIndex + 2);

	mesh.Indices.push_back(vIndex + 2);
	mesh.Indices.push_back(vIndex + 1);
	mesh.Indices.push_back(vIndex + 3);

	mesh.faces++;
}
void Renderer::RenderChunk(ChunkPrefab& chunk, Player& player, Mesh& mesh) {
	mesh.Vertices.clear();
	mesh.Indices.clear();
	mesh.faces = 0;
	
	Vector3 Radiants = player.Rotation.AngleToRadians();
		
	Vector3 CameraDir = {
		cos(Radiants.x) * sin(Radiants.y),  // X
		sin(Radiants.x),				    // Y
		cos(Radiants.x) * cos(Radiants.y)   // Z
	};

	int chunkX = (int)floor((player.Position.x) / 32);
	int chunkZ = (int)floor((player.Position.z) / 32);
	std::vector<DrawnFace> Faces;
	int index = 0;

	for (auto& face : chunk.allFaces) {
		if (CameraDir.Dot(Direction[face.side]) >= 0.2) continue;
		double maxZ = 0.0f;
		for (int j = 0; j < 4; j++) {
			Vector3 worldFacePos = face.blockPos + Verts[face.side][j];
			Vector3 local = rotate((worldFacePos - player.Position), player.Rotation);
			if (maxZ < local.z) maxZ = local.z;
		}
		if (maxZ <= 0.01f) continue;
		Faces.push_back({ face.blockPos, face.side, face.blockID, maxZ });
	}

	std::sort(Faces.begin(), Faces.end(), [](const DrawnFace& a, const DrawnFace& b) {
		return a.maxZ > b.maxZ;
		});

	for (const auto& face : Faces) {
		DrawFace(mesh, player, face.blockPos, face.blockID, face.side);
	}
}
void Renderer::DrawTerrain(Player& player) {
	/*	std::vector<std::thread> threads;
		std::vector<Mesh> threadMeshes((2 + 1) * (2 + 1));
		int threadIndex = 0;

		for (int i = -2; i <= 2; i++) {
			for (int j = -2; j <= 2; j++) {
				threads.emplace_back([=, &threadMeshes]() {
					Mesh& localMesh = threadMeshes[threadIndex];
					RenderChunk( BitMiner::get_chunk(i, j), player, localMesh);
					});

				threadIndex++;
			}
		}

		// Wait for threads to finish
		for (auto& t : threads) t.join();

		// Merge all meshes
		Mesh mesh;
		mesh.Vertices.clear();
		mesh.Indices.clear();
		mesh.faces = 0;

		for (const auto& m : threadMeshes) {
			int baseIndex = mesh.Vertices.size();
			mesh.Vertices.insert(mesh.Vertices.end(), m.Vertices.begin(), m.Vertices.end());

			for (int index : m.Indices)
				mesh.Indices.push_back(index + baseIndex);

			mesh.faces += m.faces;
		}

		std::cout << mesh.faces << " faces" << std::endl;
		SDL_RenderGeometry(renderer, texture, mesh.Vertices.data(), mesh.faces * 4, mesh.Indices.data(), mesh.faces * 6);
		*/

	return;
}
void Renderer::DrawPlayer(SDL_Renderer* Renderer, Vector3 Range, std::vector<Player>& PlayerPos)
{
	/* 2D
	//Other player
	for (const Player& Position : PlayerPos)
	{
		int dx = (int)(Position.Position.x - PlayerPos[0].Position.x);
		int dy = (int)(Position.Position.y - PlayerPos[0].Position.y);

		bool InsideX = std::abs(dx) <= (Range.x / 2);
		if (InsideX) {
			int RelativeX = (int)((Range.x / 2 - 1 + dx) * BlockPixelSize);
			int RelativeY = (int)((Range.y / 2 - 2 - dy) * BlockPixelSize);

			SDL_FRect OtherPlayerRect = {
				(double)RelativeX,
				(double)RelativeY,
				(double)BlockPixelSize,
				(double)BlockPixelSize * 2
			};
			SDL_SetRenderDrawColor(Renderer, (Uint8)SDL_clamp(PlayerPos[0].color.r, 0, 255), (Uint8)SDL_clamp(PlayerPos[0].color.g, 0, 255), (Uint8)SDL_clamp(PlayerPos[0].color.b, 0, 255), 255);
			SDL_RenderFillRect(Renderer, &OtherPlayerRect);
		}
	}

	// Your player
	SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].color.r, 0, 255), SDL_clamp(PlayerPos[0].color.g, 0, 255), SDL_clamp(PlayerPos[0].color.b, 0, 255), 255);
	SDL_FRect PlayerRect = {
		(double)(Range.x / 2 - 1) *  BlockPixelSize,
		(double)(Range.y / 2 - 2) * this->BlockPixelSize,
		(double)this->BlockPixelSize,
		(double)this->BlockPixelSize * 2
	};
	SDL_RenderFillRect(Renderer, &PlayerRect);

	SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].color.r + 90, 0, 255), SDL_clamp(PlayerPos[0].color.g + 90, 0, 255), SDL_clamp(PlayerPos[0].color.b + 90, 0, 255), 255);
	SDL_FRect InsidePlayerRect = {
		(double)(Range.x / 2 - 1) * this->BlockPixelSize + (this->BlockPixelSize * 0.1f),
		(double)(Range.y / 2 - 2) * this->BlockPixelSize + (this->BlockPixelSize * 0.1f),
		(double)(this->BlockPixelSize * 0.8f),
		(double)(this->BlockPixelSize * 0.9f) * 2
	};
	SDL_RenderFillRect(Renderer, &InsidePlayerRect);
	*/
}
void Renderer::MainRenderLoop(std::vector<Slot>& inventory, int inventorySlot, std::vector<Player>& players)
{
	while (SDL_PollEvent(&this->event)) {
		switch (this->event.type) {
		case SDL_EVENT_QUIT:
			// Handle quitting the game
			this->gameClient.~GameClient();
			// this->running = false;
			break;

		case SDL_EVENT_WINDOW_RESIZED:
			// Update screen size
			this->ScreenSize.x = this->event.window.data1;
			this->ScreenSize.y = this->event.window.data2;

			// Optionally update chunk manager here
			// ChunckManager::Size(...);
			break;

		case SDL_EVENT_KEY_DOWN: {
			SDL_Keycode key = this->event.key.scancode;

			if (key == SDL_SCANCODE_ESCAPE) {
				// this->Running = false;
				break;
			}
			else if (key == SDL_SCANCODE_F11) {
				// Toggle fullscreen
				this->fullScreen = !this->fullScreen;
				SDL_SetWindowFullscreen(this->window, this->fullScreen);
			}
			break;
		}

		case SDL_EVENT_MOUSE_BUTTON_DOWN: {
			// Handle mouse input
			if (this->event.button.button == SDL_BUTTON_LEFT) {
				// Handle block breaking
				short Type = 0;
				if (false /*ChunckManager::PlaceBlock(...)*/) {
					// short Slot = FindSlot(inventory, Type);
					// inventory[Slot].Amount++;
					// inventory[Slot].Type = Type;
					// ChunckManager::SimulateWater(...);
				}
			}
			else if (this->event.button.button == SDL_BUTTON_RIGHT && inventory[inventorySlot].Amount > 0) {
				// Handle block placing
				short Type = 0;
				if (false /*ChunckManager::PlaceBlock(...)*/) {
					inventory[inventorySlot].Amount--;

					if (inventory[inventorySlot].Amount == 0)
						inventory[inventorySlot].Type = 0;

					// ChunckManager::SimulateWater(...);
				}
			}
			break;
		}
		}
	}

	SDL_SetRenderDrawColor(this->renderer, 0, 178, 255, 255);
	SDL_RenderClear(this->renderer);



	/*
	Mesh mesh{};
	mesh.faces = 0;

	ChunckManager::Face(std::ref(mesh), players[0].Position, players[0].Rotation, { 0, 0, 0 }, Verts[1], 1, { (double)width, (double)height });
	ChunckManager::Face(std::ref(mesh), players[0].Position, players[0].Rotation, { 0, 0, 0 }, Verts[3], 2, { (double)width, (double)height });
	ChunckManager::Face(std::ref(mesh), players[0].Position, players[0].Rotation, { 0, 0, 0}, Verts[0], 3, {(double)width, (double)height});

	SDL_RenderGeometry(renderer, nullptr, mesh.Vertices.data(), mesh.faces * 4, mesh.Indices.data(), mesh.faces * 6);
	*/
	Player player = players[0];
	DrawTerrain(player);

	//DrawBG(renderer, players[0],{ (double)width, (double)height, 0}, texture);
	//ChunckManager::ShowInventor(renderer, width, height, std::ref(inventory), inventorySlot, font);

	//DrawPlayer(renderer, Range, std::ref(players));
	SDL_RenderPresent(this->renderer);
	SDL_Delay(1000 / 10);
}

Renderer::Renderer(GameClient& client)
{
	if (SDL_Init(SDL_INIT_VIDEO)) { // Fixing SDL_Init condition
		std::cout << "Error initializing SDL: " << SDL_GetError();
		assert(false);
	}
	else {
		std::cout << "SDL initialized successfully." << std::endl;
	}

	this->window = SDL_CreateWindow("Bit Miner", 600, 400, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (this->window == nullptr) { // Fixing pointer dereference
		std::cout << "Error creating window: " << SDL_GetError();
		SDL_Quit();
		assert(false);
	}

	this->renderer = SDL_CreateRenderer(this->window, NULL);
	if (this->renderer == nullptr) { // Fixing pointer dereference
		std::cout << "Error creating renderer: " << SDL_GetError();
		SDL_DestroyWindow(this->window);
		SDL_Quit();
		assert(false);
	}
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
		assert(false);
	}

	// Initialize SDL_ttf
	if (!TTF_Init()) {
		std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
		SDL_Quit();
		assert(false);
	}
	this->terrainMesh = {};
	this->event = {};
	this->chunkManager = ChunkManager();
	this->gameClient = client;
}