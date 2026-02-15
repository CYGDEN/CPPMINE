#pragma once

static const int FRACTURE_MIN_PIECES = 7;
static const int FRACTURE_MAX_PIECES = 14;
static const int MICRO_PARTICLES = 25;
static const int DUST_PARTICLES = 40;
static const float MICRO_SIZE = 0.035f;
static const float DUST_SIZE = 0.015f;
static const int SECONDARY_FRACTURE_CHANCE = 3;
static const int CRACK_DEPTH_LEVELS = 3;

static Vec3 randomPointInCube(Vec3 center, float halfSize) {
    std::uniform_real_distribution<float> d(-halfSize, halfSize);
    return {center.x + d(rng), center.y + d(rng), center.z + d(rng)};
}

static bool pointBehindPlane(Vec3 point, Vec3 planePoint, Vec3 planeNormal) {
    return Vec3::dot(point - planePoint, planeNormal) < 0;
}

static Vec3 linePlaneIntersect(Vec3 a, Vec3 b, Vec3 planePoint, Vec3 planeNormal) {
    Vec3 ab = b - a;
    float denom = Vec3::dot(ab, planeNormal);
    if (fabsf(denom) < 1e-8f) return a;
    float t = Vec3::dot(planePoint - a, planeNormal) / denom;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return a + ab * t;
}

static std::vector<Vec3> clipPolygonByPlane(
    const std::vector<Vec3>& poly, Vec3 planePoint, Vec3 planeNormal)
{
    if (poly.size() < 3) return {};
    std::vector<Vec3> result;
    int n = (int)poly.size();
    for (int i = 0; i < n; i++) {
        Vec3 curr = poly[i];
        Vec3 next = poly[(i + 1) % n];
        bool currInside = !pointBehindPlane(curr, planePoint, planeNormal);
        bool nextInside = !pointBehindPlane(next, planePoint, planeNormal);
        if (currInside) {
            result.push_back(curr);
            if (!nextInside)
                result.push_back(linePlaneIntersect(curr, next, planePoint, planeNormal));
        } else if (nextInside) {
            result.push_back(linePlaneIntersect(curr, next, planePoint, planeNormal));
        }
    }
    return result;
}

struct ConvexShape {
    std::vector<std::vector<Vec3>> faces;
    std::vector<bool> isCutFace;
};

static ConvexShape makeCubeShape(Vec3 center, float halfSize) {
    ConvexShape shape;
    float h = halfSize;
    Vec3 c = center;
    Vec3 corners[8] = {
        {c.x-h,c.y-h,c.z-h},{c.x+h,c.y-h,c.z-h},
        {c.x+h,c.y+h,c.z-h},{c.x-h,c.y+h,c.z-h},
        {c.x-h,c.y-h,c.z+h},{c.x+h,c.y-h,c.z+h},
        {c.x+h,c.y+h,c.z+h},{c.x-h,c.y+h,c.z+h}
    };
    shape.faces.push_back({corners[0],corners[3],corners[2],corners[1]});
    shape.faces.push_back({corners[4],corners[5],corners[6],corners[7]});
    shape.faces.push_back({corners[0],corners[4],corners[7],corners[3]});
    shape.faces.push_back({corners[1],corners[2],corners[6],corners[5]});
    shape.faces.push_back({corners[0],corners[1],corners[5],corners[4]});
    shape.faces.push_back({corners[3],corners[7],corners[6],corners[2]});
    for (int i = 0; i < 6; i++) shape.isCutFace.push_back(false);
    return shape;
}

