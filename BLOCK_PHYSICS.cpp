#pragma once

static const float PHYS_GRAVITY = 9.81f;
static const float PHYS_AIR_DRAG = 0.05f;
static const float PHYS_GROUND_FRICTION = 0.85f;
static const float PHYS_BOUNCE_DAMPING = 0.2f;
static const float PHYS_ANGULAR_DAMPING = 0.92f;
static const float PHYS_MIN_VELOCITY = 0.03f;
static const float PHYS_MIN_ANGULAR = 0.05f;
static const float PHYS_GROUND_Y = -0.3f;
static const float PHYS_TERMINAL_VELOCITY = 20.0f;
static const float PHYS_MAX_ANGULAR_SPEED = 3.0f;
static const float PHYS_SPIN_TRANSFER = 0.08f;

static void clampAngularSpeed(Vec3& rotSpeed) {
    if (rotSpeed.x > PHYS_MAX_ANGULAR_SPEED) rotSpeed.x = PHYS_MAX_ANGULAR_SPEED;
    if (rotSpeed.x < -PHYS_MAX_ANGULAR_SPEED) rotSpeed.x = -PHYS_MAX_ANGULAR_SPEED;
    if (rotSpeed.y > PHYS_MAX_ANGULAR_SPEED) rotSpeed.y = PHYS_MAX_ANGULAR_SPEED;
    if (rotSpeed.y < -PHYS_MAX_ANGULAR_SPEED) rotSpeed.y = -PHYS_MAX_ANGULAR_SPEED;
    if (rotSpeed.z > PHYS_MAX_ANGULAR_SPEED) rotSpeed.z = PHYS_MAX_ANGULAR_SPEED;
    if (rotSpeed.z < -PHYS_MAX_ANGULAR_SPEED) rotSpeed.z = -PHYS_MAX_ANGULAR_SPEED;
}

static void applyGravity(Vec3& vel, float dt) {
    vel.y -= PHYS_GRAVITY * dt;
    if (vel.y < -PHYS_TERMINAL_VELOCITY) vel.y = -PHYS_TERMINAL_VELOCITY;
}

static void applyAirDrag(Vec3& vel, float dt) {
    float speed = vel.length();
    if (speed < 0.001f) return;
    float dragForce = PHYS_AIR_DRAG * speed * speed;
    float decel = dragForce * dt;
    if (decel > speed) decel = speed;
    Vec3 dir = vel.normalized();
    vel.x -= dir.x * decel;
    vel.y -= dir.y * decel;
    vel.z -= dir.z * decel;
}

static void applyGroundFriction(Vec3& vel, Vec3& rotSpeed, float dt) {
    float friction = PHYS_GROUND_FRICTION * dt * 15.0f;
    if (friction > 1.0f) friction = 1.0f;
    vel.x *= (1.0f - friction);
    vel.z *= (1.0f - friction);
    rotSpeed = rotSpeed * PHYS_ANGULAR_DAMPING;
}

static bool handleGroundCollision(Vec3& pos, Vec3& vel, Vec3& rotSpeed, float fragSize) {
    float groundLevel = PHYS_GROUND_Y + fragSize * 0.5f;
    if (pos.y > groundLevel) return false;

    pos.y = groundLevel;

    if (vel.y < -0.3f) {
        vel.y = -vel.y * PHYS_BOUNCE_DAMPING;
        vel.x *= 0.7f;
        vel.z *= 0.7f;
        rotSpeed = rotSpeed * 0.5f;
        float hSpeed = sqrtf(vel.x * vel.x + vel.z * vel.z);
        if (hSpeed > 0.1f) {
            rotSpeed.x += vel.z * PHYS_SPIN_TRANSFER;
            rotSpeed.z -= vel.x * PHYS_SPIN_TRANSFER;
        }
        clampAngularSpeed(rotSpeed);
    } else {
        vel.y = 0;
    }

    return true;
}

