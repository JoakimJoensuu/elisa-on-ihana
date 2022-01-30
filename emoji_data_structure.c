
#include "emoji_data_structure.h"

typedef struct emoji
{
    unsigned short threshold;
    char emoji[sizeof("💩")];
} emoji;

#define asd sizeof("✌️")

emoji emojis[] = {
    {.threshold = 0, .emoji = "🆘"},
    {.threshold = 10, .emoji = "😭"},
    {.threshold = 30, .emoji = "😳"},
    {.threshold = 40, .emoji = "💩"},
    {.threshold = 50, .emoji = "😁"},
    {.threshold = 60, .emoji = "😊"},
    {.threshold = 70, .emoji = "👍"},
    {.threshold = 80, .emoji = "🖕"},
    {.threshold = 90, .emoji = "😎"},
    {.threshold = 100, .emoji = "👅"},
    {.threshold = 110, .emoji = "💯"},
    {.threshold = 120, .emoji = "🔥"},
    {.threshold = 999, .emoji = "🪦"},
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