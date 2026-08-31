#ifndef DBJ_THE_GAME_DIALOGUES_H
#define DBJ_THE_GAME_DIALOGUES_H

// Dialogues live here, not in draw.c: this is where every later one lands
// too. Like draw.c it needs raylib, so it sits at that level and never
// below it.
//
// What a dialogue decides comes back as this enum, never as a raylib type
// and never as a call into the world. That is what keeps the simulation
// windowless and the test binary linkable without a display.
typedef enum : unsigned char
{
    DIALOGUE_PENDING,  // still on screen, nothing chosen yet
    DIALOGUE_RESTART,  // reload the map into a fresh world
    DIALOGUE_EXIT,     // leave the loop
} dialogue_choice;

// The death dialogue: two buttons, Restart and Exit. Drawn over whatever
// frame is already there -- the caller does not clear the screen for it.
//
// Returns DIALOGUE_PENDING on every frame nothing was pressed, so the
// caller can treat "no decision yet" as the ordinary case and act only on
// the other two.
dialogue_choice dialogues_death(void);

// The victory dialogue. Same two choices as death, and deliberately the
// same enum: winning and losing differ in what the player is told, not in
// what they may do next. A third value would name a choice nobody has.
dialogue_choice dialogues_win(void);

#endif  // DBJ_THE_GAME_DIALOGUES_H
