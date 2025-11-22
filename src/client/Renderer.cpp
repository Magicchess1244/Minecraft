#include "../../include/client/Renderer.hpp"
#include "../../include/client/GameClient.hpp"
#include <SDL3/SDL_gpu.h>


constexpr Uint32 vertexSize = sizeof(Vertex) * 4 * 16000;
constexpr Uint32 indexSize = sizeof(Uint32) * 6 * 16000;
const float FOV = tan((90 * (PI / 180)) / 2.0f);
const float Znear = 0.1f;
constexpr float Zfar = 50.0f;
constexpr int RenderDistance = 0;
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
	{ -1, 0, 0 },  // Right
	{ 1, 0, 0 }, // Left
	{ 0, -1, 0 },  // Top
	{ 0, 1, 0 }   // Bottom
};
const Color Colors[3] = {
	{0,0,0}, // Front / Back
	{5,5,5}, // Right / Left
	{10,10,10}, // Top / Bottom
};
Matrix Perspective(float tanHalfFov, float aspect, float Near, float Far) {
    const float f = 1.0f / tanHalfFov;
    Matrix m(4, 4, 0.0f);
    
    m(0, 0) = f / aspect;
    m(1, 1) = f;
    m(2, 2) = (Far + Near) / (Near - Far);
    m(3, 1) = (2.0f * Far * Near) / (Near - Far);
    m(2, 2) = -1.0f;
    m(3, 3) = 0.0f;  // IMPORTANT: This was missing!
    
    return m;
}

// Rotation matrices look correct
Matrix RotationX(float angleRad) {
    Matrix m = Matrix::Identity(4);
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);
    m(1, 1) = c;
    m(1, 2) = -s;
    m(2, 1) = s;
    m(2, 2) = c;
    return m;
}

Matrix RotationY(float angleRad) {
    Matrix m = Matrix::Identity(4);
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);
    m(0, 0) = c;
    m(0, 2) = s;
    m(2, 0) = -s;
    m(2, 2) = c;
    return m;
}

Matrix RotationZ(float angleRad) {
    Matrix m = Matrix::Identity(4);
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);
    m(0, 0) = c;
    m(0, 1) = -s;
    m(1, 0) = s;
    m(1, 1) = c;
    return m;
}

Matrix Rotation(float x, float y, float z) {
    return RotationY(y) * RotationX(x) * RotationZ(z);
}

Matrix Translation(float x, float y, float z) {
    Matrix m = Matrix::Identity(4);
    m(0, 3) = x;
    m(1, 3) = y;
    m(2, 3) = z;
    return m;
}