static ConvexShape clipShapeByPlane(const ConvexShape& shape, Vec3 planePoint, Vec3 planeNormal) {
    ConvexShape result;
    std::vector<Vec3> capPoints;

    for (int fi = 0; fi < (int)shape.faces.size(); fi++) {
        std::vector<Vec3> clipped = clipPolygonByPlane(shape.faces[fi], planePoint, planeNormal);
        if (clipped.size() >= 3) {
            result.faces.push_back(clipped);
            result.isCutFace.push_back(shape.isCutFace[fi]);
            for (auto& p : clipped) {
                float dist = fabsf(Vec3::dot(p - planePoint, planeNormal));
                if (dist < 0.01f) capPoints.push_back(p);
            }
        }
    }

    if (capPoints.size() >= 3) {
        std::vector<Vec3> uniqueCap;
        for (auto& p : capPoints) {
            bool dup = false;
            for (auto& u : uniqueCap) {
                if ((p - u).lengthSq() < 0.0001f) { dup = true; break; }
            }
            if (!dup) uniqueCap.push_back(p);
        }
        if (uniqueCap.size() >= 3) {
            Vec3 cc = {0, 0, 0};
            for (auto& p : uniqueCap) cc += p;
            cc = cc * (1.0f / uniqueCap.size());
            Vec3 refDir = (uniqueCap[0] - cc).normalized();
            Vec3 rightDir = Vec3::cross(planeNormal, refDir).normalized();
            if (rightDir.lengthSq() < 0.001f) {
                Vec3 alt = fabsf(planeNormal.y) > 0.9f ? Vec3{1,0,0} : Vec3{0,1,0};
                rightDir = Vec3::cross(planeNormal, alt).normalized();
                refDir = Vec3::cross(rightDir, planeNormal).normalized();
            }
            std::sort(uniqueCap.begin(), uniqueCap.end(),
                [&](const Vec3& a, const Vec3& b) {
                    Vec3 da = a - cc;
                    Vec3 db = b - cc;
                    return atan2f(Vec3::dot(da, rightDir), Vec3::dot(da, refDir))
                         < atan2f(Vec3::dot(db, rightDir), Vec3::dot(db, refDir));
                });
            result.faces.push_back(uniqueCap);
            result.isCutFace.push_back(true);
        }
    }
    return result;
}

static Vec3 shapeCenter(const ConvexShape& shape) {
    Vec3 sum = {0, 0, 0};
    int count = 0;
    for (auto& face : shape.faces) {
        for (auto& p : face) { sum += p; count++; }
    }
    return count > 0 ? sum * (1.0f / count) : sum;
}

static float shapeVolume(const ConvexShape& shape) {
    Vec3 c = shapeCenter(shape);
    float vol = 0;
    for (auto& face : shape.faces) {
        if (face.size() < 3) continue;
        for (int i = 1; i < (int)face.size() - 1; i++) {
            Vec3 a = face[0] - c;
            Vec3 b = face[i] - c;
            Vec3 d = face[i + 1] - c;
            vol += fabsf(Vec3::dot(a, Vec3::cross(b, d))) / 6.0f;
        }
    }
    return vol;
}

static Vec3 perturbPoint(Vec3 p, float amount) {
    std::uniform_real_distribution<float> d(-amount, amount);
    return {p.x + d(rng), p.y + d(rng), p.z + d(rng)};
}

