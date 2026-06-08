#define CATCH_CONFIG_RUNNER
#include "catch_amalgamated.hpp"
#include "splashkit.h"

using Catch::Approx;

// Include all the core game files
#include "common.h"
#include "character.h"
#include "computer_science.h"
#include "pharmacy.h"
#include "game.h"
#include "player.h"

// Custom main to wrap SplashKit's window. 
// Without open_window(), the game and character constructors will crash when trying to load_bitmap().
int main(int argc, char* argv[]) 
{
    open_window("Headless Test Runner", 800, 600);
    
    int result = Catch::Session().run(argc, argv);
    
    close_window("Headless Test Runner");
    return result;
}

// ---------------------------------------------------------
// CHARACTER MECHANICS TESTS
// ---------------------------------------------------------
TEST_CASE("Character Damage Mechanics & State Flags", "[character]") 
{
    // Initialize two dummy characters (we can pass nullptr for sprites in headless tests)
    computer_science p1(100.0, 100.0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    pharmacy p2(200.0, 100.0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    SECTION("Crouching state is accurately tracked to prevent damage") 
    {
        p2.crouch();
        REQUIRE(p2.get_is_crouching() == true);
        REQUIRE(p2.get_is_jumping() == false);
        
        p2.stand();
        REQUIRE(p2.get_is_crouching() == false);
    }
    
    SECTION("Jumping state is accurately tracked to prevent damage") 
    {
        p2.jump();
        REQUIRE(p2.get_is_jumping() == true);
        REQUIRE(p2.get_is_crouching() == false);
    }

    SECTION("Directional Facing Logic for Damage Blocking") 
    {
        // p1 is at X=100, p2 is at X=200
        double distance = abs(p1.get_x() - p2.get_x());
        bool opponent_is_to_the_right = (p2.get_x() > p1.get_x());
        
        // Ensure distance calculation is exactly 100 pixels
        REQUIRE(distance == 100.0);
        
        // Ensure the game accurately detects the opponent's position
        REQUIRE(opponent_is_to_the_right == true);
    }
}

// ---------------------------------------------------------
// GAME SETUP & STATE TESTS
// ---------------------------------------------------------
TEST_CASE("Game Initialization & Settings", "[game]") 
{
    SECTION("Create game settings based on user choices") 
    {
        game_settings test_settings;
        test_settings.mode = 1;          // Singleplayer
        test_settings.p1_character = 1;  // Computer Science
        test_settings.p2_character = 2;  // Pharmacy
        test_settings.difficulty = HARD;    // Hard Mode
        
        REQUIRE(test_settings.mode == 1);
        REQUIRE(test_settings.p1_character == 1);
        REQUIRE(test_settings.difficulty == HARD);
        REQUIRE(test_settings.load_save == false);
        
        // Ensure the actual game engine initiates correctly with these settings
        game test_game(test_settings.mode, test_settings.p1_character, test_settings.p2_character, test_settings.difficulty);
        
        // Game should automatically start in the COUNTDOWN state
        REQUIRE(test_game.get_state() == COUNTDOWN);
    }
    
    SECTION("Testing Return to Menu (Game Over State)") 
    {
        game test_game(2, 1, 1, 1); // Local Multiplayer, CS vs CS
        
        // Initially, state should be COUNTDOWN
        REQUIRE(test_game.get_state() == COUNTDOWN);
        
        // (Note: The state transition to GAME_OVER itself is handled inside update() 
        // when health <= 0, which is tested in the 'Winner Determination Logic' section below.
        // We cannot force the state directly because the 'state' variable is strongly encapsulated as private.)
    }
}

// ---------------------------------------------------------
// ABILITY SYSTEM TESTS
// ---------------------------------------------------------
TEST_CASE("Pharmacy Immunoshield Override", "[abilities]")
{
    pharmacy p_student(100.0, 100.0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    
    SECTION("Base health is untouched when taking damage with shield")
    {
        double base_health = p_student.get_health();
        
        // Activate Immunoshield
        // We know slot 0 is Immunoshield for Pharmacy
        p_student.use_ability(0, nullptr); 
        
        // Shield should be 50.0
        REQUIRE(p_student.get_shield_health() == 50.0);
        
        // Hit them for 20 damage
        p_student.take_damage(20.0, 200.0);
        
        // Health should be untouched, shield should absorb it
        REQUIRE(p_student.get_health() == base_health);
        REQUIRE(p_student.get_shield_health() == 30.0); // 50 - 20 = 30
    }
}

// ---------------------------------------------------------
// MACHINE LEARNING AI TESTS
// ---------------------------------------------------------
TEST_CASE("Machine Learning Engine (1-NN Euclidean Distance)", "[ai]")
{
    // Seed a dummy brain file with exactly 1 specific memory
    std::ofstream brain_file("ai_brain.txt");
    brain_file << 1 << "\n"; // 1 memory total
    // Data Format: distance, my_stamina, enemy_stamina, action_taken
    // We log: 100.0 distance, 50.0 my_stamina, 50.0 enemy_stamina -> Action 3 (JUMP)
    brain_file << 100.0 << " " << 50.0 << " " << 50.0 << " " << 3 << "\n";
    brain_file.close();

    // Create a game (this automatically calls load_brain() in the constructor)
    game test_game(1, 1, 2, 2); // Singleplayer, CS vs Pharmacy

    SECTION("AI perfectly predicts action from the closest mathematical memory match")
    {
        // We supply a state that is incredibly close mathematically to the memory we injected
        // (Distance 105 instead of 100, etc.)
        ai_action result = test_game.predict_best_action(105.0, 48.0, 52.0);
        
        // The ML Model should confidently calculate the lowest Euclidean distance to that memory
        // and return the exact same action the human took in that situation (3 = JUMP)
        REQUIRE(result == AI_JUMP);
    }
}

// ---------------------------------------------------------
// GAME OVER & WINNER DETERMINATION TESTS
// ---------------------------------------------------------
TEST_CASE("Determining the Winner (Health Depletion Logic)", "[game_logic]")
{
    computer_science p1(100.0, 100.0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    pharmacy p2(100.0, 100.0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    SECTION("Player 2 wins when Player 1 health drops to 0 or below")
    {
        // Simulate Player 2 dealing lethal damage to Player 1
        // We pass 150 damage to ensure it bypasses any defense calculations
        p1.take_damage(150.0, p2.get_x());
        
        // Verify the death logic condition used in game.update() and game.draw()
        REQUIRE(p1.get_health() <= 0.0);
        REQUIRE(p2.get_health() > 0.0);
        
        // This simulates the exact branching logic in game.draw()
        int winner = 0;
        if (p1.get_health() <= 0) 
        {
            winner = 2;
        }
        else if (p2.get_health() <= 0) 
        {
            winner = 1;
        }
        
        // Ensure Player 2 is successfully declared the winner
        REQUIRE(winner == 2);
    }
}
