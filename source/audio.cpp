enum {
    AUDIO_MASTER,
    AUDIO_ENEMY,
    AUDIO_UI,
    MAX_AUDIO
};

global
struct {
    const char *name;
    r32 volume, modifier;
} audio_type_data[MAX_AUDIO] = {
    { "MASTER", 1, 1 },
    { "ENEMIES", 1, 1 },
    { "UI", 1, 1 },
};