static void addMicroCracksToFace(const std::vector<Vec3>& face, Vec3 center,
    Vec3 faceNormal, Vec3 color, int depth,
    std::vector<Vertex>& V, std::vector<uint32_t>& I)
{
    if (face.size() < 3 || depth <= 0) return;

    Vec3 darkColor = {color.x * 0.15f, color.y * 0.15f, color.z * 0.15f};
    std::uniform_real_distribution<float> cv(-0.03f, 0.03f);

    Vec3 faceCenter = {0, 0, 0};
    for (auto& p : face) faceCenter += p;
    faceCenter = faceCenter * (1.0f / face.size());
    faceCenter = faceCenter - center;

    float lineWidth = 0.006f / (float)depth;
    Vec3 offset = faceNormal * (0.001f * depth);

    std::uniform_int_distribution<int> crackCount(2, 4);
    int numCracks = crackCount(rng);

    for (int c = 0; c < numCracks; c++) {
        std::uniform_int_distribution<int> startIdx(0, (int)face.size() - 1);
        int si = startIdx(rng);
        Vec3 start = face[si] - center;
        Vec3 end = faceCenter;

        std::uniform_real_distribution<float> bend(-0.08f, 0.08f);
        int segments = 3 + depth;
        Vec3 prev = start;

        for (int s = 1; s <= segments; s++) {
            float t = (float)s / (float)segments;
            Vec3 curr = {
                start.x + (end.x - start.x) * t + bend(rng),
                start.y + (end.y - start.y) * t + bend(rng),
                start.z + (end.z - start.z) * t + bend(rng)
            };

            Vec3 segDir = (curr - prev).normalized();
            Vec3 segNormal = Vec3::cross(segDir, faceNormal).normalized();
            if (segNormal.lengthSq() < 0.001f) continue;

            Vec3 crackCol = {
                clampf(darkColor.x + cv(rng), 0, 1),
                clampf(darkColor.y + cv(rng), 0, 1),
                clampf(darkColor.z + cv(rng), 0, 1)
            };

            uint32_t base = (uint32_t)V.size();
            V.push_back({prev - segNormal * lineWidth + offset, faceNormal, crackCol});
            V.push_back({prev + segNormal * lineWidth + offset, faceNormal, crackCol});
            V.push_back({curr + segNormal * lineWidth + offset, faceNormal, crackCol});
            V.push_back({curr - segNormal * lineWidth + offset, faceNormal, crackCol});
            I.push_back(base); I.push_back(base+1); I.push_back(base+2);
            I.push_back(base); I.push_back(base+2); I.push_back(base+3);

            if (depth > 1 && s == segments / 2) {
                std::uniform_int_distribution<int> branch(0, 2);
                if (branch(rng) == 0) {
                    Vec3 branchEnd = curr + segNormal * 0.1f;
                    branchEnd = perturbPoint(branchEnd, 0.03f);
                    float bw = lineWidth * 0.6f;
                    Vec3 bDir = (branchEnd - curr).normalized();
                    Vec3 bNorm = Vec3::cross(bDir, faceNormal).normalized();
                    if (bNorm.lengthSq() > 0.001f) {
                        uint32_t bb = (uint32_t)V.size();
                        V.push_back({curr - bNorm * bw + offset, faceNormal, crackCol});
                        V.push_back({curr + bNorm * bw + offset, faceNormal, crackCol});
                        V.push_back({branchEnd + bNorm * bw + offset, faceNormal, crackCol});
                        V.push_back({branchEnd - bNorm * bw + offset, faceNormal, crackCol});
                        I.push_back(bb); I.push_back(bb+1); I.push_back(bb+2);
                        I.push_back(bb); I.push_back(bb+2); I.push_back(bb+3);
                    }
                }
            }
            prev = curr;
        }
    }
}

