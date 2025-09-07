#include "Physics.hpp"
#include "ChunkManager.hpp"
#include "Chunk.hpp"

RayHit Physics::RaycastVoxel(const Vector3& rayOrigin, const Vector3& rayDir, float maxDistance) {
    RayHit result{false, 0, 0, 0, 0.0f};

    // Start voxel
    int x = int(floor(rayOrigin.x));
    int y = int(floor(rayOrigin.y));
    int z = int(floor(rayOrigin.z));

    // Step direction
    int stepX = (rayDir.x > 0) ? 1 : -1;
    int stepY = (rayDir.y > 0) ? 1 : -1;
    int stepZ = (rayDir.z > 0) ? 1 : -1;

    // Distances to the first voxel boundary
    float tMaxX = (rayDir.x != 0.0f)
                      ? ((stepX > 0 ? (x + 1 - rayOrigin.x) : (rayOrigin.x - x)) / rayDir.x)
                      : INFINITY;
    float tMaxY = (rayDir.y != 0.0f)
                      ? ((stepY > 0 ? (y + 1 - rayOrigin.y) : (rayOrigin.y - y)) / rayDir.y)
                      : INFINITY;
    float tMaxZ = (rayDir.z != 0.0f)
                      ? ((stepZ > 0 ? (z + 1 - rayOrigin.z) : (rayOrigin.z - z)) / rayDir.z)
                      : INFINITY;

    // Distances to cross one full voxel
    float tDeltaX = (rayDir.x != 0.0f) ? fabs(1.0f / rayDir.x) : INFINITY;
    float tDeltaY = (rayDir.y != 0.0f) ? fabs(1.0f / rayDir.y) : INFINITY;
    float tDeltaZ = (rayDir.z != 0.0f) ? fabs(1.0f / rayDir.z) : INFINITY;

    float t = 0.0f;
    Vector3 Pos = {(float)x, (float)y, (float)z};
    Pos = (Pos / 32).Truncate();
    while (t < maxDistance) {
        // Check voxel
        auto& Pref = this->chunkManager.get_chunk(Pos);
        auto it = Pref.Blocks.find(Pos);
        if (it == Pref.Blocks.end()) {  // 0 = air
            result.hit = true;
            result.x = x;
            result.y = y;
            result.z = z;
            result.distance = t;
            return result;
        }

        // Step to next voxel
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                x += stepX;
                t = tMaxX;
                tMaxX += tDeltaX;
            } else {
                z += stepZ;
                t = tMaxZ;
                tMaxZ += tDeltaZ;
            }
        } else {
            if (tMaxY < tMaxZ) {
                y += stepY;
                t = tMaxY;
                tMaxY += tDeltaY;
            } else {
                z += stepZ;
                t = tMaxZ;
                tMaxZ += tDeltaZ;
            }
        }
    }
    return result;  // no hit
}

bool Physics::RayIntersectsAABB(const Vector3& rayOrigin, const Vector3& rayDir,
                                const Vector3& boxMin,
                       const Vector3& boxMax, float& tNear, float& tFar) {
    tNear = -INFINITY;
    tFar = INFINITY;

    for (int i = 0; i < 3; i++) {
        if (fabs(rayDir[i]) < 1e-6) {
            // Ray is parallel to slab
            if (rayOrigin[i] < boxMin[i] || rayOrigin[i] > boxMax[i]) return false;
        } else {
            float t1 = (boxMin[i] - rayOrigin[i]) / rayDir[i];
            float t2 = (boxMax[i] - rayOrigin[i]) / rayDir[i];

            if (t1 > t2) std::swap(t1, t2);

            if (t1 > tNear) tNear = t1;
            if (t2 < tFar) tFar = t2;

            if (tNear > tFar) return false;
            if (tFar < 0) return false;
        }
    }
    return true;
}