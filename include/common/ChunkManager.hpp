#ifndef __CHUNKMANAGER_HPP__
#define __CHUNKMANAGER_HPP__

#include "BlockDef.hpp"
#include "Common.hpp"
#include <SDL3/SDL_stdinc.h>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ChunkPrefab;

// ─── Spline helpers ──────────────────────────────────────────────────────────

typedef struct {
  float x;
  float y;
} HeightsDif;

float SampleSpline(float value, const HeightsDif *spline, int length);
int GetBaseHeight(float Continentalness, float Erosion, float Peaks);
float GetCaveThreshold(float y);
float GetCoalChance(float y);
float GetIronChance(float y);
float GetDiamondChance(float y);

// ─── Biome ───────────────────────────────────────────────────────────────────

enum class BiomeType {
  Ice,
  Tundra,
  Taiga,
  BigTaiga,
  Plains,
  Forest,
  Birch,
  DarkForest,
  Jungle,
  Desert,
  Savanna
};

typedef struct {
  const char *Name;
  BiomeType Type;
  int MaxHumidity;
  int MaxTemperature;
  int MinHumidity;
  int MinTemperature;
  int BaseHeight;
  int ChangeAmount;
} Biome;

// ─── Raycast ─────────────────────────────────────────────────────────────────

struct RaycastResult {
  bool hit;
  Vector3 pos;
  Vector3 prevPos;
  Uint8 BlockID;
};

// ─── ChunkManager ────────────────────────────────────────────────────────────

class ChunkManager {
public:
  ChunkManager();
  ~ChunkManager();

  int DayLightLevel = 15;

  // ── Chunk access / lifecycle ──────────────────────────────────────────────
  ChunkPrefab &get_chunk(Vector3 chunkKey);
  void refresh_ready_neighbours(Vector3 centerKey);

  // ── World queries ─────────────────────────────────────────────────────────
  Uint8 GetBlockID(Vector3 worldPos);
  Uint8 GetLightLevel(Vector3 worldPos);
  Uint8 GetSunlightLevel(Vector3 worldPos);
  Uint8 GetBlockLightLevel(Vector3 worldPos);
  bool IsSolid(Vector3 worldPos);
  RaycastResult RayCast(Vector3 origin, Vector3 direction, float maxDistance);

  // ── Block modification ────────────────────────────────────────────────────
  void Place(Vector3 worldPos, int blockID);
  void SetBlock(Vector3 worldPos, int blockID, bool updateNeighbours = true);
  bool try_set_block_local(ChunkPrefab &chunk, Vector3 worldPos, int blockID);
  void rebuild_chunk(ChunkPrefab &chunk);

  // ── Modifications map  ────────────────────────────────────────────────────
  // Returns the player-placed block at worldPos, or 255 if unmodified.
  Uint8 GetMod(Vector3 worldPos);
  void GetModificationsForChunk(int xStart, int zStart,
                                std::unordered_map<int, Uint8> &localMods);

  // ── Biome ─────────────────────────────────────────────────────────────────
  Biome GetBiome(float humidity, float temperature);

  // ── Block definition helper ───────────────────────────────────────────────
  Block GetBlock(int blockID) {
    if (blockID < 0 || blockID >= (int)BlockDefinitionsAmount) {
      std::cerr << "Invalid Block ID: " << blockID << std::endl;
      return BlockDefinitions[0];
    }
    return BlockDefinitions[blockID];
  }

  // ── Water simulation ──────────────────────────────────────────────────────
  void TickWater();
  void AddActiveWater(Vector3 pos);
  void AddActiveWater(Vector3 pos, int level);

private:
  // ── Thread pool ──────────────────────────────────────────────────────────
  // Owned by ChunkManager so its workers are joined *before* Chunks is
  // destroyed (members are destroyed in reverse declaration order, so pool
  // must be declared AFTER Chunks).
  struct ChunkThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop = false;

    ChunkThreadPool() {
      int n = (int)std::thread::hardware_concurrency();
      if (n < 2)
        n = 2;
      if (n > 8)
        n = 8;
      for (int i = 0; i < n; i++) {
        workers.emplace_back([this] {
          while (true) {
            std::function<void()> task;
            {
              std::unique_lock<std::mutex> lock(mtx);
              cv.wait(lock, [this] { return stop || !tasks.empty(); });
              if (stop && tasks.empty())
                return;
              task = std::move(tasks.front());
              tasks.pop();
            }
            task();
          }
        });
      }
    }

    ~ChunkThreadPool() {
      {
        std::unique_lock<std::mutex> lock(mtx);
        stop = true;
      }
      cv.notify_all();
      for (auto &w : workers)
        w.join();
    }

    void submit(std::function<void()> fn) {
      {
        std::unique_lock<std::mutex> lock(mtx);
        tasks.push(std::move(fn));
      }
      cv.notify_one();
    }
  };

  // Shared lock used by every method that touches Chunks or Modifications.
  std::recursive_mutex chunks_mutex;

  std::unordered_map<Vector3, std::unique_ptr<ChunkPrefab>> Chunks;
  std::unordered_map<Vector3, Uint8> Modifications;
  std::vector<std::pair<Vector3, int>> activeWater;

  // pool is declared LAST so it is destroyed FIRST (reverse member order).
  // This ensures all worker threads are joined before Chunks is freed.
  ChunkThreadPool pool;
};

#endif