static void addCutSurfaceDetail(const std::vector<Vec3>& face, Vec3 center,
    Vec3 faceNormal, Vec3 color,
    std::vector<Vertex>& V, std::vector<uint32_t>& I)
{
    if (face.size() < 3) return;

    Vec3 faceCenter = {0, 0, 0};
    for (auto& p : face) faceCenter += p;
    faceCenter = faceCenter * (1.0f / face.size());

    std::uniform_real_distribution<float> cv(-0.04f, 0.04f);
    std::uniform_real_distribution<float> bumpAmt(-0.01f, 0.01f);
    Vec3 offset = faceNormal * 0.001f;

    int n = (int)face.size();
    for (int i = 0; i < n; i++) {
        Vec3 a = face[i] - center;
        Vec3 b = face[(i + 1) % n] - center;
        Vec3 mid = (a + b) * 0.5f;
        mid = mid + faceNormal * bumpAmt(rng);

        Vec3 fc2 = faceCenter - center;
        Vec3 innerA = a + (fc2 - a) * 0.15f;
        Vec3 innerB = b + (fc2 - b) * 0.15f;

        Vec3 roughCol = {
            clampf(color.x * 0.45f + cv(rng), 0, 1),
            clampf(color.y * 0.45f + cv(rng), 0, 1),
            clampf(color.z * 0.45f + cv(rng), 0, 1)
        };
        Vec3 roughCol2 = {
            clampf(color.x * 0.55f + cv(rng), 0, 1),
            clampf(color.y * 0.55f + cv(rng), 0, 1),
            clampf(color.z * 0.55f + cv(rng), 0, 1)
        };

        uint32_t base = (uint32_t)V.size();
        V.push_back({a + offset, faceNormal, roughCol});
        V.push_back({mid + offset, faceNormal, roughCol2});
        V.push_back({innerA + offset, faceNormal, roughCol});
        I.push_back(base); I.push_back(base+1); I.push_back(base+2);

        base = (uint32_t)V.size();
        V.push_back({mid + offset, faceNormal, roughCol2});
        V.push_back({b + offset, faceNormal, roughCol});
        V.push_back({innerB + offset, faceNormal, roughCol2});
        I.push_back(base); I.push_back(base+1); I.push_back(base+2);
    }
}

static void shapeToMesh(const ConvexShape& shape, Vec3 center, Vec3 color,
    std::vector<Vertex>& V, std::vector<uint32_t>& I)
{
    for (int fi = 0; fi < (int)shape.faces.size(); fi++) {
        auto& face = shape.faces[fi];
        if (face.size() < 3) continue;
        bool isCut = fi < (int)shape.isCutFace.size() && shape.isCutFace[fi];

        Vec3 e1 = face[1] - face[0];
        Vec3 e2 = face[2] - face[0];
        Vec3 normal = Vec3::cross(e1, e2).normalized();

        float shade;
        if (normal.y > 0.5f) shade = 1.0f;
        else if (normal.y < -0.5f) shade = 0.4f;
        else if (fabsf(normal.x) > 0.5f) shade = 0.7f;
        else shade = 0.8f;

        Vec3 fc;
        if (isCut) {
            std::uniform_real_distribution<float> cv(-0.03f, 0.03f);
            fc = {
                clampf(color.x * shade * 0.55f + cv(rng), 0, 1),
                clampf(color.y * shade * 0.55f + cv(rng), 0, 1),
                clampf(color.z * shade * 0.55f + cv(rng), 0, 1)
            };
        } else {
            std::uniform_real_distribution<float> cv(-0.015f, 0.015f);
            fc = {
                clampf(color.x * shade + cv(rng), 0, 1),
                clampf(color.y * shade + cv(rng), 0, 1),
                clampf(color.z * shade + cv(rng), 0, 1)
            };
        }

        uint32_t base = (uint32_t)V.size();
        for (auto& p : face) V.push_back({p - center, normal, fc});
        for (int i = 1; i < (int)face.size() - 1; i++) {
            I.push_back(base);
            I.push_back(base + i);
            I.push_back(base + i + 1);
        }

        if (isCut) {
            addCutSurfaceDetail(face, center, normal, color, V, I);
            for (int d = 1; d <= CRACK_DEPTH_LEVELS; d++) {
                addMicroCracksToFace(face, center, normal, color, d, V, I);
            }
        } else {
            std::uniform_int_distribution<int> surfCrack(0, 3);
            if (surfCrack(rng) == 0) {
                addMicroCracksToFace(face, center, normal, color, 1, V, I);
            }
        }
    }
}

