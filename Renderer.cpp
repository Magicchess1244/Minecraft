#include "Renderer.h"

const float FOV = 0.01f;//(double)tanf((45.0f / 2.0f) * (PI / 180.0f));
constexpr Vector3 Verts[6][4] = {
	{//Front
		{ 0, 0, 0 },
		{1, 0, 0},
		{0, 1, 0},
		{1, 1, 0} },
	{//Back
		{ 1, 0, 1 },
		{0, 0, 1},
		{1, 1, 1},
		{0, 1, 1} },
	{//Right
		{1, 0, 0},
		{1, 0, 1},
		{1, 1, 0},
		{1, 1, 1} },
	{//Left
		{0, 0, 1},
		{0, 0, 0},
		{0, 1, 1},
		{0, 1, 0} },
	{//Top
		{0, 1, 0},
		{1, 1, 0},
		{0, 1, 1},
		{1, 1, 1} },
	{//Bottom
		{0, 0, 0},
		{1, 0, 0},
		{0, 0, 1},
		{1, 0, 1}
	}
};
constexpr Vector3 Direction[6] = {
	{ 0, 0, -1 }, // Front
	{ 0, 0, 1 },  // Back
	{ 1, 0, 0 },  // Right
	{ -1, 0, 0 }, // Left
	{ 0, 1, 0 },  // Top
	{ 0, -1, 0 }   // Bottom
};
constexpr SDL_FColor Colors[3] = {
	{0,0,0,1}, // Front / Back
	{0.05f,0.05f,0.05f,1}, // Right / Left
	{0.1f,0.1f,0.1f,1}, // Top / Bottom
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
	double cx = cos(AngleToRadians(Angle.x)), sx = sin(AngleToRadians(Angle.x));
	double cy = cos(AngleToRadians(Angle.y)), sy = sin(AngleToRadians(Angle.y));
	double cz = cos(AngleToRadians(Angle.z)), sz = sin(AngleToRadians(Angle.z));

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
		Vector3 World = ADDVECTORS(verts[i], blocks);
		Vector3 Relative = SUBSVECTORS(World, player.Position);
		Vector3 relToScreen = MULTIPLYVECTOR(Relative, BlockSize);

		Vector3 localPos = this->rotate(relToScreen, player.Rotation);

		//std::cout << " 3D:\t Px: " << Px << " Py: " << Py << " Pz: " << Pz << std::endl;

		double screenX = localPos.x;
		double screenY = localPos.y;

		if (localPos.z <= 0.01f) localPos.z = 0.001f;

		screenX = (localPos.x / (localPos.z * FOV)) + ScreenSize.x / 2.0f;
		screenY = (localPos.y / (localPos.z * FOV)) + ScreenSize.y / 2.0f;
		screenY = ScreenSize.y - screenY;

		//std::cout << " 2D:\t Px: " << screenX << " Py: " << screenY << std::endl;

		SDL_Vertex vertex = {
			{ screenX, screenY }, ADDCOLOR(BlockDef[color].Color, Colors[(int)(Side / 2)])
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
	
	double pitch = AngleToRadians(player.Rotation.x);
	double yaw = AngleToRadians(player.Rotation.y);

	Vector3 CameraDir = {
		cos(pitch) * sin(yaw),  // X
		sin(pitch),              // Y
		cos(pitch) * cos(yaw)   // Z
	};

	int chunkX = (int)floor((player.Position.x) / 32);
	int chunkZ = (int)floor((player.Position.z) / 32);
	std::vector<DrawnFace> Faces;
	int index = 0;

	for (auto& face : chunk.allFaces) {
		if (DotProduct(CameraDir, Direction[face.side]) >= 0) continue;
		double maxZ = 0.0f;
		for (int j = 0; j < 4; j++) {
			Vector3 worldFacePos = ADDVECTORS(face.blockPos, Verts[face.side][j]);
			Vector3 local = rotate(SUBSVECTORS(worldFacePos, player.Position), player.Rotation);
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
	return;
}

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



Renderer::Renderer()
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
}
