#pragma once

struct BlockGrid {
    static const int GRID_SIZE = 256;
    static const int GRID_OFFSET = 64;
    static const int GRID_HEIGHT = 128;
    int data[GRID_SIZE][GRID_HEIGHT][GRID_SIZE];

    void clear() { memset(data, -1, sizeof(data)); }

    void set(int x, int y, int z, int idx) {
        x += GRID_OFFSET; z += GRID_OFFSET;
        if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_HEIGHT && z >= 0 && z < GRID_SIZE)
            data[x][y][z] = idx;
    }

    int get(int x, int y, int z) {
        x += GRID_OFFSET; z += GRID_OFFSET;
        if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_HEIGHT && z >= 0 && z < GRID_SIZE)
            return data[x][y][z];
        return -1;
    }

    bool occupied(int x, int y, int z) {
        return get(x, y, z) >= 0;
    }
};

static BlockGrid* blockGrid = nullptr;

static void initGrid() {
    if (!blockGrid) blockGrid = new BlockGrid();
}

static void rebuildGrid() {
    initGrid();
    blockGrid->clear();
    for (int i = 0; i < (int)worldBlocks.size(); i++) {
        if (!worldBlocks[i].active) continue;
        int bx = (int)floorf(worldBlocks[i].position.x + 0.5f);
        int by = (int)floorf(worldBlocks[i].position.y + 0.5f);
        int bz = (int)floorf(worldBlocks[i].position.z + 0.5f);
        blockGrid->set(bx, by, bz, i);
    }
}

static bool gridOccupied(int x, int y, int z) {
    if (!blockGrid) return false;
    return blockGrid->occupied(x, y, z);
}

static bool faceVisible(Vec3 pos, int face) {
    int bx = (int)floorf(pos.x + 0.5f);
    int by = (int)floorf(pos.y + 0.5f);
    int bz = (int)floorf(pos.z + 0.5f);
    switch (face) {
        case 0: return !gridOccupied(bx, by, bz - 1);
        case 1: return !gridOccupied(bx, by, bz + 1);
        case 2: return !gridOccupied(bx - 1, by, bz);
        case 3: return !gridOccupied(bx + 1, by, bz);
        case 4: return !gridOccupied(bx, by - 1, bz);
        case 5: return !gridOccupied(bx, by + 1, bz);
    }
    return true;
}

static void genCubeOptimized(Vec3 pos, Vec3 col, float size,
    std::vector<Vertex>& V, std::vector<uint32_t>& I)
{
    float h = size * 0.5f;
    Vec3 c[8] = {
        {pos.x-h,pos.y-h,pos.z-h},{pos.x+h,pos.y-h,pos.z-h},
        {pos.x+h,pos.y+h,pos.z-h},{pos.x-h,pos.y+h,pos.z-h},
        {pos.x-h,pos.y-h,pos.z+h},{pos.x+h,pos.y-h,pos.z+h},
        {pos.x+h,pos.y+h,pos.z+h},{pos.x-h,pos.y+h,pos.z+h}
    };
    struct FD { int v[4]; Vec3 n; };
    FD faces[6] = {
        {{0,3,2,1},{0,0,-1}},{{4,5,6,7},{0,0,1}},
        {{0,4,7,3},{-1,0,0}},{{1,2,6,5},{1,0,0}},
        {{0,1,5,4},{0,-1,0}},{{3,7,6,2},{0,1,0}}
    };
    for (int f = 0; f < 6; f++) {
        if (!faceVisible(pos, f)) continue;
        Vec3 n = faces[f].n;
        float shade;
        if (n.y > 0.5f) shade = 1.0f;
        else if (n.y < -0.5f) shade = 0.4f;
        else if (fabsf(n.x) > 0.5f) shade = 0.7f;
        else shade = 0.8f;
        Vec3 fc = col * shade;
        uint32_t base = (uint32_t)V.size();
        for (int v = 0; v < 4; v++) V.push_back({c[faces[f].v[v]], n, fc});
        I.push_back(base); I.push_back(base+1); I.push_back(base+2);
        I.push_back(base); I.push_back(base+2); I.push_back(base+3);
    }
}

static bool collidesPlayerFast(Vec3 pos) {
    if (!blockGrid) return false;
    float r = PLAYER_RADIUS;
    int minX = (int)floorf(pos.x - r - 0.5f);
    int maxX = (int)floorf(pos.x + r + 1.5f);
    int minY = (int)floorf(pos.y - 0.5f);
    int maxY = (int)floorf(pos.y + PLAYER_HEIGHT + 1.5f);
    int minZ = (int)floorf(pos.z - r - 0.5f);
    int maxZ = (int)floorf(pos.z + r + 1.5f);
    for (int bx = minX; bx <= maxX; bx++) {
        for (int by = minY; by <= maxY; by++) {
            for (int bz = minZ; bz <= maxZ; bz++) {
                int idx = blockGrid->get(bx, by, bz);
                if (idx < 0 || !worldBlocks[idx].active) continue;
                float h = 0.5f;
                float blx = worldBlocks[idx].position.x;
                float bly = worldBlocks[idx].position.y;
                float blz = worldBlocks[idx].position.z;
                if (pos.x+r > blx-h && pos.x-r < blx+h &&
                    pos.y+PLAYER_HEIGHT > bly-h && pos.y < bly+h &&
                    pos.z+r > blz-h && pos.z-r < blz+h)
                    return true;
            }
        }
    }
    return false;
}

static void removeBlockFromGrid(int idx) {
    if (!blockGrid || idx < 0) return;
    Block& bl = worldBlocks[idx];
    int bx = (int)floorf(bl.position.x + 0.5f);
    int by = (int)floorf(bl.position.y + 0.5f);
    int bz = (int)floorf(bl.position.z + 0.5f);
    bx += BlockGrid::GRID_OFFSET;
    bz += BlockGrid::GRID_OFFSET;
    if (bx >= 0 && bx < BlockGrid::GRID_SIZE &&
        by >= 0 && by < BlockGrid::GRID_HEIGHT &&
        bz >= 0 && bz < BlockGrid::GRID_SIZE)
        blockGrid->data[bx][by][bz] = -1;
}

static void cleanupGrid() {
    delete blockGrid;
    blockGrid = nullptr;
}