static void addEdgeCracks(const ConvexShape& shape, Vec3 center, Vec3 color,
    std::vector<Vertex>& V, std::vector<uint32_t>& I)
{
    Vec3 darkColor = {color.x * 0.12f, color.y * 0.12f, color.z * 0.12f};
    float lineThick = 0.005f;

    for (int fi = 0; fi < (int)shape.faces.size(); fi++) {
        if (fi >= (int)shape.isCutFace.size() || !shape.isCutFace[fi]) continue;
        auto& face = shape.faces[fi];
        if (face.size() < 3) continue;

        Vec3 faceNormal = Vec3::cross(face[1] - face[0], face[2] - face[0]).normalized();
        Vec3 offset = faceNormal * 0.002f;

        for (int i = 0; i < (int)face.size(); i++) {
            Vec3 a = face[i] - center;
            Vec3 b = face[(i + 1) % face.size()] - center;
            Vec3 edgeDir = (b - a).normalized();
            Vec3 edgeNormal = Vec3::cross(edgeDir, faceNormal).normalized();
            if (edgeNormal.lengthSq() < 0.001f) continue;

            uint32_t base = (uint32_t)V.size();
            V.push_back({a - edgeNormal * lineThick + offset, faceNormal, darkColor});
            V.push_back({a + edgeNormal * lineThick + offset, faceNormal, darkColor});
            V.push_back({b + edgeNormal * lineThick + offset, faceNormal, darkColor});
            V.push_back({b - edgeNormal * lineThick + offset, faceNormal, darkColor});
            I.push_back(base); I.push_back(base+1); I.push_back(base+2);
            I.push_back(base); I.push_back(base+2); I.push_back(base+3);
        }
    }
}

static void spawnDustCloud(Vec3 blockPos, Vec3 color) {
    std::uniform_real_distribution<float> pd(-0.45f, 0.45f);
    std::uniform_real_distribution<float> vd(-1.0f, 1.0f);
    std::uniform_real_distribution<float> vy(0.0f, 0.8f);
    std::uniform_real_distribution<float> sd(0.6f, 1.4f);
    std::uniform_real_distribution<float> cv(-0.04f, 0.04f);

    for (int i = 0; i < DUST_PARTICLES; i++) {
        Fragment fr;
        fr.position = {
            blockPos.x + pd(rng),
            blockPos.y + pd(rng),
            blockPos.z + pd(rng)
        };
        fr.velocity = {vd(rng), vy(rng), vd(rng)};
        fr.rotation = {0, 0, 0};
        fr.rotSpeed = {vd(rng) * 0.2f, vd(rng) * 0.2f, vd(rng) * 0.2f};
        fr.color = color;

        float s = DUST_SIZE * sd(rng);
        float hs = s * 0.5f;

        Vec3 dustCol = {
            clampf(color.x * 0.8f + cv(rng), 0, 1),
            clampf(color.y * 0.8f + cv(rng), 0, 1),
            clampf(color.z * 0.8f + cv(rng), 0, 1)
        };

        Vec3 cc[8] = {
            {-hs,-hs,-hs},{hs,-hs,-hs},{hs,hs,-hs},{-hs,hs,-hs},
            {-hs,-hs,hs},{hs,-hs,hs},{hs,hs,hs},{-hs,hs,hs}
        };
        struct FD { int v[4]; Vec3 n; };
        FD ff[6] = {
            {{0,3,2,1},{0,0,-1}},{{4,5,6,7},{0,0,1}},
            {{0,4,7,3},{-1,0,0}},{{1,2,6,5},{1,0,0}},
            {{0,1,5,4},{0,-1,0}},{{3,7,6,2},{0,1,0}}
        };
        for (int f = 0; f < 6; f++) {
            uint32_t base = (uint32_t)fr.vertices.size();
            for (int v = 0; v < 4; v++)
                fr.vertices.push_back({cc[ff[f].v[v]], ff[f].n, dustCol});
            fr.indices.push_back(base); fr.indices.push_back(base+1); fr.indices.push_back(base+2);
            fr.indices.push_back(base); fr.indices.push_back(base+2); fr.indices.push_back(base+3);
        }

        fr.scale = {1, 1, 1};
        fr.lifetime = 0;
        fr.maxLifetime = 5.0f;
        fr.eternal = fragmentsEternal;
        fr.active = true;
        fragments.push_back(fr);
    }
}

