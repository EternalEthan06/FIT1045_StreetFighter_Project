# pragma once

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 600;

/**
 * @brief Represents the current state of the game.
 * Used to manage the game flow between menus, playing, and ending the game.
 */
enum game_state
{
    COUNTDOWN,
    PLAYING,
    GAME_OVER,
};

/**
 * @brief Represents the AI difficulty levels for Singleplayer mode.
 */
enum difficulty
{
    EASY = 1,
    MEDIUM = 2,
    HARD = 3
};

/**
 * @brief Machine Learning Action Space.
 * Represents all the possible discrete decisions a human can make in one frame.
 */
enum ai_action
{
    AI_ATTACK,
    AI_ABILITY_1,
    AI_ABILITY_2,
    AI_JUMP,
    AI_CROUCH,
    AI_MOVE_CLOSER,
    AI_MOVE_AWAY,
    AI_IDLE
};

/**
 * @brief Machine Learning State Space (A single neuron/memory)
 * Records exactly what the game looked like mathematically, and what action
 * the human chose to perform in response to that specific situation.
 */
struct game_memory
{
    double distance;
    double my_stamina;
    double enemy_stamina;
    ai_action action_taken;
};

/**
 * @brief Stores the user's initial configuration for the game match.
 * Contains the selected game mode, characters, difficulty, and whether a saved game is loaded.
 */
struct game_settings
{
    int mode;
    int p1_character;
    int p2_character;
    difficulty difficulty;
    bool load_save;
};

/**
 * @brief Represents floating damage text that appears on screen when a character is hit.
 * Used for visual feedback during combat.
 */
struct damage_text
{
    double amount;
    double x;
    double y;
    int lifetime; // How many frames it would stay on the screen
};