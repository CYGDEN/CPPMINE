// SOUNDMANAGER.cpp
#pragma once

static const int STONE_SOUND_COUNT = 6;
static int currentStoneSound = 0;
static bool soundsLoaded = false;

static char stoneSoundPaths[STONE_SOUND_COUNT][256] = {
    "SOUNDS\\stone1.wav",
    "SOUNDS\\stone2.wav",
    "SOUNDS\\stone3.wav",
    "SOUNDS\\stone4.wav",
    "SOUNDS\\stone5.wav",
    "SOUNDS\\stone6.wav"
};

static void initSounds() {
    soundsLoaded = true;
    for (int i = 0; i < STONE_SOUND_COUNT; i++) {
        FILE* f = fopen(stoneSoundPaths[i], "rb");
        if (!f) {
            char buf[512];
            sprintf(buf, "Sound missing: %s", stoneSoundPaths[i]);
            OutputDebugStringA(buf);
            OutputDebugStringA("\n");
        } else {
            fclose(f);
        }
    }
}

static void playStoneBreak() {
    if (!soundsLoaded) initSounds();

    PlaySoundA(stoneSoundPaths[currentStoneSound], NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);

    currentStoneSound++;
    if (currentStoneSound >= STONE_SOUND_COUNT) currentStoneSound = 0;
}

static void stopAllSounds() {
    PlaySoundA(NULL, NULL, 0);
}