static void spawnMicroParticles(Vec3 blockPos, Vec3 color) {
    std::uniform_real_distribution<float> pd(-0.4f, 0.4f);
    std::uniform_real_distribution<float> vd(-2.5f, 2.5f);
    std::uniform_real_distribution<float> vy(-0.3f, 0.5f);
    std::uniform_real_distribution<float> rd(-0.5f, 0.5f);
    std::uniform_real_distribution<float> sd(0.5f, 1.5f);
    std::uniform_real_distribution<float> cv(-0.05f, 0.05f);

    for (int i = 0; i < MICRO_PARTICLES; i++) {
        Fragment fr;
        fr.position = {
            blockPos.x + pd(rng),
            blockPos.y + pd(rng),
            blockPos.z + pd(rng)
        };
        fr.velocity = {vd(rng), vy(rng), vd(rng)};
        fr.rotation = {0, 0, 0};
        fr.rotSpeed = {rd(rng), rd(rng), rd(rng)};
        fr.color = color;

        float s = MICRO_SIZE * sd(rng);
        float hs = s * 0.5f;

        Vec3 chipCol = {
            clampf(color.x * 0.65f + cv(rng), 0, 1),
            clampf(color.y * 0.65f + cv(rng), 0, 1),
            clampf(color.z * 0.65f + cv(rng), 0, 1)
        };

        std::uniform_real_distribution<float> jit(-hs * 0.3f, hs * 0.3f);
        Vec3 cc[8];
        float bx[8] = {-1,1,1,-1,-1,1,1,-1};
        float by[8] = {-1,-1,1,1,-1,-1,1,1};
        float bz[8] = {-1,-1,-1,-1,1,1,1,1};
        for (int k = 0; k < 8; k++) {
            cc[k] = {bx[k]*hs + jit(rng), by[k]*hs + jit(rng), bz[k]*hs + jit(rng)};
        }

        struct FD { int v[4]; Vec3 n; };
        FD ff[6] = {
            {{0,3,2,1},{0,0,-1}},{{4,5,6,7},{0,0,1}},
            {{0,4,7,3},{-1,0,0}},{{1,2,6,5},{1,0,0}},
            {{0,1,5,4},{0,-1,0}},{{3,7,6,2},{0,1,0}}
        };
        for (int f = 0; f < 6; f++) {
            uint32_t base = (uint32_t)fr.vertices.size();
            for (int v = 0; v < 4; v++)
                fr.vertices.push_back({cc[ff[f].v[v]], ff[f].n, chipCol});
            fr.indices.push_back(base); fr.indices.push_back(base+1); fr.indices.push_back(base+2);
            fr.indices.push_back(base); fr.indices.push_back(base+2); fr.indices.push_back(base+3);
        }

        fr.scale = {1, 1, 1};
        fr.lifetime = 0;
        fr.maxLifetime = 8.0f;
        fr.eternal = fragmentsEternal;
        fr.active = true;
        fragments.push_back(fr);
    }
}