Matrix Scale(float x, float y, float z) {
    Matrix m = Matrix::Identity(4);
    m(0, 0) = x;
    m(1, 1) = y;
    m(2, 2) = z;
    return m;
}
Matrix LookAt(const Vector3& Rotation, const Vector3& Position) {
    // Build camera basis
    Vector3 zaxis = Rotation.Forward();
    Vector3 xaxis = Rotation.Right(); // Right
    Vector3 yaxis = Rotation.Up();           // Up
    
    Matrix Mrot = Matrix::Identity(4);
    
    // Rotation part
    Mrot(0, 0) = xaxis.x;
    Mrot(0, 1) = xaxis.y;
    Mrot(0, 2) = xaxis.z;
    Mrot(0, 3) = 1;
    
    Mrot(1, 0) = yaxis.x;
    Mrot(1, 1) = yaxis.y;
    Mrot(1, 2) = yaxis.z;
    Mrot(1, 3) = 1;
    
    Mrot(2, 0) = zaxis.x;
    Mrot(2, 1) = zaxis.y;
    Mrot(2, 2) = zaxis.z;
    Mrot(2, 3) = 1;
    
    Mrot(3, 0) = 0;
    Mrot(3, 1) = 0;
    Mrot(3, 2) = 0;
    Mrot(3, 3) = 1;
    

	Matrix Mtrans = Matrix::Identity(4);
    
    // Rotation part
    Mtrans(0, 0) = 1;
    Mtrans(0, 1) = 0;
    Mtrans(0, 2) = 0;
    Mtrans(0, 3) = -Position.x;
    
    Mtrans(1, 0) = 0;
    Mtrans(1, 1) = 1;
    Mtrans(1, 2) = 0;
    Mtrans(1, 3) = -Position.y;
    
    Mtrans(2, 0) = 0;
    Mtrans(2, 1) = 0;
    Mtrans(2, 2) = 1;
    Mtrans(2, 3) = -Position.z;
    
    Mtrans(3, 0) = 0;
    Mtrans(3, 1) = 0;
    Mtrans(3, 2) = 0;
    Mtrans(3, 3) = 1;

    return Mrot * Mtrans;
}
bool isFaceInFrustum(const Frustum& frustum, const Vector3 faceVerts[4]) {
    const Plane planes[6] = {frustum.nearFace, frustum.farFace, frustum.rightFace,
                             frustum.leftFace, frustum.topFace, frustum.bottomFace};

    // For each plane, check if all 4 vertices are outside
    for (const auto& plane : planes) {
        int outsideCount = 0;
        for (int i = 0; i < 4; ++i) {
            if (plane.getSignedDistanceToPlane(faceVerts[i]) < 0.0f) {
                ++outsideCount;
            }
        }
        if (outsideCount == 4) {
            return false;  // entire face is outside this plane
        }
    }

    return true;  // at least partially inside all planes
}
SDL_FPoint Renderer::getUV(int tileIndex, int cornerX, int cornerY) {
	const int tileSize = 16;
	const int atlasSize = 64;
	const float pixelNudge = 0.25f; // or 0.25f if needed

	int tilesPerRow = atlasSize / tileSize;
	int tileX = tileIndex % tilesPerRow;
	int tileY = tileIndex / tilesPerRow;

	float u = (tileX * tileSize + cornerX * (tileSize - pixelNudge * 2) + pixelNudge) / (float)atlasSize;
	float v = (tileY * tileSize + cornerY * (tileSize - pixelNudge * 2) + pixelNudge) / (float)atlasSize;

	SDL_FPoint point = { (float)u, (float)v };
	return point;
}
Vector3 Renderer::rotate(const Vector3& pos, const Vector3& Angle)
{
    Vector3 angleRadians = Angle.AngleToRadians();
    float cx = cos(angleRadians.x), sx = sin(angleRadians.x);
    float cy = cos(angleRadians.y), sy = sin(angleRadians.y);
    float cz = cos(angleRadians.z), sz = sin(angleRadians.z);
    
    Vector3 result;
    
    // Row 1
    result.x = pos.x * (cy * cz) 
             + pos.y * (-cy * sz) 
             + pos.z * (sy);
    
    // Row 2
    result.y = pos.x * (sx * sy * cz + cx * sz) 
             + pos.y * (-sx * sy * sz + cx * cz) 
             + pos.z * (-sx * cy);
    
    // Row 3
    result.z = pos.x * (-cx * sy * cz + sx * sz) 
             + pos.y * (cx * sy * sz + sx * cz) 
             + pos.z * (cx * cy);
    
    return result;
}
void Renderer::DrawTestTriangle(Mesh* mesh, Vertex* Vertexdata, Uint32* Indexdata) {
    // Simple triangle in normalized device coordinates
    Vector3 positions[3] = {
        { -0.2f, -0.2f, 0.5f},  // Bottom left
        { 0.2f, -0.2f, 0.5f},  // Bottom right
        { 0.0f,  0.2f, 0.5f}   // Top center
    };
    
    Vector3 colors[3] = {
        {1.0f, 0.0f, 0.0f},    // Red
        {0.0f, 1.0f, 0.0f},    // Green
        {0.0f, 0.0f, 1.0f}     // Blue
    };
    
    for (int i = 0; i < 3; i++) {
        Vertex vertex = {positions[i], colors[i]};
        Vertexdata[i] = vertex;
    }
    
    // Simple triangle indices
    Indexdata[0] = 0;
    Indexdata[1] = 1;
    Indexdata[2] = 2;
    
    mesh->faces = 1;  // One triangle = 1 face
}
void Renderer::DrawFace(Player& player, Vector3 blocks, int blockID, int Side, Mesh* mesh,
                        Vertex* Vertexdata, Uint32* Indexdata) {
    const Vector3* verts = Verts[Side];
    const int baseVertex = mesh->faces * 4;
    const int baseIndex = mesh->faces * 6;

    for (int i = 0; i < 4; i++) {
        // 1. Transform to camera space (relative to player)
        Vector3 worldPos = (verts[i] + blocks);// - player.Position;
        
        // 2. Apply camera rotation (IMPORTANT - you need this!)
        //worldPos = rotate(worldPos, player.Rotation);
        
        // 3. Check if vertex is behind camera
        //if (worldPos.z <= Znear) {
            // Clamp to near plane to avoid division by zero
        //    worldPos.z = Znear;
        //}
        // 4. Manual perspective projection
        //float projectedX = worldPos.x / (worldPos.z * FOV);
        //float projectedY = worldPos.y / (worldPos.z * FOV);
        
        // 5. Proper depth calculation (don't set to 1!)
        //float projectedZ = (worldPos.z - Znear) / (Zfar - Znear);
        // Clamp to valid depth range [0, 1]
        //Vector3 finalPos = {
        //    projectedX,   // -1 to 1 (left to right)
        //    projectedY,   // -1 to 1 (bottom to top)
        //    projectedZ    // 0 to 1 (near to far)
        //};
        
        Vector3 Color = BlockDef[blockID].color.ToFloat() - Colors[(int)(Side/2)].ToFloat();
        Vertex vertex = {worldPos, Color};
        Vertexdata[baseVertex + i] = vertex;
    	std::cout << "Point pos,  x: " << worldPos.x << "; y: " << worldPos.y << "; z: " << worldPos.z << std::endl;
        // Debug output
		/*
        if (finalPos.x > -1 && finalPos.x < 1 && finalPos.y > -1 && finalPos.y < 1) {
            std::cout << "Point in screen: " << finalPos.x << ", " << finalPos.y 
                      << ", " << finalPos.z << std::endl;
        }
		*/
    }
    
    Indexdata[baseIndex] = baseVertex + 0;
    Indexdata[baseIndex + 1] = baseVertex + 1;
    Indexdata[baseIndex + 2] = baseVertex + 2;
    
    Indexdata[baseIndex + 3] = baseVertex + 2;
    Indexdata[baseIndex + 4] = baseVertex + 1;
    Indexdata[baseIndex + 5] = baseVertex + 3;
    
    mesh->faces++;
}
void Renderer::RenderChunk(ChunkPrefab& chunk, Player& player, int NumChunk)
{
	auto* mesh = &this->Terrain[NumChunk];
	mesh->faces = 0;
	
	std::vector<DrawnFace> Faces;
	int index = 0;

	for (int i = 0; i < chunk.allFaces.size(); i++) {
		auto* face = &chunk.allFaces[i];
		//std::cout << face << std::endl;
		//if (player.Rotation.Forward().Dot(Direction[face->side]) >= 0.3) continue;
		
		Vector3 Max, Min;
		Vector3 local[4];
		for (int j = 0; j < 4; j++) {
			Vector3 worldFacePos = face->blockPos + Verts[face->side][j];
			local[j] = rotate((worldFacePos - player.Position), player.Rotation);
			if (j == 0) {
				Max = Min = local[j];
			}
			else {
				Max = Max.Max(local[j]);
				Min = Min.Min(local[j]);
			}
		}
		if (Max.z < Znear) continue;
		//if (!isFaceInFrustum(this->frustum, local)) continue;
		//std::cout << Max.x << std::endl;
		
		Faces.push_back({ face->blockPos, face->side, face->blockID, 1, face->blockID == 5 });
	}
	std::cout << "faces: " << Faces.size() << std::endl;
	Vertex* Vertexdata = (Vertex*)SDL_MapGPUTransferBuffer(this->GPU, mesh->VertextransferBuffer, false);
	Uint32* Indexdata = (Uint32*)SDL_MapGPUTransferBuffer(this->GPU, mesh->IndextransferBuffer, false);

	for (auto& Face : Faces) {
		DrawFace(player, Face.blockPos, Face.blockID, Face.side, mesh, Vertexdata, Indexdata);
	}
	

    //DrawTestTriangle(mesh, Vertexdata, Indexdata);
	//DrawFace(player, {0,0,0}, 0, 1, mesh, Vertexdata, Indexdata);
	//---------------Vertex-----------------

	SDL_UnmapGPUTransferBuffer(this->GPU, mesh->VertextransferBuffer);
	
	// where is the data
	SDL_GPUTransferBufferLocation Vertexlocation{};
	Vertexlocation.transfer_buffer = mesh->VertextransferBuffer;
	Vertexlocation.offset = 0; // start from the beginning

	// where to upload the data
	SDL_GPUBufferRegion Vertexregion{};
	Vertexregion.buffer = mesh->VertexBuffer.buffer;
	Vertexregion.size = vertexSize;
	Vertexregion.offset = 0;

	// upload the data
	SDL_UploadToGPUBuffer(this->copyPass, &Vertexlocation, &Vertexregion, true);

	//---------------Index-----------------

	SDL_UnmapGPUTransferBuffer(this->GPU, mesh->IndextransferBuffer);
	
	// where is the data
	SDL_GPUTransferBufferLocation Indexlocation{};
	Indexlocation.transfer_buffer = mesh->IndextransferBuffer;
	Indexlocation.offset = 0; // start from the beginning

	// where to upload the data
	SDL_GPUBufferRegion Indexregion{};
	Indexregion.buffer = mesh->IndexBuffer.buffer;
	Indexregion.size = indexSize;
	Indexregion.offset = 0;

	// upload the data
	SDL_UploadToGPUBuffer(this->copyPass, &Indexlocation, &Indexregion, true);
}
void Renderer::DrawTerrain(Player& player) {
	/*
	std::vector<std::thread> threads;
	std::cout << "Nigga" << std::endl;

	int threadIndex = 0;
	Vector3 PlayerChunk = (player.Position / 32).Truncate();
	for (int i = -RenderDistance; i <= RenderDistance; i++) {
		for (int j = -RenderDistance; j <= RenderDistance; j++) {
			Vector3 Chunk = { (float)i, 0, (float)j };
			Chunk += PlayerChunk;

			threads.emplace_back(
				&Renderer::RenderChunk,
				this,
				std::ref(chunkManager.get_chunk(Chunk)),
				std::ref(player),
				0
			);

			threadIndex++;
		}
	}

	for (auto& t : threads) {
		if (t.joinable()) t.join();
	}
	*/
	Vector3 Pos;
	RenderChunk(std::ref(chunkManager.get_chunk(Pos)), std::ref(player), 0);
}
/*
void Renderer::DrawPlayer(SDL_Renderer* Renderer, Vector3 Range, std::vector<Player>& PlayerPos)
{
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
				(float)RelativeX,
				(float)RelativeY,
				(float)BlockPixelSize,
				(float)BlockPixelSize * 2
			};
			SDL_SetRenderDrawColor(Renderer, (Uint8)SDL_clamp(PlayerPos[0].color.r, 0, 255), (Uint8)SDL_clamp(PlayerPos[0].color.g, 0, 255), (Uint8)SDL_clamp(PlayerPos[0].color.b, 0, 255), 255);
			SDL_RenderFillRect(Renderer, &OtherPlayerRect);
		}
	}

	// Your player
	SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].color.r, 0, 255), SDL_clamp(PlayerPos[0].color.g, 0, 255), SDL_clamp(PlayerPos[0].color.b, 0, 255), 255);
	SDL_FRect PlayerRect = {
		(float)(Range.x / 2 - 1) *  BlockPixelSize,
		(float)(Range.y / 2 - 2) * this->BlockPixelSize,
		(float)this->BlockPixelSize,
		(float)this->BlockPixelSize * 2
	};
	SDL_RenderFillRect(Renderer, &PlayerRect);

	SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].color.r + 90, 0, 255), SDL_clamp(PlayerPos[0].color.g + 90, 0, 255), SDL_clamp(PlayerPos[0].color.b + 90, 0, 255), 255);
	SDL_FRect InsidePlayerRect = {
		(float)(Range.x / 2 - 1) * this->BlockPixelSize + (this->BlockPixelSize * 0.1f),
		(float)(Range.y / 2 - 2) * this->BlockPixelSize + (this->BlockPixelSize * 0.1f),
		(float)(this->BlockPixelSize * 0.8f),
		(float)(this->BlockPixelSize * 0.9f) * 2
	};
	SDL_RenderFillRect(Renderer, &InsidePlayerRect);
	
}
*/
/*
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
*/
void Renderer::UpdateViewportAndProjection() {
    // Get the actual drawable size (important for high-DPI displays)
    int drawableWidth, drawableHeight;
    SDL_GetWindowSizeInPixels(this->window, &drawableWidth, &drawableHeight);
    
    this->Width = drawableWidth;
    this->Height = drawableHeight;
    
    std::cout << "Updating viewport to: " << drawableWidth << "x" << drawableHeight << std::endl;
    
    // Recreate depth texture with new size
    if (this->DepthTexture) {
        SDL_ReleaseGPUTexture(this->GPU, this->DepthTexture);
    }
    this->DepthTexture = CreateDepthTexture(drawableWidth, drawableHeight);
    
    // Update frustum for new aspect ratio
    float aspect = (float)drawableWidth / (float)drawableHeight;
    this->frustum = Frustum().createFrustumFromCamera(aspect, FOV, Znear, Zfar);
}

