#include "dialogues.h"

#include "draw.h"  // WINDOW_WIDTH, WINDOW_HEIGHT

#include <raylib.h>

#define BUTTON_WIDTH   160
#define BUTTON_HEIGHT  48
#define BUTTON_GAP     24
#define PANEL_WIDTH    440
#define PANEL_HEIGHT   200

// One button, drawn and asked whether it was clicked this frame. Hovering
// only changes the fill -- the click is the mouse button going down inside
// the rect, so a press begun outside and dragged in does not count.
static bool button(Rectangle rect, char const label[static 1])
{
	bool const hovered = CheckCollisionPointRec(GetMousePosition(), rect);

	DrawRectangleRec(rect, hovered ? (Color){70, 70, 70, 255} : (Color){40, 40, 40, 255});
	DrawRectangleLinesEx(rect, 2.0f, RAYWHITE);

	int const text_width = MeasureText(label, 20);
	DrawText(label,
	         (int)rect.x + ((int)rect.width - text_width) / 2,
	         (int)rect.y + ((int)rect.height - 20) / 2,
	         20,
	         RAYWHITE);

	return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

dialogue_choice dialogues_death(void)
{
	float const panel_x = ((float)WINDOW_WIDTH - PANEL_WIDTH) / 2.0f;
	float const panel_y = ((float)WINDOW_HEIGHT - PANEL_HEIGHT) / 2.0f;

	// Dim what is behind rather than clear it: the frame the player died in
	// stays visible under the panel, which is what makes it read as an
	// overlay and not a new screen.
	DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, (Color){0, 0, 0, 160});
	DrawRectangle((int)panel_x, (int)panel_y, PANEL_WIDTH, PANEL_HEIGHT, (Color){20, 20, 20, 240});
	DrawRectangleLines((int)panel_x, (int)panel_y, PANEL_WIDTH, PANEL_HEIGHT, RAYWHITE);

	int const title_width = MeasureText("YOU DIED", 40);
	DrawText("YOU DIED",
	         (int)panel_x + (PANEL_WIDTH - title_width) / 2,
	         (int)panel_y + 32,
	         40,
	         RED);

	float const buttons_width = (2 * BUTTON_WIDTH) + BUTTON_GAP;
	float const buttons_x     = panel_x + ((PANEL_WIDTH - buttons_width) / 2.0f);
	float const buttons_y     = panel_y + PANEL_HEIGHT - BUTTON_HEIGHT - 32.0f;

	Rectangle const restart = {buttons_x, buttons_y, BUTTON_WIDTH, BUTTON_HEIGHT};
	Rectangle const exit_it = {buttons_x + BUTTON_WIDTH + BUTTON_GAP, buttons_y, BUTTON_WIDTH, BUTTON_HEIGHT};

	// Both buttons are drawn every frame; the first one clicked decides.
	bool const restart_clicked = button(restart, "Restart");
	bool const exit_clicked    = button(exit_it, "Exit");

	if (restart_clicked)
		return DIALOGUE_RESTART;
	if (exit_clicked)
		return DIALOGUE_EXIT;

	// Keyboard, so the dialogue is answerable without a mouse.
	if (IsKeyPressed(KEY_R))
		return DIALOGUE_RESTART;
	if (IsKeyPressed(KEY_ESCAPE))
		return DIALOGUE_EXIT;

	return DIALOGUE_PENDING;
}
