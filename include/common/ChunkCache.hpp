#ifndef __CHUNKCACHE_HPP__
#define __CHUNKCACHE_HPP__

#include "Chunck.hpp"
#include <string>

class ChunkCache {
public:
  ChunkCache(const std::string &cacheDir = "");
  ~ChunkCache() = default;

  // Save/load chunks
  bool saveChunk(const ChunkPrefab &chunk, int chunkX, int chunkZ);
  bool loadChunk(ChunkPrefab &chunk, int chunkX, int chunkZ);
  bool chunkExists(int chunkX, int chunkZ) const;

  // Cache management
  void clearCache();
  std::string getCacheDir() const { return cacheDirectory; }

private:
  std::string cacheDirectory;
  std::string getChunkFilePath(int chunkX, int chunkZ) const;
  void ensureCacheDirectoryExists();
};

#endif
