
#include "emoji_data_structure.h"

typedef struct emoji
{
    unsigned short threshold;
    char emoji[sizeof("ðĐ")];
} emoji;

#define asd sizeof("âïļ")

emoji emojis[] = {
    {.threshold = 0, .emoji = "ð"},
    {.threshold = 10, .emoji = "ð­"},
    {.threshold = 30, .emoji = "ðģ"},
    {.threshold = 40, .emoji = "ðĐ"},
    {.threshold = 50, .emoji = "ð"},
    {.threshold = 60, .emoji = "ð"},
    {.threshold = 70, .emoji = "ð"},
    {.threshold = 80, .emoji = "ð"},
    {.threshold = 90, .emoji = "ð"},
    {.threshold = 100, .emoji = "ð"},
    {.threshold = 110, .emoji = "ðŊ"},
    {.threshold = 120, .emoji = "ðĨ"},
    {.threshold = 999, .emoji = "ðŠĶ"},
};

static emoji *current = &emojis[0];

char *get_emoji(double key)
{
    while (key < current->threshold)
    {
        current--;
    }

    while ((current + 1)->threshold < key)
    {
        current++;
    }

    return current->emoji;
}