static bool handleBlockCollision(Vec3& pos, Vec3& vel, Vec3& rotSpeed, float fragSize) {
    if (!blockGrid) return false;

    int bx = (int)floorf(pos.x + 0.5f);
    int by = (int)floorf(pos.y + 0.5f);
    int bz = (int)floorf(pos.z + 0.5f);

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                int idx = blockGrid->get(bx + dx, by + dy, bz + dz);
                if (idx < 0 || !worldBlocks[idx].active) continue;

                Vec3 bp = worldBlocks[idx].position;
                float h = 0.5f;
                float hr = fragSize * 0.3f;

                if (pos.x + hr > bp.x - h && pos.x - hr < bp.x + h &&
                    pos.y + hr > bp.y - h && pos.y - hr < bp.y + h &&
                    pos.z + hr > bp.z - h && pos.z - hr < bp.z + h)
                {
                    float overlapX = std::min(pos.x + hr - (bp.x - h), (bp.x + h) - (pos.x - hr));
                    float overlapY = std::min(pos.y + hr - (bp.y - h), (bp.y + h) - (pos.y - hr));
                    float overlapZ = std::min(pos.z + hr - (bp.z - h), (bp.z + h) - (pos.z - hr));

                    if (overlapX < overlapY && overlapX < overlapZ) {
                        if (pos.x < bp.x) pos.x = bp.x - h - hr;
                        else pos.x = bp.x + h + hr;
                        vel.x = -vel.x * PHYS_BOUNCE_DAMPING;
                        rotSpeed.y += vel.x * PHYS_SPIN_TRANSFER;
                    } else if (overlapY < overlapZ) {
                        if (pos.y < bp.y) pos.y = bp.y - h - hr;
                        else pos.y = bp.y + h + hr;
                        vel.y = -vel.y * PHYS_BOUNCE_DAMPING;
                        vel.x *= 0.7f;
                        vel.z *= 0.7f;
                        rotSpeed = rotSpeed * 0.5f;
                    } else {
                        if (pos.z < bp.z) pos.z = bp.z - h - hr;
                        else pos.z = bp.z + h + hr;
                        vel.z = -vel.z * PHYS_BOUNCE_DAMPING;
                        rotSpeed.y -= vel.z * PHYS_SPIN_TRANSFER;
                    }
                    clampAngularSpeed(rotSpeed);
                    return true;
                }
            }
        }
    }
    return false;
}

static void updateFragmentPhysics(Fragment& fr, float dt) {
    if (!fr.active) return;

    applyGravity(fr.velocity, dt);
    applyAirDrag(fr.velocity, dt);

    fr.position.x += fr.velocity.x * dt;
    fr.position.y += fr.velocity.y * dt;
    fr.position.z += fr.velocity.z * dt;

    float fragSize = (fr.scale.x + fr.scale.y + fr.scale.z) / 3.0f;

    bool onGnd = handleGroundCollision(fr.position, fr.velocity, fr.rotSpeed, fragSize);
    handleBlockCollision(fr.position, fr.velocity, fr.rotSpeed, fragSize);

    if (onGnd) {
        applyGroundFriction(fr.velocity, fr.rotSpeed, dt);
    }

    fr.rotation.x += fr.rotSpeed.x * dt;
    fr.rotation.y += fr.rotSpeed.y * dt;
    fr.rotation.z += fr.rotSpeed.z * dt;

    float totalSpeed = fr.velocity.length();
    float totalSpin = fr.rotSpeed.length();

    if (onGnd && totalSpeed < PHYS_MIN_VELOCITY && totalSpin < PHYS_MIN_ANGULAR) {
        fr.velocity = {0, 0, 0};
        fr.rotSpeed = {0, 0, 0};
    }

    if (!fr.eternal) {
        fr.lifetime += dt;
        if (fr.lifetime >= fr.maxLifetime) fr.active = false;
    }

    if (fr.position.y < -50.0f) fr.active = false;
}

static void updateAllFragments(float dt) {
    for (auto& fr : fragments) {
        updateFragmentPhysics(fr, dt);
    }
    fragments.erase(
        std::remove_if(fragments.begin(), fragments.end(),
            [](const Fragment& f) { return !f.active; }),
        fragments.end());
}