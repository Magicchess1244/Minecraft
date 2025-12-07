#include "../../include/common/ChunkCache.hpp"
#include <filesystem>
#include <fstream>
#include <sys/stat.h>

// Chunk file format version
constexpr uint32_t CHUNK_FILE_VERSION = 1;

struct ChunkFileHeader {
  uint32_t version;
  uint32_t xSize;
  uint32_t ySize;
  uint32_t zSize;
  int32_t xPos;
  int32_t zPos;
};

ChunkCache::ChunkCache(const std::string &cacheDir) {
  if (cacheDir.empty()) {
    // Use default cache directory in user's home
    const char *home = getenv("HOME");
    if (home) {
      cacheDirectory = std::string(home) + "/.minecraft3d/chunks/";
    } else {
      cacheDirectory = ".minecraft3d_cache/chunks/";
    }
  } else {
    cacheDirectory = cacheDir;
    if (cacheDirectory.back() != '/') {
      cacheDirectory += '/';
    }
  }

  ensureCacheDirectoryExists();
}

void ChunkCache::ensureCacheDirectoryExists() {
  std::filesystem::create_directories(cacheDirectory);
}

std::string ChunkCache::getChunkFilePath(int chunkX, int chunkZ) const {
  return cacheDirectory + "chunk_" + std::to_string(chunkX) + "_" +
         std::to_string(chunkZ) + ".dat";
}

bool ChunkCache::chunkExists(int chunkX, int chunkZ) const {
  return std::filesystem::exists(getChunkFilePath(chunkX, chunkZ));
}

bool ChunkCache::saveChunk(const ChunkPrefab &chunk, int chunkX, int chunkZ) {
  std::string filepath = getChunkFilePath(chunkX, chunkZ);

  std::ofstream file(filepath, std::ios::binary);
  if (!file) {
    return false;
  }

  // Write header
  ChunkFileHeader header;
  header.version = CHUNK_FILE_VERSION;
  header.xSize = ChunkPrefab::xSize;
  header.ySize = ChunkPrefab::ySize;
  header.zSize = ChunkPrefab::zSize;
  header.xPos = chunk.xPos;
  header.zPos = chunk.zPos;

  file.write(reinterpret_cast<const char *>(&header), sizeof(header));

  // Write block data
  file.write(reinterpret_cast<const char *>(chunk.blocks.data()),
             chunk.blocks.size() * sizeof(int));

  file.close();
  return true;
}

bool ChunkCache::loadChunk(ChunkPrefab &chunk, int chunkX, int chunkZ) {
  std::string filepath = getChunkFilePath(chunkX, chunkZ);

  std::ifstream file(filepath, std::ios::binary);
  if (!file) {
    return false;
  }

  // Read header
  ChunkFileHeader header;
  file.read(reinterpret_cast<char *>(&header), sizeof(header));

  // Verify version and size
  if (header.version != CHUNK_FILE_VERSION ||
      header.xSize != ChunkPrefab::xSize ||
      header.ySize != ChunkPrefab::ySize ||
      header.zSize != ChunkPrefab::zSize) {
    return false;
  }

  // Read block data
  file.read(reinterpret_cast<char *>(chunk.blocks.data()),
            chunk.blocks.size() * sizeof(int));

  chunk.xPos = header.xPos;
  chunk.zPos = header.zPos;
  chunk.isDirty = false;

  file.close();
  return true;
}

void ChunkCache::clearCache() {
  std::filesystem::remove_all(cacheDirectory);
  ensureCacheDirectoryExists();
}
