#include "splashkit.h"
#include "splashkit-arrays.h"
#include "utilities.h"
#include <math.h>
#include "common.h"
#include "ability.h"
#include "character.h"
#include "computer_science.h"
#include "pharmacy.h"
#include "player.h"
#include "game.h"

/**
 * @brief Helper function to easily draw a clickable button.
 */
bool draw_and_check_button(string text, double x, double y, double width, double height)
{
    // Draw the button background
    fill_rectangle(COLOR_LIGHT_GRAY, x, y, width, height);
    draw_rectangle(COLOR_BLACK, x, y, width, height);

    // Automatically calculate the exact center for the text
    double text_w = text_width(text, "arcade", 30);
    double text_h = text_height(text, "arcade", 30);

    draw_text(text, COLOR_BLACK, "arcade", 30, x + (width / 2) - (text_w / 2), y + (height / 2) - (text_h / 2));

    // Check if the user clicked inside the rectangle
    if (mouse_clicked(LEFT_BUTTON))
    {
        // Record the clicked mouse position
        double mouse_x_pos = mouse_x();
        double mouse_y_pos = mouse_y();

        // Checks if the mouse is clicked inside the rectangle drawn
        if (mouse_x_pos >= x && mouse_x_pos <= x + width && mouse_y_pos >= y && mouse_y_pos <= y + height)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Handles the character selection screen logic for a specific player.
 * Draws character icons, handles hover text, and records mouse clicks to update the game settings.
 * 
 * @param cs_student_icon Bitmap image for the CS student.
 * @param pharmacy_icon Bitmap image for the Pharmacy student.
 * @param show_text Text to display at the top (e.g. "PLAYER 1").
 * @param settings Reference to the game_settings to update.
 * @param menu_state Reference to the current menu state machine integer.
 * @param in_menu Reference to the boolean controlling the menu loop.
 * @param player_number The player currently selecting a character (1 or 2).
 */
void character_selection(bitmap cs_student_icon, bitmap pharmacy_icon, string show_text, game_settings &settings, int &menu_state, bool &in_menu, int player_number)
{
    double cs_x = 450;
    double cs_y = 250;
    draw_bitmap(cs_student_icon, cs_x, cs_y);

    double pharmacy_x = 650;
    double pharmacy_y = 250;
    draw_bitmap(pharmacy_icon, pharmacy_x, pharmacy_y);

    // Get the width and height of the image to create a "hitbox" for the mouse
    double cs_width = bitmap_width(cs_student_icon);
    double cs_height = bitmap_height(cs_student_icon);

    double pharmacy_width = bitmap_width(pharmacy_icon);
    double pharmacy_height = bitmap_height(pharmacy_icon);

    double text_w = text_width(show_text, "arcade", 100);

    draw_text(show_text, COLOR_BLACK, "arcade", 100, (WINDOW_WIDTH / 2) - (text_w / 2) - 10, 50);
    draw_text(show_text, COLOR_BLACK, "arcade", 100, (WINDOW_WIDTH / 2) - (text_w / 2) + 10, 50);
    draw_text(show_text, COLOR_BLACK, "arcade", 100, (WINDOW_WIDTH / 2) - (text_w / 2), 40);
    draw_text(show_text, COLOR_BLACK, "arcade", 100, (WINDOW_WIDTH / 2) - (text_w / 2), 60);
    draw_text(show_text, COLOR_BLUE, "arcade", 100, (WINDOW_WIDTH / 2) - (text_w / 2), 50);
    
    // --- HOVER LOGIC ---
    // If the mouse is inside the image boundaries, show the description
    if (mouse_x() >= cs_x && mouse_x() <= cs_x + cs_width && mouse_y() >= cs_y && mouse_y() <= cs_y + cs_height)
    {
        // Draw the stats box slightly below the mouse
        fill_rectangle(COLOR_BLACK, mouse_x(), mouse_y() + 20, 300, 100);
        draw_text("Computer Science", COLOR_WHITE, "arial", 20, mouse_x() + 10, mouse_y() + 30);
        draw_text("Abilities:", COLOR_GREEN, "arial", 15, mouse_x() + 10, mouse_y() + 60);
        draw_text("Corrupted Combo", COLOR_GREEN, "arial", 15, mouse_x() + 10, mouse_y() + 80);
        draw_text("Glitch Protocol", COLOR_GREEN, "arial", 15, mouse_x() + 10, mouse_y() + 100);
        
        // --- CLICK LOGIC ---
        // If they click while hovering, select the character
        if (mouse_clicked(LEFT_BUTTON))
        {
            if (player_number == 1)
            {
                settings.p1_character = 1;
                if (settings.mode == 1)
                {
                    menu_state = 3;
                }
                else if (settings.mode == 2)
                {
                    menu_state = 4; // Go to player 2
                }
            }
            else if (player_number == 2)
            {
                settings.p2_character = 1;
                in_menu = false;
            }
        }
    }

    if (mouse_x() >= pharmacy_x && mouse_x() <= pharmacy_x + pharmacy_width && mouse_y() >= pharmacy_y && mouse_y() <= pharmacy_y + pharmacy_height)
    {
        // Draw the stats box slightly below the mouse
        fill_rectangle(COLOR_BLACK, mouse_x(), mouse_y() + 20, 300, 100);
        draw_text("Pharmacy", COLOR_WHITE, "arial", 20, mouse_x() + 10, mouse_y() + 30);
        draw_text("Abilities:", COLOR_GREEN, "arial", 15, mouse_x() + 10, mouse_y() + 60);
        draw_text("Immunoshield", COLOR_GREEN, "arial", 15, mouse_x() + 10, mouse_y() + 80);
        draw_text("Toxic Dose", COLOR_GREEN, "arial", 15, mouse_x() + 10, mouse_y() + 100);
        
        // --- CLICK LOGIC ---
        // If they click while hovering, select the character
        if (mouse_clicked(LEFT_BUTTON))
        {
            if (player_number == 1)
            {
                settings.p1_character = 2;
                if (settings.mode == 1)
                {
                    menu_state = 3;
                }
                else if (settings.mode == 2)
                {
                    menu_state = 4; // Go to player 2
                }
            }
            else if (player_number == 2)
            {
                settings.p2_character = 2;
                in_menu = false;
            }
        }
    }

    if (draw_and_check_button("BACK", 10, 540, 200, 50))
    {
        if (player_number == 1)
        {
            menu_state = 1;
        }
        else if (player_number == 2)
        {
            menu_state = 2;
        }
    }
}

/**
 * @brief Runs the fully graphical setup menu before the game begins.
 * Features a state machine to navigate through Mode Selection, Character Selection, and Difficulty.
 * 
 * @return game_settings The final configuration selected by the user.
 */
game_settings run_gui_menu()
{
    game_settings settings;

    // --- SETUP DEFAULT SETTINGS ---
    int menu_state = 1; // 1 = Mode Select, 2 = P1 Select, etc
    bool in_menu = true;

    // Checks if there is a saved game
    ifstream check_file("savegame.txt");

    if (check_file.is_open())
    {
        menu_state = 0;
    }

    // The Menu Loop
    while (!quit_requested() && in_menu)
    {
        process_events(); // Checks for mouse clicks
        clear_screen(COLOR_WHITE);

        // Draw the background menu image
        bitmap starting_menu = load_bitmap("Starting Menu", "starting_menu.png");
        bitmap menu = load_bitmap("Menu", "menu.png");

        // Load character icon images
        bitmap cs_student_icon = load_bitmap("CS Icon", "cs_student_selection.png");
        bitmap pharmacy_icon = load_bitmap("Pharmacy Icon", "pharmacy_student_selection.png");

        if (menu_state == 0 || menu_state == 1)
        {
            draw_bitmap(starting_menu, 0, 0);
        }
        else
        {
            draw_bitmap(menu, 0, 0);
        }

        // State Machine
        if (menu_state == 0)
        {
            if (draw_and_check_button("RESUME GAME", 400, 425, 400, 30))
            {
                settings.load_save = true;

                // Read the necessary setup info directly from the save file so we can skip the menus
                check_file >> settings.mode;
                
                int temp_difficulty;
                check_file >> temp_difficulty;
                settings.difficulty = (difficulty) temp_difficulty;
                
                // Skip the timer
                double junk_timer;
                check_file >> junk_timer;
                check_file >> settings.p1_character;

                // Skip player 1's stats
                double junk;
                check_file >> junk >> junk >> junk >> junk;

                check_file >> settings.p2_character;
                check_file.close();

                return settings; // Skip the rest of the setup and immediately launch the game
            }
            
            // Continue with mode selection menu
            if (draw_and_check_button("NEW GAME", 400, 475, 400, 30))
            {
                menu_state = 1;
            }
        }
        else if (menu_state == 1) // Mode Selection Screen
        {
            if (draw_and_check_button("SINGLEPLAYER", 400, 425, 400, 30))
            {
                settings.mode = 1;
                menu_state = 2; // Move to the next screen
            }

            if (draw_and_check_button("MULTIPLAYER", 400, 475, 400, 30))
            {
                settings.mode = 2;
                menu_state = 2; // Move to the next screen
            }

            if (check_file.is_open())
            {
                if (draw_and_check_button("BACK", 10, 540, 200, 50))
                {
                    menu_state = 0;
                }
            }
        }
        else if (menu_state == 2) // Player 1 Character Selection Screen
        {
            character_selection(cs_student_icon, pharmacy_icon, "PLAYER 1", settings, menu_state, in_menu, 1);
        }
        else if (menu_state == 3) // Difficulty Selection
        {
            string show_text = "DIFFICULTY";

            double text_w = text_width(show_text, "arcade", 100);

            draw_text(show_text, COLOR_BLACK, "arcade", 100, (WINDOW_WIDTH / 2) - (text_w / 2) - 10, 50);
            draw_text(show_text, COLOR_BLACK, "arcade", 100, (WINDOW_WIDTH / 2) - (text_w / 2) + 10, 50);
            draw_text(show_text, COLOR_BLACK, "arcade", 100, (WINDOW_WIDTH / 2) - (text_w / 2), 40);
            draw_text(show_text, COLOR_BLACK, "arcade", 100, (WINDOW_WIDTH / 2) - (text_w / 2), 60);
            draw_text(show_text, COLOR_BLUE, "arcade", 100, (WINDOW_WIDTH / 2) - (text_w / 2), 50);

            if (draw_and_check_button("EASY", 400, 200, 400, 50))
            {
                settings.difficulty = EASY;
                return settings;
            }
            
            if (draw_and_check_button("MEDIUM", 400, 275, 400, 50))
            {
                settings.difficulty = MEDIUM;
                return settings;
            }

            if (draw_and_check_button("HARD", 400, 350, 400, 50))
            {
                settings.difficulty = HARD;
                return settings;
            }

            if (draw_and_check_button("BACK", 10, 540, 200, 50))
            {
                menu_state = 2;
            }
        }
        else if (menu_state == 4) // Player 2 Selection
        {
            character_selection(cs_student_icon, pharmacy_icon, "PLAYER 2", settings, menu_state, in_menu, 2);
        }
        
        refresh_screen(60);
    }

    return settings;
}

// ---------------------------------------------------------
// MAIN FUNCTION
// ---------------------------------------------------------
int main()
{
    open_window("Ethan's Street Fighter Game", WINDOW_WIDTH, WINDOW_HEIGHT);

    load_font("arcade", "arcade.ttf");

    while (!quit_requested())
    {
        game_settings setup = run_gui_menu(); // Read in the game settings wanted by the user
        
        // Prevent crash if the user close the window using the X button during the menu
        if (quit_requested())
        {
            break;
        }

        game my_game(setup.mode, setup.p1_character, setup.p2_character, setup.difficulty);
    
        // If they chose to resume, overwrite the initial state with the saved data
        if (setup.load_save)
        {
            my_game.load_game();
        }

        bool play_again = false;
    
        while (!quit_requested())
        {
            process_events();
            my_game.update();
            my_game.draw();

            // --- GAME OVER SCREEN LOGIC ---
            if (my_game.get_state() == GAME_OVER)
            {
                remove("savegame.txt"); // Ensure the save is deleted

                // Becasue my_game.draw() just ran, buttons can be drawn directly on top of the arena
                if (draw_and_check_button("PLAY AGAIN", 400, 400, 400, 50))
                {
                    play_again = true;
                    break; // Breaks the combat loop, sending them back to the menu
                }

                if (draw_and_check_button("QUIT", 400, 475, 400, 50))
                {
                    play_again = false;
                    break; // Breaks the combat loop to quit
                }
            }
    
            // --- MID-MATCH SAVING LOGIC ---
            // Check if we need to save
            if (my_game.get_state() == PLAYING || my_game.get_state() == COUNTDOWN)
            {
                // Let the user press ESCAPE to save and quit at any time
                if (key_typed(ESCAPE_KEY))
                {
                    my_game.save_game();
                    play_again = false; // Quit after saving
                    break;
                }

                static int save_cooldown = 0;
                save_cooldown++;
                if (save_cooldown >= 60)
                {
                    my_game.save_game();
                    save_cooldown = 0; // Reset the timer
                }
            }

            refresh_screen(60);
        }
        
        // Check if we should quit the whole game
        // If they clicked the QUIT button or closed the window with X, break the outer loop
        if (!play_again)
        {
            break;
        }
    }

    close_window("Ethan's Street Fighter Game");
    return 0;
}