void Renderer::MainRenderLoop(std::vector<Slot>& inventory, int inventorySlot, std::vector<Player>& players)
{
	while (SDL_PollEvent(&this->event)) {
		switch (this->event.type) {
			case SDL_EVENT_QUIT:
				// Handle quitting the game
				this->gameClient.Quit();
				break;

			case SDL_EVENT_WINDOW_RESIZED: {
                
                    this->ScreenSize.x = this->event.window.data1;
                    this->ScreenSize.y = this->event.window.data2;
                    std::cout << "Window resized to: " << this->event.window.data1 
                              << "x" << this->event.window.data2 << std::endl;
					UpdateViewportAndProjection();

            }

            case SDL_EVENT_KEY_DOWN: {
                SDL_Keycode key = this->event.key.scancode;

                if (key == SDL_SCANCODE_ESCAPE) {
                    this->gameClient.Quit();
                    break;
                }
                else if (key == SDL_SCANCODE_F11) {
                    
                    this->fullScreen = !this->fullScreen;
                    std::cout << "Toggling fullscreen: " << (this->fullScreen ? "ON" : "OFF") << std::endl;
                    
                    SDL_SetWindowFullscreen(this->window, this->fullScreen);
                    
                    // Update screen size immediately
                    int w, h;
                    SDL_GetWindowSize(this->window, &w, &h);
                    this->ScreenSize.x = w;
                    this->ScreenSize.y = h;
                    std::cout << "New screen size: " << w << "x" << h << std::endl;
					UpdateViewportAndProjection();
                }
                break;
            }
		}
	}
	
	this->cmdCopy = SDL_AcquireGPUCommandBuffer(this->GPU);

	this->copyPass = SDL_BeginGPUCopyPass(this->cmdCopy);
	DrawTerrain(players[0]);
	
	SDL_EndGPUCopyPass(this->copyPass);
	if(!SDL_SubmitGPUCommandBuffer(this->cmdCopy)) {
		std::cout << "Heil la memoria de Puigdemont\n";
		return;
	}
	
	this->cmdRender = SDL_AcquireGPUCommandBuffer(this->GPU);

	SDL_GPUTexture* swap_texture;
	SDL_WaitAndAcquireGPUSwapchainTexture(this->cmdRender, this->window, &swap_texture, &this->Width, &this->Height);

	if (swap_texture == NULL) {
		std::cout << "La swap_texture no s'ha fet be\n";
		SDL_SubmitGPUCommandBuffer(this->cmdRender);
	}
	SDL_GPUColorTargetInfo color_target_info;
	SDL_zero(color_target_info);
	color_target_info.clear_color = SDL_FColor{ 0.1f, 0.79f, 1.0f, 1.0f };
	color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
	color_target_info.store_op = SDL_GPU_STOREOP_STORE;
	color_target_info.texture = swap_texture;

	
	Player player = players[0];
    Matrix model = Rotation(0, 0, 0);
	
    Matrix view = LookAt(player.Rotation, player.Position);
	
    float aspect = (float)this->Width / (float)this->Height;
    Matrix proj = Perspective(FOV, aspect, Znear, Zfar);

    // Final MVP matrix
    Matrix mvp = proj * view * model;
	
	mvp.print();
    SDL_PushGPUVertexUniformData(this->cmdRender, 0, mvp.getColumnMajorData().data(),
                                 sizeof(float) * mvp.getColumnMajorData().size());
	
	this->pass = SDL_BeginGPURenderPass(this->cmdRender, &color_target_info, 1, NULL);
	SDL_BindGPUGraphicsPipeline(this->pass, this->graphicsPipeline);

	for (int i = 0; i < this->Terrain.size(); i++)
	{
		auto* mesh = &this->Terrain[i];
		//std::cout << "Mesh pointer: " << mesh << "\tVertexBuffer pointer: " << &mesh->VertexBuffer << "\t Index Buffer pointer: " << &mesh->IndexBuffer << std::endl;
		//std::cout << "Pass pointer: " << this->pass << "\t Index size: " << SDL_GPU_INDEXELEMENTSIZE_32BIT << "\t Amount of faces: " << mesh->faces << std::endl;
		//std::cout << "Terrain size: " << this->Terrain.size() << "Drawing Chunk[" << i << "]\n" << std::endl;
		SDL_BindGPUVertexBuffers(this->pass, 0, &mesh->VertexBuffer, 1);
		SDL_BindGPUIndexBuffer(this->pass, &mesh->IndexBuffer, SDL_GPU_INDEXELEMENTSIZE_32BIT);
		SDL_DrawGPUIndexedPrimitives(this->pass, mesh->faces * 6, 1, 0, 0, 0);
	}

	SDL_EndGPURenderPass(this->pass);

	if (!SDL_SubmitGPUCommandBuffer(this->cmdRender)) {
		std::cout << "Heil el renderer de Puigdemont\n";
		return;
	}
}
SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *filename,
			  Uint32 sampler_count, Uint32 uniform_buffer_count,
			  Uint32 storage_buffer_count,
			  Uint32 storage_texture_count)
{
	SDL_GPUShaderStage stage;
	if (SDL_strstr(filename, ".vert"))
	{
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	}
	else if (SDL_strstr(filename, ".frag"))
	{
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	}
	else
	{
		SDL_Log("Unknown shader type: %s", filename);
		return NULL;
	}

	SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	char fullpath[256];
	const char *entrypoint;
	const char *basepath = SDL_GetBasePath();

	if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
	{
		SDL_snprintf(fullpath, sizeof(fullpath), "%sassets/shaders/%s.spv",
			     basepath, filename);
		entrypoint = "main";
		format = SDL_GPU_SHADERFORMAT_SPIRV;
	}
	else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
	{
		SDL_snprintf(fullpath, sizeof(fullpath), "%sshaders/%s.dxil",
			     basepath, filename);
		entrypoint = "main";
		format = SDL_GPU_SHADERFORMAT_DXIL;
	}
	else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL)
	{
		SDL_snprintf(fullpath, sizeof(fullpath), "%sshaders/%s.msl",
			     basepath, filename);
		entrypoint = "main0";
		format = SDL_GPU_SHADERFORMAT_MSL;
	}
	else
	{
		SDL_Log("No supported shader format found!");
		return NULL;
	}

	size_t code_size;
	void *code = SDL_LoadFile(fullpath, &code_size);
	if (!code)
	{
		SDL_Log("Couldn't load shader file: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUShaderCreateInfo shader_info = {
		.code_size = code_size,
		.code = (const unsigned char*)code,
		.entrypoint = entrypoint,
		.format = format,
		.stage = stage,
		.num_samplers = sampler_count,
		.num_storage_textures = storage_texture_count,
		.num_storage_buffers = storage_buffer_count,
		.num_uniform_buffers = uniform_buffer_count,

	};

	SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shader_info);
	if (!shader)
	{
		SDL_Log("Couldn't create shader: %s", SDL_GetError());
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return shader;
}
SDL_GPUTexture *Renderer::CreateDepthTexture(Uint32 drawablew, Uint32 drawableh)
{
	SDL_GPUTextureCreateInfo createinfo;
	SDL_GPUTexture *result;

	createinfo.type = SDL_GPU_TEXTURETYPE_2D;
	createinfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
	createinfo.width = drawablew;
	createinfo.height = drawableh;
	createinfo.layer_count_or_depth = 1;
	createinfo.num_levels = 1;
	createinfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
	createinfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
	createinfo.props = 0;

	result = SDL_CreateGPUTexture(this->GPU, &createinfo);
	if (!result)
	{
		SDL_Log("Failed to create depth texture: %s", SDL_GetError());
		return NULL;
	}

	return result;
}
Renderer::Renderer(GameClient& gameClient): gameClient(gameClient), chunkManager()
{
	this->Width = 600;
	this->Height = 400;

	this->window = SDL_CreateWindow("Bit Miner", this->Width, this->Height, SDL_WINDOW_RESIZABLE);
	if (this->window == nullptr) {
		std::cout << "Error creating window: " << SDL_GetError();
		SDL_Quit();
		assert(false);
	}

	this->GPU = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
	if (this->GPU == nullptr) {
		std::cout << "SDL GPU creation failed: \n" << SDL_GetError();
	}
	if (!SDL_ClaimWindowForGPUDevice(this->GPU, this->window)) {
		std::cout << "Error claiming window for GPU device: " << SDL_GetError() << std::endl;
	}
	this->event = {};
	SDL_SetWindowRelativeMouseMode(window, true);
	
	//Fix
	//float aspect = (float)this->Width / (float)this->Height;
	//this->frustum = Frustum().createFrustumFromCamera(aspect, FOV, Znear, Zfar);


	std::cout << "Vertex size: " << vertexSize << "\t Index Size: " << indexSize << std::endl;
	for (int i = 0; i < (RenderDistance * 2 + 1) * (RenderDistance * 2 + 1); i++) {
		Mesh mesh{};
		SDL_GPUBufferCreateInfo vertexInfo = {};
		vertexInfo.size = vertexSize;
		vertexInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		mesh.VertexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &vertexInfo);
		mesh.VertexBuffer.offset = 0;

		SDL_GPUBufferCreateInfo indexInfo = {};
		indexInfo.size = indexSize;
		indexInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
		mesh.IndexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &indexInfo);
		mesh.IndexBuffer.offset = 0;

		SDL_GPUTransferBufferCreateInfo VertextransferInfo{};
		VertextransferInfo.size = vertexSize;
		VertextransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		mesh.VertextransferBuffer = SDL_CreateGPUTransferBuffer(this->GPU, &VertextransferInfo);

		SDL_GPUTransferBufferCreateInfo IndextransferInfo{};
		IndextransferInfo.size = indexSize;
		IndextransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		mesh.IndextransferBuffer = SDL_CreateGPUTransferBuffer(this->GPU, &IndextransferInfo);

		this->Terrain.push_back(mesh);
	}

	// load the vertex shader code

	SDL_GPUShader *vertex_shader =
		LoadShader(this->GPU, "Shader.vert", 0, 1, 0, 0);
	if (!vertex_shader)
	{
		SDL_Log("Couldn't load vertex shader: %s", SDL_GetError());
	}
	SDL_GPUShader *fragment_shader =
		LoadShader(this->GPU, "Shader.frag", 0, 0, 0, 0);
	if (!fragment_shader)
	{
		SDL_Log("Couldn't load fragment shader: %s", SDL_GetError());
	}
	SDL_GPUColorTargetDescription colorTargetDescriptions[1];
    colorTargetDescriptions[0] = {};
    colorTargetDescriptions[0].blend_state.enable_blend = true;
    colorTargetDescriptions[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTargetDescriptions[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTargetDescriptions[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTargetDescriptions[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTargetDescriptions[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTargetDescriptions[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTargetDescriptions[0].format = SDL_GetGPUSwapchainTextureFormat(this->GPU, window);

	SDL_GPUGraphicsPipelineCreateInfo pipeline_desc;

	pipeline_desc.target_info.num_color_targets = 1;
	pipeline_desc.target_info.color_target_descriptions =
		colorTargetDescriptions;
	pipeline_desc.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pipeline_desc.vertex_shader = vertex_shader;
	pipeline_desc.fragment_shader = fragment_shader;

	SDL_GPUVertexBufferDescription vertex_buffer_desc;
	SDL_GPUVertexAttribute vertex_attributes[2];

	// describe the vertex attribute
	SDL_GPUVertexAttribute vertexAttributes[2];

	vertex_buffer_desc.slot = 0;
	vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	vertex_buffer_desc.instance_step_rate = 0;
	vertex_buffer_desc.pitch = sizeof(Vertex);

	vertex_attributes[0].buffer_slot = 0;
	vertex_attributes[0].location = 0;
	vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertex_attributes[0].offset = 0;

	vertex_attributes[1].buffer_slot = 0;
	vertex_attributes[1].location = 1;
	vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertex_attributes[1].offset = sizeof(float) * 3;

	pipeline_desc.vertex_input_state.num_vertex_buffers = (RenderDistance * 2 + 1 ) * (RenderDistance * 2 + 1 );
	pipeline_desc.vertex_input_state.vertex_buffer_descriptions =
		&vertex_buffer_desc;
	pipeline_desc.vertex_input_state.num_vertex_attributes = 2;
	pipeline_desc.vertex_input_state.vertex_attributes =
		vertex_attributes;

	pipeline_desc.props = 0;

	this->graphicsPipeline = SDL_CreateGPUGraphicsPipeline(this->GPU, &pipeline_desc);
	if (!this->graphicsPipeline)
	{
		SDL_Log("Failed to create pipeline: %s", SDL_GetError());
	}
	std::cout << this->graphicsPipeline << std::endl;

	SDL_ReleaseGPUShader(this->GPU, vertex_shader);
	SDL_ReleaseGPUShader(this->GPU, fragment_shader);

	Uint32 drawablew, drawableh;
	SDL_GetWindowSizeInPixels(window, (int *)&drawablew, (int *)&drawableh);
	this->DepthTexture = CreateDepthTexture(drawablew, drawableh);
	UpdateViewportAndProjection();
}