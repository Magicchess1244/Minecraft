#include "Renderer.h"
#include "GameClient.h"

const double FOV = tan((45 * (PI / 180)) / 2.0f);
const double Znear = 1 / FOV;
constexpr float Zfar = 1000.0f;
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
		Vector3 relToScreen = ((verts[i] + blocks) - player.Position);

		Vector3 localPos = this->rotate(relToScreen, player.Rotation);

		//std::cout << " 3D:\t Px: " << Px << " Py: " << Py << " Pz: " << Pz << std::endl;

		double screenX = localPos.x;
		double screenY = localPos.y;

		if (localPos.z <= Znear) localPos.z = Znear;

		screenX = (localPos.x / (localPos.z * FOV)) * BlockPixelSize + ScreenSize.x / 2.0f;
		screenY = (localPos.y / (localPos.z * FOV)) * BlockPixelSize + ScreenSize.y / 2.0f;
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
void Renderer::RenderChunk(ChunkPrefab& chunk, Player& player) {
	Mesh mesh = {};
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
		if (CameraDir.Dot(Direction[face.side]) >= 0.5) continue;
		Vector3 Max, Min;
		Vector3 local[4];
		for (int j = 0; j < 4; j++) {
			Vector3 worldFacePos = face.blockPos + Verts[face.side][j];
			local[j] = rotate((worldFacePos - player.Position), player.Rotation);
			if (j == 0) {
				Max = Min = local[j];
			}
			else {
				Max = Max.Max(local[j]);
				Min = Min.Min(local[j]);
			}
		}
		AABB volume(Min, Max);
		if (Max.z < Znear) continue;
		if (!volume.isOnFrustum(this->frustum, local)) continue;
		Faces.push_back({ face.blockPos, face.side, face.blockID, Max.z, face.blockID == 5 });
	}
	std::sort(Faces.begin(), Faces.end(), [](const DrawnFace& a, const DrawnFace& b) {
		if (a.maxZ != b.maxZ) return a.maxZ > b.maxZ;
		if (a.Transparent != b.Transparent) return !a.Transparent && b.Transparent;
		return false; // consider them equal if both fields match
		});

	// Vertex + Index setup
		Uint32 vertexSize = sizeof(SDL_Vertex) * mesh.Vertices.size();
		Uint32 indexSize = sizeof(Uint32) * mesh.Indices.size();

		// Create vertex buffer
		SDL_GPUBufferCreateInfo vertexInfo = {};
		vertexInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
		vertexInfo.size = vertexSize;
		SDL_GPUBuffer* Vertex = SDL_CreateGPUBuffer(this->GPU, &vertexInfo);

		SDL_GPUTransferBufferCreateInfo vertexTransferInfo = {};
		vertexTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		vertexTransferInfo.size = vertexSize;
		SDL_GPUTransferBuffer* vTransfer = SDL_CreateGPUTransferBuffer(this->GPU, &vertexTransferInfo);

		void* vMapped = SDL_MapGPUTransferBuffer(this->GPU, vTransfer, false);
		memcpy(vMapped, mesh.Vertices.data(), vertexSize);
		SDL_UnmapGPUTransferBuffer(this->GPU, vTransfer);

		SDL_GPUBufferBinding vertexBinding = {};
		vertexBinding.buffer = Vertex;
		vertexBinding.offset = 0;

		// Create index buffer
		SDL_GPUBufferCreateInfo indexInfo = {};
		indexInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
		indexInfo.size = indexSize;
		SDL_GPUBuffer* Index = SDL_CreateGPUBuffer(this->GPU, &indexInfo);

		SDL_GPUTransferBufferCreateInfo indexTransferInfo = {};
		indexTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		indexTransferInfo.size = indexSize;
		SDL_GPUTransferBuffer* iTransfer = SDL_CreateGPUTransferBuffer(this->GPU, &indexTransferInfo);

		void* iMapped = SDL_MapGPUTransferBuffer(this->GPU, iTransfer, false);
		memcpy(iMapped, mesh.Indices.data(), indexSize);
		SDL_UnmapGPUTransferBuffer(this->GPU, iTransfer);

		SDL_GPUBufferBinding indexBinding = {};
		indexBinding.buffer = Index;
		indexBinding.offset = 0;

		// Draw
		SDL_BindGPUVertexBuffers(this->pass, 0, &vertexBinding, 1);
		SDL_BindGPUIndexBuffer(this->pass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
		SDL_DrawGPUIndexedPrimitives(this->pass, mesh.Indices.size(), 1, 0, 0, 0);
}
void Renderer::DrawTerrain(Player& player) {
	std::vector<std::thread> threads;
	std::cout << "Nigga" << std::endl;
	int chunksPerAxis = 5;

	int threadIndex = 0;
	Vector3 PlayerChunk = (player.Position / 32).Truncate();
	for (int i = -0; i <= 0; i++) {
		for (int j = -0; j <= 0; j++) {
			Vector3 Chunk = { (double)i, 0, (double)j };
			Chunk += PlayerChunk;

			threads.emplace_back(
				&Renderer::RenderChunk,
				this,
				std::ref(chunkManager.get_chunk(Chunk)),
				std::ref(player)
			);

			threadIndex++;
		}
	}

	for (auto& t : threads) {
		if (t.joinable()) t.join();
	}
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
void Renderer::Stats(Player& player)
{
	std::string text = std::to_string(player.Position.x) + ", " + std::to_string(player.Position.y) + ", " + std::to_string(player.Position.z);
	SDL_Color White = { 200, 200, 200 };

	if (!this->font) {
		std::cerr << "Font not loaded!\n";
		return;
	}

	if (text.empty()) return;

	SDL_Surface* surface = TTF_RenderText_Blended(this->font, text.c_str(), text.length(), White);
	if (!surface) {
		std::cerr << "TTF_RenderText_Blended failed: \n";
		return;
	}

	//SDL_Texture* texture = SDL_CreateTextureFromSurface(this->renderer, surface);
	if (!texture) {
		std::cerr << "SDL_CreateTextureFromSurface failed: " << SDL_GetError() << "\n";
		SDL_DestroySurface(surface);
		return;
	}

	SDL_FRect dstRect{ 10, 10, (float)surface->w, (float)surface->h };
	//SDL_RenderTexture(this->renderer, texture, NULL, &dstRect);

	//SDL_DestroyTexture(texture);
	//SDL_DestroySurface(surface);
}
void Renderer::MainRenderLoop(std::vector<Slot>& inventory, int inventorySlot, std::vector<Player>& players)
{
	while (SDL_PollEvent(&this->event)) {
		switch (this->event.type) {
		case SDL_EVENT_QUIT:
			// Handle quitting the game
			this->gameClient.Quit();
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
				this->gameClient.Quit();
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

	std::cout << "Nigga2" << std::endl;

	SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(this->GPU);

	// 2. Get the current window framebuffer
	SDL_GPUTexture* swap_texture;
	SDL_WaitAndAcquireGPUSwapchainTexture(cmd, this->window, &swap_texture, &this->Width, &this->Height);

	SDL_GPUColorTargetInfo colorInfo = { 0 };
	colorInfo.texture = swap_texture;
	colorInfo.clear_color = SDL_FColor{ 1.0f, 0.0f, 0.0f, 1.0f };
	colorInfo.load_op = SDL_GPU_LOADOP_CLEAR;
	colorInfo.store_op = SDL_GPU_STOREOP_STORE;
	
	this->pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, NULL);

	//SDL_SetGPUViewport(pass, NULL);
	std::cout << "Nigga3\n";


	//DrawTerrain(players[0]);
	//Stats(player);
	//DrawBG(renderer, players[0],{ (double)width, (double)height, 0}, texture);
	//ChunckManager::ShowInventor(renderer, width, height, std::ref(inventory), inventorySlot, font);
	//DrawPlayer(renderer, Range, std::ref(players));
	//SDL_RenderPresent(this->renderer);
	//SDL_Delay(1000 / 10);
	std::cout << "Nigga4\n";

	SDL_EndGPURenderPass(this->pass);
	std::cout << "Nigga5\n";
	if(!SDL_SubmitGPUCommandBuffer(cmd)) {
		std::cout << "Heil Puigdemont\n";
	}
	//SDL_GL_SwapWindow(this->window);
}

Renderer::Renderer(GameClient& gameClient): gameClient(gameClient), chunkManager()
{
	this->Width = 0;
	this->Height = 0;

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::cout << "Error initializing SDL: " << SDL_GetError() << std::endl;
		//assert(false);
	}
	else {
		std::cout << "SDL initialized successfully." << std::endl;
	}

	this->window = SDL_CreateWindow("Bit Miner", 600, 400, SDL_WINDOW_RESIZABLE);
	if (this->window == nullptr) {
		std::cout << "Error creating window: " << SDL_GetError();
		SDL_Quit();
		assert(false);
	}

	// Use correct shader format for Direct3D (DXIL)
	this->GPU = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL, false, "direct3d12");  // or "d3d11"
	const char* err = SDL_GetError();
	printf("SDL GPU creation failed: %s\n", err);

	if (SDL_ClaimWindowForGPUDevice(this->GPU, this->window)) {
		std::cout << "Error claiming window for GPU device: " << SDL_GetError() << std::endl;
	}

	// Initialize SDL_ttf
	if (!TTF_Init()) {
		std::cerr << "TTF_Init failed: \n";
		SDL_Quit();
		assert(false);
	}

	/*
	this->font = TTF_OpenFont("C:\\Users\\pumu\\source\\repos\\2Dminecraft\\x64\\Release\\Quantico-Bold.ttf", 14);
	if (!this->font) {
		std::cerr << "Font is null!" << std::endl;
	}

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

	SDL_Surface* surface = SDL_LoadBMP("C:\\Users\\pumu\\source\\repos\\2Dminecraft\\x64\\Release\\Textures.bmp");
	if (!surface) {
		std::cerr << "SDL_LoadBMP failed: " << SDL_GetError() << std::endl;
	}

	//this->texture = SDL_CreateTextureFromSurface(this->renderer, surface);
	SDL_DestroySurface(surface);
	if (!texture) {
		std::cerr << "SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl;
	}
	*/
	this->terrainMesh = {};
	this->event = {};
	SDL_SetWindowRelativeMouseMode(window, true);

	double aspect = this->ScreenSize.x / this->ScreenSize.y;
	this->frustum = Frustum().createFrustumFromCamera(aspect, FOV, Znear, Zfar);
}