static void fractureAndSpawn(Block& bl) {
    float halfSize = BLOCK_SIZE * 0.5f;
    Vec3 blockCenter = bl.position;

    std::uniform_int_distribution<int> numPieces(FRACTURE_MIN_PIECES, FRACTURE_MAX_PIECES);
    int pieceCount = numPieces(rng);

    std::vector<Vec3> seeds;
    for (int i = 0; i < pieceCount; i++)
        seeds.push_back(randomPointInCube(blockCenter, halfSize * 0.85f));

    std::uniform_real_distribution<float> rd(-0.5f, 0.5f);
    std::uniform_real_distribution<float> jitter(-0.025f, 0.025f);
    std::uniform_int_distribution<int> secondaryChance(0, SECONDARY_FRACTURE_CHANCE);

    for (int i = 0; i < pieceCount; i++) {
        ConvexShape piece = makeCubeShape(blockCenter, halfSize);

        for (int j = 0; j < pieceCount; j++) {
            if (i == j) continue;
            Vec3 mid = (seeds[i] + seeds[j]) * 0.5f;
            Vec3 dir = (seeds[i] - seeds[j]).normalized();
            mid.x += jitter(rng);
            mid.y += jitter(rng);
            mid.z += jitter(rng);
            piece = clipShapeByPlane(piece, mid, dir);
            if (piece.faces.empty()) break;
        }

        if (piece.faces.empty()) continue;
        float vol = shapeVolume(piece);
        if (vol < 0.0003f) continue;

        Vec3 center = shapeCenter(piece);
        Vec3 awayDir = (center - blockCenter);
        float awayLen = awayDir.length();
        if (awayLen > 0.01f) awayDir = awayDir * (1.0f / awayLen);
        else awayDir = {rd(rng), 0, rd(rng)};

        if (secondaryChance(rng) == 0 && vol > 0.01f) {
            Vec3 subSeed1 = perturbPoint(center, halfSize * 0.3f);
            Vec3 subSeed2 = perturbPoint(center, halfSize * 0.3f);
            Vec3 subMid = (subSeed1 + subSeed2) * 0.5f;
            Vec3 subDir = (subSeed1 - subSeed2).normalized();

            ConvexShape half1 = clipShapeByPlane(piece, subMid, subDir);
            ConvexShape half2 = clipShapeByPlane(piece, subMid, Vec3{0,0,0} - subDir);

            ConvexShape parts[2] = {half1, half2};
            for (int p = 0; p < 2; p++) {
                if (parts[p].faces.empty()) continue;
                float sv = shapeVolume(parts[p]);
                if (sv < 0.0003f) continue;

                Vec3 sc = shapeCenter(parts[p]);
                Vec3 ad = (sc - blockCenter);
                float al = ad.length();
                if (al > 0.01f) ad = ad * (1.0f / al);

                Fragment fr;
                fr.position = sc;
                fr.velocity = {
                    ad.x * 1.0f + rd(rng) * 0.4f,
                    rd(rng) * 0.15f,
                    ad.z * 1.0f + rd(rng) * 0.4f
                };
                fr.rotation = {0, 0, 0};
                fr.rotSpeed = {rd(rng) * 0.2f, rd(rng) * 0.2f, rd(rng) * 0.2f};
                fr.color = bl.color;
                fr.scale = {1, 1, 1};
                shapeToMesh(parts[p], sc, bl.color, fr.vertices, fr.indices);
                addEdgeCracks(parts[p], sc, bl.color, fr.vertices, fr.indices);
                fr.lifetime = 0;
                fr.maxLifetime = fragmentTimeout;
                fr.eternal = fragmentsEternal;
                fr.active = true;
                fragments.push_back(fr);
            }
            continue;
        }

        Fragment fr;
        fr.position = center;
        fr.velocity = {
            awayDir.x * 1.0f + rd(rng) * 0.4f,
            rd(rng) * 0.15f,
            awayDir.z * 1.0f + rd(rng) * 0.4f
        };
        fr.rotation = {0, 0, 0};
        fr.rotSpeed = {rd(rng) * 0.25f, rd(rng) * 0.25f, rd(rng) * 0.25f};
        fr.color = bl.color;
        fr.scale = {1, 1, 1};

        shapeToMesh(piece, center, bl.color, fr.vertices, fr.indices);
        addEdgeCracks(piece, center, bl.color, fr.vertices, fr.indices);

        fr.lifetime = 0;
        fr.maxLifetime = fragmentTimeout;
        fr.eternal = fragmentsEternal;
        fr.active = true;
        fragments.push_back(fr);
    }

    spawnMicroParticles(blockCenter, bl.color);
    spawnDustCloud(blockCenter, bl.color);
}