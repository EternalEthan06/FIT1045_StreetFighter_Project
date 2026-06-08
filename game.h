# include "splashkit.h"
# include "splashkit-arrays.h"
# include "common.h"
# include "player.h"
# include <iostream>
# include <fstream>
# include <math.h>

using std::ofstream;
using std::ifstream;

/**
 * @class game
 * @brief The central game manager. Manages game state, backgrounds, initialization, and the main game updates.
 */
class game
{
    player player1;
    player player2;
    int p1_choice;
    int p2_choice;
    character* fighter1;
    character* fighter2;
    int countdown_timer;
    
    game_state state;
    bool is_singleplayer;
    int ai_difficulty;
    double round_timer;
    dynamic_array<game_memory> ai_brain;
    
    bitmap background_image;
    bitmap glitch_bmp;
    bitmap corrupted_bmp;
    bitmap immunoshield_bmp;
    bitmap toxic_bmp;

    private:
        /**
         * @brief Controls Fighter 2 when the timer is running out.
         * @param ai_hp The remaining health of Fighter 2.
         * @param p1_hp The remaining health of Fighter 1.
         * @param distance The distance between Fighter 1 and Fighter 2
         */
        void handle_low_timer(double ai_hp, double p1_hp, double distance)
        {
            if (ai_hp > p1_hp)
            {
                // The AI has more health, so it tries to run away to win by timeout
                if (distance > 0)
                {
                    fighter2->move_left();
                }
                else
                {
                    fighter2->move_right();
                }
            }
            else
            {
                // The AI is losing, it will attack more aggressively
                if (distance > 80.0)
                {
                    fighter2->move_right();
                }
                else if (distance < -80.0)
                {
                    fighter2->move_left();
                }
                else
                {
                    fighter2->perform_attack(); // Spam attack
                }
            }
        }

        /**
         * @brief Controls Fighter 2 when the AI is critically low on health.
         * @param distance The distance between Fighter 1 and Fighter 2.
         */
        void handle_low_hp(double distance)
        {
            // Run away to create space
            if (distance > 0)
            {
                fighter2->move_left();
            }
            else
            {
                fighter2->move_right();
            }

            // Use Immunoshield (Slot 0) to protect itself
            fighter2->use_ability(0, fighter1);
        }

        /**
         * @brief Checks if the AI is low on stamina and retreats to recover.
         * @param distance The distance between Fighter 1 and Fighter 2.
         * @return True if the AI took action to retreat, false otherwise.
         */
        bool handle_normal_low_stamina(double distance)
        {
            // If the difficulty is not hard, the computer is not smart enough to retreat
            if (ai_difficulty != HARD)
            {
                return false;
            }

            double ai_stamina = fighter2->get_stamina();

            // If the AI is exhausted (below 20.0 stamina), it should retreat
            if (ai_stamina < 20.0)
            {
                if (distance > 0)
                {
                    fighter2->move_left(); // Run away
                }
                else
                {
                    fighter2->move_right();
                }
                
                return true; // We handled this state, skip other normal combat logic
            }

            return false; // Did not trigger, continue normal logic
        }

        /**
         * @brief Checks if the opponent is low on stamina and acts aggressively.
         * @param distance The distance between Fighter 1 and Fighter 2.
         * @return True if the AI took action to exploit the opponent, false otherwise.
         */
        bool handle_normal_opponent_low_stamina(double distance)
        {
            // The easy mode is oblivious and won't exploit the opponent's low stamina
            if (ai_difficulty == EASY)
            {
                return false;
            }

            double p1_stamina = fighter1->get_stamina();

            // If the player is exhausted, the AI goes on the offensive
            if (p1_stamina < 5.0)
            {
                // Close the distance fast
                if (distance > 80.0)
                {
                    fighter2->move_right();
                }
                else if (distance < -80.0)
                {
                    fighter2->move_left();
                }
                else
                {
                    // Player is close and exhausted, punish them
                    fighter2->perform_attack();
                }
                return true; // We handled this state, skip other normal combat logic
            }

            return false; // Did not trigger, continue normal logic
        }

        /**
         * @brief Checks if the opponent is attacking and randomly decides to dodge.
         * @return True if the AI took action to dodge, false otherwise.
         */
        bool handle_normal_dodge()
        {
            // If the player is throwing a punch
            if (fighter1->is_currently_attacking())
            {
                // Check if the AI has enough stamina to perform a dodge
                if (fighter2->get_stamina() >= 20.0)
                {
                    int dodge_chance;

                    // Set the probability based on difficulty
                    if (ai_difficulty == HARD)
                    {
                        dodge_chance = 30; // ~38% chance to dodge an attack
                    }
                    else if (ai_difficulty == MEDIUM)
                    {
                        dodge_chance = 75; // ~18% chance to dodge an attack
                    }
                    else
                    {
                        dodge_chance = 150; // ~9% chance to dodge an attack
                    }

                    int random = rnd(0, dodge_chance);

                    // ~9% chance per frame to dodge the incoming attack
                    if (random == 10)
                    {
                        int random_dodge = rnd(0, 1);

                        switch (random_dodge)
                        {
                            case 0:
                                fighter2->crouch();
                                break;
                            case 1:
                                fighter2->jump();
                                break;
                        }
                        
                        return true; // We handled this state, skip other normal combat logic
                    }
                }
            }

            return false; // Did not trigger, continue normal logic
        }

        /**
         * @brief Executes the specific action determined by the AI logic.
         * @param current_decision The action the AI has decided to take.
         * @param distance The current distance between Fighter 1 and Fighter 2.
         */
        void execute_decision(ai_action current_decision, double distance)
        {
            if (current_decision == AI_ATTACK)
                {
                    fighter2->perform_attack();
                }
                else if (current_decision == AI_ABILITY_1)
                {
                    fighter2->use_ability(0, fighter1);
                }
                else if (current_decision == AI_ABILITY_2)
                {
                    fighter2->use_ability(1, fighter1);
                }
                else if (current_decision == AI_JUMP)
                {
                    fighter2->jump();
                }
                else if (current_decision == AI_CROUCH)
                {
                    fighter2->crouch();
                }
                else if (current_decision == AI_MOVE_CLOSER)
                {
                    if (distance > 0)
                    {
                        fighter2->move_right(); // Player is to the right
                    }
                    else
                    {
                        fighter2->move_left(); // Player is to the left
                    }
                }
                else if (current_decision == AI_MOVE_AWAY)
                {
                    if (distance > 0)
                    {
                        // If they are cornered on the left wall, fight back instead of running
                        if (fighter2->get_x() <= 0)
                        {
                            fighter2->move_right();
                        }
                        else
                        {
                        fighter2->move_left();
                        }
                    }
                    else
                    {
                        if (fighter2->get_x() >= 1100)
                        {
                            fighter2->move_left();
                        }
                        else
                        {
                            fighter2->move_right();
                        }
                    }
                }
        }

        /**
         * @brief Manages the attack probabilities within the decision tree AI.
         * Uses RNG to determine if the AI attacks or uses an ability this frame based on difficulty.
         */
        void ai_decision_tree_attack()
        {
            // Using a random number so the AI doesn't perfectly attack every single frame
            double random_chance = rnd();

            if (ai_difficulty == HARD)
            {
                if (random_chance < 0.025)
                {
                    fighter2->perform_attack(); // 2.5% chance every frame to attack
                }
                else if (random_chance > 0.98)
                {
                    fighter2->use_ability(1, fighter1); // 2% chance every frame to use "Toxic Dose" ability
                }
            }
            else if (ai_difficulty == MEDIUM)
            {
                if (random_chance < 0.01)
                {
                    fighter2->perform_attack(); // 1% chance every frame to attack
                }
                else if (random_chance > 0.99)
                {
                    fighter2->use_ability(1, fighter1); // 1% chance every frame to use "Toxic Dose" ability
            }
            }
            else
            {
                if (random_chance < 0.005)
                {
                    fighter2->perform_attack(); // 0.5% chance every frame to attack
                }
                else if (random_chance > 0.99)
                {
                    fighter2->use_ability(1, fighter1); // 1% chance every frame to use "Toxic Dose" ability
                }
            }
        }

        /**
         * @brief Determines the movement and spacing strategy for the decision tree AI.
         * @param distance Current distance between the fighters.
         * @param desired_distance The optimal distance the AI wants to maintain.
         */
        void ai_decision_tree_actions(double distance, double desired_distance)
        {
            // If the player is far away, move towards them
            if (distance > desired_distance)
            {
                fighter2->move_right();
            }
            else if (distance < -desired_distance)
            {
                fighter2->move_left();
            }
            else
            {
                fighter2->stop_moving(); // Stop walking to attack

                ai_decision_tree_attack();
            }
        }

        /**
         * @brief Artificially injects noise into the AI's perceived distance based on difficulty.
         * @param distance The true distance between the fighters.
         * @return The distorted distance perceived by the AI.
         */
        double add_sensory_noise(double distance)
        {
            // --- SENSORY NOISE ---
            double perceived_distance = distance;

            if (ai_difficulty == EASY)
            {
                perceived_distance += rnd(-400.0, 400.0); // Modify the data to an extreme extent
            }
            else if (ai_difficulty == MEDIUM)
            {
                perceived_distance += rnd(-100.0, 100.0); // Slight miscalculations
            }

            return perceived_distance;
        }

        /**
         * @brief Determines how many frames the AI must wait before making a new decision.
         * Simulates human reaction time based on difficulty settings.
         * @return Number of frames to lock the current decision.
         */
        int determine_decision_timer()
        {
            int decision_timer = 0;

            if (ai_difficulty == HARD)
            {
                decision_timer = 5; // Reaction of 0.08s
            }
            else if (ai_difficulty == MEDIUM)
            {
                decision_timer = 30; // Normal reaction of 0.5s
            }
            else if (ai_difficulty == EASY)
            {
                decision_timer = 90; // Slow reaction time of 1.5s
            }

            return decision_timer;
        }

        /**
         * @brief Controls Fighter 2 in a normal combat loop.
         * @param distance The distance between Fighter 1 and Fighter 2.
         */
        void handle_normal_combat(double distance)
        {
            // Check higher priority states first. If any of these helper methods return true, 
            // the AI took an action and we should return early to prevent multiple actions per frame.
            if (handle_normal_low_stamina(distance)) 
            {
                return;
            }

            if (handle_normal_opponent_low_stamina(distance)) 
            {
                return;
            }

            if (handle_normal_dodge()) 
            {
                return;
            }

            // --- MACHINE LEARNING ---
            // Check if the brain has actually learned some memories
            if (length(ai_brain) > 0)
            {
                static int decision_timer = 0;
                static ai_action current_decision = AI_IDLE;

                // Only allow the AI to rethink its strategy when the timer hits 0
                if (decision_timer <= 0)
                {
                    // --- SENSORY NOISE ---
                    double perceived_distance = add_sensory_noise(distance);
                    
                    // Ask the ML Brain what the human would do right now
                    current_decision = predict_best_action(perceived_distance, fighter2->get_stamina(), fighter1->get_stamina());

                    // That decision is locked in for a specific amount of time based on difficulty
                    decision_timer = determine_decision_timer();
                }

                // Count down the decision timer every frame
                decision_timer--;

                // Execute the decision
                execute_decision(current_decision, distance);

                // We used the ML Brain, so we return early to skip the other logics
                return;
            }

            // --- STANDARD LOGIC (EASY & MEDIUM MODE) ---
            double desired_distance = 80.0; // Easy Mode: Mindlessly rush the player

            if (ai_difficulty == MEDIUM)
            {
                // Medium Mode: Use the smart zoning logic
                if (fighter2->get_stamina() > 80.0)
                {
                    desired_distance = 60.0;
                }
                else
                {
                    desired_distance = 180.0;
                }
            }

            ai_decision_tree_actions(distance, desired_distance);
        }

        /**
         * @brief Controls Fighter 2 automatically if the game is in singleplayer mode.
         */
        void run_ai_logic()
        {
            // --- GLITCH OVERRIDE ---
            // If the AI is glitched by the CS Student, it loses its mind!
            if (fighter2->get_is_glitched())
            {
                // Random movement
                int move_roll = rnd(0, 2); 

                if (move_roll == 0) 
                {
                    fighter2->move_left();
                }
                else if (move_roll == 1) 
                {
                    fighter2->move_right();
                }
                else 
                {
                    fighter2->stop_moving();
                }
                
                // Random actions
                if (rnd() < 0.05) fighter2->jump();
                
                if (rnd() < 0.03) 
                {
                    fighter2->crouch();
                }
                else 
                {
                    fighter2->stand();
                }

                if (rnd() < 0.05) 
                {
                    fighter2->perform_attack();
                }
                // RETURN EARLY! The AI is not allowed to use its brain while glitched!
                return;
            }

            double distance = fighter1->get_x() - fighter2->get_x();
            double ai_hp = fighter2->get_health();
            double p1_hp = fighter1->get_health();
            double time_left = round_timer;

            // Making Decisions

            // Condition A: Time is running out
            if (time_left < 15.0)
            {
                handle_low_timer(ai_hp, p1_hp, distance);
            }

            // Condition B: AI is critically low on health
            else if (ai_hp <= 30.0)
            {
                handle_low_hp(distance);
            }

            // Condition C: Normal Combat Loop (Default behaviour)
            else
            {
                handle_normal_combat(distance);
            }
        }

    public:
        /**
         * @brief Saves the ML brain to a file so it remembers your playstyle forever!
         */
        void save_brain()
        {
            // Use a completely separate file so it doesn't corrupt the normal savegame
            ofstream brain_file("ai_brain.txt");
            
            if (brain_file.is_open())
            {
                // Save exactly how many memories we have
                brain_file << length(ai_brain) << "\n";

                // Loop through the brain and write every single memory to the file
                for (int i = 0; i < length(ai_brain); i++)
                {
                    brain_file << ai_brain[i].distance << " "
                               << ai_brain[i].my_stamina << " "
                               << ai_brain[i].enemy_stamina << " "
                               << ai_brain[i].action_taken << "\n";
                }
                brain_file.close();
            }
        }

        /**
         * @brief Loads the ML brain from the file when the game starts.
         */
        void load_brain()
        {
            ifstream brain_file("ai_brain.txt");
            
            if (brain_file.is_open())
            {
                int total_memories;
                brain_file >> total_memories; // Read how many memories there are

                for (int i = 0; i < total_memories; i++)
                {
                    game_memory mem;
                    int action_int;
                    
                    // Read the 4 variables from the text file
                    brain_file >> mem.distance >> mem.my_stamina >> mem.enemy_stamina >> action_int;
                    
                    // Convert the integer back into our Enum label!
                    mem.action_taken = (ai_action)action_int; 
                    
                    // Add it back to the brain
                    ai_brain.add(mem);
                }
                brain_file.close();
            }
        }


        /**
         * @brief Takes a snapshot of the game and saves it to a text file.
         */
        void save_game()
        {
            // ofstream stands for Output File Stream. It creates/overwrites a file.
            ofstream save_file("savegame.txt");

            if (save_file.is_open())
            {
                // Write the core settings
                save_file << is_singleplayer << "\n";
                save_file << ai_difficulty << "\n";
                save_file << round_timer << "\n";

                // Write fighter 1's exact state
                save_file << p1_choice << "\n";
                save_file << fighter1->get_health() << "\n";
                save_file << fighter1->get_stamina() << "\n";
                save_file << fighter1->get_x() << "\n";
                save_file << fighter1->get_y() << "\n";

                // Write fighter 2's exact state
                save_file << p2_choice << "\n";
                save_file << fighter2->get_health() << "\n";
                save_file << fighter2->get_stamina() << "\n";
                save_file << fighter2->get_x() << "\n";
                save_file << fighter2->get_y() << "\n";

                save_file.close();
                write_line("[System] Game saved successfully!");

                save_brain();
            }
        }    

        /**
         * @brief Reads the snapshot from the text file and overwrites the characters.
         */
        void load_game()
        {
            // ifstream stands for Input File Stream. It reads an existing file.
            ifstream load_file("savegame.txt");

            if (load_file.is_open())
            {
                double p1_hp;
                double p1_stamina;
                double p1_x;
                double p1_y;
                double p2_hp;
                double p2_stamina;
                double p2_x;
                double p2_y;

                // Absorb lines we don't need anymore
                int junk;

                // Read the lines in the EXACT SAME ORDER it is written
                load_file >> junk >> junk >> round_timer;
                load_file >> junk >> p1_hp >> p1_stamina >> p1_x >> p1_y;
                load_file >> junk >> p2_hp >> p2_stamina >> p2_x >> p2_y;

                load_file.close();

                // Force the characters into the loaded state
                fighter1->load_state(p1_hp, p1_stamina, p1_x, p1_y);
                fighter2->load_state(p2_hp, p2_stamina, p2_x, p2_y);

                write_line("[System] Game loaded successfully!");
            }
        }

        /**
         * @brief Initializes the game, loading the background, all character sprites, and assigning players.
         */
        game(int mode, int p1_choice_in, int p2_choice_in, int difficulty)
        {
            load_brain();
            state = COUNTDOWN;
            round_timer = 90.0;
            countdown_timer = 240;

            // Save the choices so we can write them to the save file
            p1_choice = p1_choice_in;
            p2_choice = p2_choice_in;

            background_image = load_bitmap("arena", "battle_arena.png");

            glitch_bmp = load_bitmap("Glitch", "glitch_protocol.png");
            corrupted_bmp = load_bitmap("Corrupted", "corrupted_combo.png");
            immunoshield_bmp = load_bitmap("Immunoshield", "immunoshield.png");
            toxic_bmp = load_bitmap("Toxic", "toxic_dose.png");
            
            // Player 1 Sprites
            bitmap cs_right_bmp = load_bitmap("Computer_Science_Right", "cs_student_idle_right.png");
            bitmap cs_left_bmp = load_bitmap("Computer_Science_Left", "cs_student_idle_left.png");
            bitmap cs_jump_right_bmp = load_bitmap("Computer_Science_Jump_Right", "cs_student_jump_right.png");
            bitmap cs_jump_left_bmp = load_bitmap("Computer_Science_Jump_Left", "cs_student_jump_left.png");
            bitmap cs_crouch_right_bmp = load_bitmap("Computer_Science_Crouch_R", "cs_student_crouch_right.png");
            bitmap cs_crouch_left_bmp = load_bitmap("Computer_Science_Crouch_L", "cs_student_crouch_left.png");
            bitmap cs_attack_right_bmp = load_bitmap("Computer_Science_Attack_R", "cs_student_attack_right.png");
            bitmap cs_attack_left_bmp = load_bitmap("Computer_Science_Attack_L", "cs_student_attack_left.png");

            // Player 2 Sprites
            bitmap pharmacy_right_bmp = load_bitmap("Pharmacy_Right", "pharmacy_student_right.png");
            bitmap pharmacy_left_bmp = load_bitmap("Pharmacy_Left", "pharmacy_student_left.png");
            bitmap pharmacy_jump_right_bmp = load_bitmap("Pharmacy_Jump_Right", "pharmacy_student_jump_right.png");
            bitmap pharmacy_jump_left_bmp = load_bitmap("Pharmacy_Jump_Left", "pharmacy_student_jump_left.png");
            bitmap pharmacy_crouch_right_bmp = load_bitmap("Pharmacy_Crouch_R", "pharmacy_student_crouch_right.png");
            bitmap pharmacy_crouch_left_bmp = load_bitmap("Pharmacy_Crouch_L", "pharmacy_student_crouch_left.png");
            bitmap pharmacy_attack_right_bmp = load_bitmap("Pharmacy_Attack_R", "pharmacy_student_attack_right.png");
            bitmap pharmacy_attack_left_bmp = load_bitmap("Pharmacy_Attack_L", "pharmacy_student_attack_left.png");

            is_singleplayer = (mode == 1);
            ai_difficulty = difficulty;

            if (p1_choice == 1)
            {
                // Create unique sprites for Player 1
                sprite right = create_sprite(cs_right_bmp);
                sprite left = create_sprite(cs_left_bmp);
                sprite j_right = create_sprite(cs_jump_right_bmp);
                sprite j_left = create_sprite(cs_jump_left_bmp);
                sprite c_right = create_sprite(cs_crouch_right_bmp);
                sprite c_left = create_sprite(cs_crouch_left_bmp);
                sprite a_right = create_sprite(cs_attack_right_bmp);
                sprite a_left = create_sprite(cs_attack_left_bmp);
                fighter1 = new computer_science(10.0, 300.0, 
                                     right, left, j_right, j_left, c_right, c_left, a_right, a_left, right);
            }
            else
            {
                // Create unique sprites for Player 1
                sprite right = create_sprite(pharmacy_right_bmp);
                sprite left = create_sprite(pharmacy_left_bmp);
                sprite j_right = create_sprite(pharmacy_jump_right_bmp);
                sprite j_left = create_sprite(pharmacy_jump_left_bmp);
                sprite c_right = create_sprite(pharmacy_crouch_right_bmp);
                sprite c_left = create_sprite(pharmacy_crouch_left_bmp);
                sprite a_right = create_sprite(pharmacy_attack_right_bmp);
                sprite a_left = create_sprite(pharmacy_attack_left_bmp);
                fighter1 = new pharmacy(10.0, 300.0, 
                                     right, left, j_right, j_left, c_right, c_left, a_right, a_left, right);
            }

            // --- PLAYER 2 SETUP ---
            if (p2_choice == 1)
            {
                // Create unique sprites for Player 2 (using the exact same bitmaps!)
                sprite right = create_sprite(cs_right_bmp);
                sprite left = create_sprite(cs_left_bmp);
                sprite j_right = create_sprite(cs_jump_right_bmp);
                sprite j_left = create_sprite(cs_jump_left_bmp);
                sprite c_right = create_sprite(cs_crouch_right_bmp);
                sprite c_left = create_sprite(cs_crouch_left_bmp);
                sprite a_right = create_sprite(cs_attack_right_bmp);
                sprite a_left = create_sprite(cs_attack_left_bmp);
                // Note that the last argument is 'left' so Player 2 faces left at the start
                fighter2 = new computer_science(1050.0, 300.0, 
                                     right, left, j_right, j_left, c_right, c_left, a_right, a_left, left);
            }
            else
            {
                // Create unique sprites for Player 2
                sprite right = create_sprite(pharmacy_right_bmp);
                sprite left = create_sprite(pharmacy_left_bmp);
                sprite j_right = create_sprite(pharmacy_jump_right_bmp);
                sprite j_left = create_sprite(pharmacy_jump_left_bmp);
                sprite c_right = create_sprite(pharmacy_crouch_right_bmp);
                sprite c_left = create_sprite(pharmacy_crouch_left_bmp);
                sprite a_right = create_sprite(pharmacy_attack_right_bmp);
                sprite a_left = create_sprite(pharmacy_attack_left_bmp);
                fighter2 = new pharmacy(1050.0, 300.0, 
                                     right, left, j_right, j_left, c_right, c_left, a_right, a_left, left);
            }

            player1 = player(fighter1, A_KEY, D_KEY, W_KEY, S_KEY, SPACE_KEY, E_KEY, R_KEY);
            player2 = player(fighter2, LEFT_KEY, RIGHT_KEY, UP_KEY, DOWN_KEY, RIGHT_CTRL_KEY, RIGHT_SHIFT_KEY, RIGHT_ALT_KEY);
        }

        /**
         * @brief Secretly watches a Player's inputs and logs them to the ML memory bank.
         * @param distance The distance between the players.
         */
        void log_human_memory(player p, character* my_fighter, character* enemy_fighter)
        {
            // Calculate the distance from this specific player's perspective
            // If relative distance < 0, this player is to the left of the enemy
            double relative_distance = my_fighter->get_x() - enemy_fighter->get_x();

            ai_action current_action = AI_IDLE;
            bool is_critical = true;

            // Read this specific player's keys
            if (key_typed(p.attack_key))
            {
                current_action = AI_ATTACK;
            }
            else if (key_typed(p.ability_key_1))
            {
                current_action = AI_ABILITY_1;
            }
            else if (key_typed(p.ability_key_2))
            {
                current_action = AI_ABILITY_2;
            }
            else if (key_typed(p.jump_key))
            {
                current_action = AI_JUMP;
            }
            else if (key_down(p.crouch_key))
            {
                current_action = AI_CROUCH;
            }
            else
            {
                is_critical = false;

                if (key_down(p.left_key))
                {
                    if (relative_distance < 0) 
                    {
                        current_action = AI_MOVE_AWAY; // Enemy on the right, moving left means running away
                    }
                    else
                    {
                        current_action = AI_MOVE_CLOSER;
                    }
                }
                else if (key_down(p.right_key))
                {
                    if (relative_distance < 0)
                    {
                        current_action = AI_MOVE_CLOSER; // Enemy on the right, moving left means approaching
                    }
                    else
                    {
                        current_action = AI_MOVE_AWAY;
                    }
                }
            }

            // SMART LOGGING: Only record walking / idling every 10 frames to prevent spam
            static int logging_timer = 0;
            logging_timer++;

            if (is_critical || logging_timer >= 10)
            {
                game_memory memory;
                memory.distance = abs(relative_distance); // The ML Brain only cares about absolute distance
                memory.my_stamina = my_fighter->get_stamina();
                memory.enemy_stamina = enemy_fighter->get_stamina();
                memory.action_taken = current_action;
                
                ai_brain.add(memory);

                if (!is_critical)
                logging_timer = 0; // Reset the timer only if it was a walking/idle frame
            }
        }

        /**
         * @brief The Machine Learning Predictor (1-Nearest Neighbour).
         * Calculates the Euclidean disatnce across the 3D state space to find the closest memory.
         * 
         * @param current_distance The AI's current distance to the player.
         * @param my_stamina The AI's current stamina
         * @param enemy_stamina The Player's current stamina
         * @return The AI Action that the human took in the most similar situation.
         */
        ai_action predict_best_action(double current_distance, double my_stamina, double enemy_stamina)
        {
            // If the brain is empty (no data has been recorded), default to doing nothing
            if (length(ai_brain) == 0)
            {
                return AI_IDLE;
            }

            const double MAX_DISTANCE = 1200.0;
            const double MAX_STAMINA = 100.0;

            double smallest_difference = 999999.0; // Set the starting value to be massive
            ai_action best_action = AI_IDLE;

            // Search through every single memory we have ever recorded
            for (int i = 0; i < length(ai_brain); i++)
            {
                game_memory memory = ai_brain[i];

                // Normalise the data first so that everyting is fair
                double distance_difference = (memory.distance - abs(current_distance)) / MAX_DISTANCE;
                double my_stamina_difference = (memory.my_stamina - my_stamina) / MAX_STAMINA;
                double enemy_stamina_difference = (memory.enemy_stamina - enemy_stamina) / MAX_STAMINA;

                // Euclidean distance (a^2 + b^2 + c^2)
                // This calculates exactly how "different" this memory is from our current situation.
                double difference = (distance_difference * distance_difference) + 
                (my_stamina_difference * my_stamina_difference) + 
                (enemy_stamina_difference * enemy_stamina_difference);

                // Find the closest neighbour
                // If this is the most similar memory we have seen so far, remember its action!
                if (difference < smallest_difference)
                {
                    smallest_difference = difference;
                    best_action = memory.action_taken;
                }
            }

            // After searching the entire brain, return the best possible action
            return best_action;
        }

        /**
         * @brief Master update function. Called every frame to process input and update physics for all entities.
         */
        void update()
        {
            if (state == COUNTDOWN)
            {
                countdown_timer--; // Tick down the clock

                if (countdown_timer <= 0)
                {
                    state = PLAYING; // Start the match
                }
            }
            else if (state == PLAYING)
            {
                // Subtract 1 frame's worth of time
                round_timer -= (1.0 / 60.0);

                // Don't let it go below 0 and trigger Game Over status
                if (round_timer <= 0)
                {
                    round_timer = 0;
                    state = GAME_OVER;
                }

                // We always log Player 1 because they are always human
                log_human_memory(player1, fighter1, fighter2); // ML Data Logger: Record the human's actions

                player1.handle_input(fighter2);

                if (is_singleplayer == true)
                {
                    run_ai_logic();
                }
                else
                {
                    // If we are in Multiplayer, log Player 2's brain as well
                    log_human_memory(player2, fighter2, fighter1);
                    
                    player2.handle_input(fighter1);
                }

                fighter1->update();
                fighter2->update();

                // --- Fighter 1 Attacking Fighter 2
                // If they are attacking and have not already dealt damage this attack turn
                if (fighter1->is_currently_attacking() && !fighter1->has_already_hit())
                {
                    // Calculate the distance between them
                    double distance = abs(fighter1->get_x() - fighter2->get_x());
                    
                    // Check if fighter 2 is physically to the right of fighter 1
                    bool opponent_is_to_the_right = (fighter2->get_x() > fighter1->get_x());

                    // Check if figher 1 is facing fighter 2
                    bool facing_correctly = (fighter1->is_facing_right_direction() == opponent_is_to_the_right);

                    // If close enough (80 pixels), facing the right way and the opponent is not jumping/ crouching, do damage
                    if (distance < 80.0 && facing_correctly && !fighter2->get_is_jumping() && !fighter2->get_is_crouching())
                    {
                        // Apply damage
                        fighter2->take_damage(fighter1->get_attack_power(), fighter1->get_x());
                        fighter2->apply_knockback(fighter1->get_x());

                        // Finish dealing damage
                        fighter1->register_hit();
                    }
                }

                // --- Fighter 2 attacking fighter 1
                // If they are attacking and have not already dealt damage this attack turn
                if (fighter2->is_currently_attacking() && !fighter2->has_already_hit())
                {
                    // Calculate the distance between them
                    double distance = abs(fighter1->get_x() - fighter2->get_x());
                    
                    // Check if fighter 1 is physically to the right of fighter 2
                    bool opponent_is_to_the_right = (fighter1->get_x() > fighter2->get_x());

                    // Check if figher 2 is facing fighter 1
                    bool facing_correctly = (fighter2->is_facing_right_direction() == opponent_is_to_the_right);

                    // If close enough (80 pixels), facing the right way and the opponent is not jumping/ crouching, do damage
                    if (distance < 80.0 && facing_correctly && !fighter1->get_is_jumping() && !fighter1->get_is_crouching())
                    {
                        // Apply damage
                        fighter1->take_damage(fighter2->get_attack_power(), fighter2->get_x());
                        fighter1->apply_knockback(fighter2->get_x());

                        // Finish dealing damage
                        fighter2->register_hit();
                    }
                }

                // Check for any deaths
                if (fighter1->get_health() <= 0 || fighter2->get_health() <= 0)
                {
                    state = GAME_OVER; // Set the game state to be game over after there is one death
                }
            }
        }

        /**
         * @brief Renders the health bars for both players.
         * @param max_bar_width The maximum visual width of the health bars.
         * @param bar_height The height of the health bars.
         */
        void draw_health_bars(double max_bar_width, double bar_height) const
        {
            // Player 1 health bar
            double p1_health_pct = fighter1->get_health() / fighter1->get_max_health();
            fill_rectangle(COLOR_RED, 50, 50, max_bar_width, bar_height);
            fill_rectangle(COLOR_GREEN, 50, 50, max_bar_width * p1_health_pct, bar_height);

            // Player 2 health bar
            double p2_health_pct = fighter2->get_health() / fighter2->get_max_health();
            fill_rectangle(COLOR_RED, WINDOW_WIDTH - 50 - max_bar_width, 50, max_bar_width, bar_height);
            fill_rectangle(COLOR_GREEN, WINDOW_WIDTH - 50 - max_bar_width, 50, max_bar_width * p2_health_pct, bar_height);
        }

        /**
         * @brief Renders the countdown round timer at the top center of the screen.
         */
        void draw_game_timer() const
        {
            // Convert the timer into a solid integer using 'ceil' to display 15.1 seconds as 16.
            int display_time = ceil(round_timer);
            string timer_str = to_string(display_time);

            // Deciding the color
            color timer_color = COLOR_YELLOW;

            if (display_time <= 15)
            {
                timer_color = COLOR_RED; // Swtich the timer color to red when the timer reaches below 15 seconds
            }

            // Calculate perfectly centred X and Y coordinates
            int font_size = 65;
            string font_name = "arcade";

            // Use splashkit to know exactly how wide the text is so we can center it perfectly
            double text_w = text_width(timer_str, font_name, font_size);
            double text_x = (WINDOW_WIDTH / 2.0) - (text_w / 2.0);
            double text_y = 25.0; // Place it near the top

            // Drawing the outline (Shifts 2 pixels in every direction)
            int thickness = 2;
            draw_text(timer_str, COLOR_BLACK, font_name, font_size, text_x - thickness, text_y); // Left
            draw_text(timer_str, COLOR_BLACK, font_name, font_size, text_x + thickness, text_y); // Right
            draw_text(timer_str, COLOR_BLACK, font_name, font_size, text_x, text_y + thickness); // Down
            draw_text(timer_str, COLOR_BLACK, font_name, font_size, text_x, text_y - thickness); // Up

            // Draw the real text on top
            draw_text(timer_str, timer_color, font_name, font_size, text_x, text_y);
        }

        /**
         * @brief Renders the shield bars for players with active Immunoshield abilities.
         * @param max_bar_width The maximum visual width of the health bars.
         * @param bar_height The height of the shield bars.
         */
        void draw_shield_bars(double max_bar_width, double bar_height) const
        {
            double shield_max = 50.0; // The maximum health of hte Immunoshield
            double shield_height = 10.0;

            // Player 1 Shield
            if (fighter1->get_shield_timer() > 0)
            {
                double p1_shield_pct = fighter1->get_shield_health() / shield_max;

                // Draw a thin blue bar slightly below the main health bar
                fill_rectangle(COLOR_BLUE, 50, 85, max_bar_width * p1_shield_pct, shield_height);
            }

            // Player 2 Shield
            if (fighter2->get_shield_timer() > 0)
            {
                double p2_shield_pct = fighter2->get_shield_health() / shield_max;

                // Draw a thin blue bar slightly below the main health bar
                fill_rectangle(COLOR_CYAN, WINDOW_WIDTH - 50 - max_bar_width, 85, max_bar_width * p2_shield_pct, shield_height);
            }
        }

        /**
         * @brief Renders the stamina bars for both players.
         * @param stamina_height The height of the stamina bar.
         * @param max_bar_width The maximum visual width of the stamina bar.
         * @param p1_stamina_y The y-coordinate for Player 1's stamina bar.
         * @param p2_stamina_y The y-coordinate for Player 2's stamina bar.
         */
        void draw_stamina_bars(double stamina_height, double max_bar_width, double p1_stamina_y, double p2_stamina_y) const
        {
            color gold = rgb_color(225, 215, 0); // Yellowish-Gold colour

            if (fighter1->get_shield_timer() > 0)
            {
                p1_stamina_y += 15.0; // Push the stamina bar down 15 pixels if the shield is currently active
            }

            double p1_stamina_pct = fighter1->get_stamina() / fighter1->get_max_stamina();

            // Draw a grey background to see how much stamina is missing
            fill_rectangle(COLOR_GRAY, 50, p1_stamina_y, max_bar_width, stamina_height); 

            // Draw the gold bar on top of it
            fill_rectangle(gold, 50, p1_stamina_y, max_bar_width * p1_stamina_pct, stamina_height);

            if (fighter2->get_shield_timer() > 0)
            {
                p2_stamina_y += 15.0; // Push the stamina bar down 15 pixels if the shield is currently active
            }

            double p2_stamina_pct = fighter2->get_stamina() / fighter2->get_max_stamina();

            // Draw a gray background to see how much stamina is missing
            fill_rectangle(COLOR_GRAY, WINDOW_WIDTH - 50 - max_bar_width, p2_stamina_y, max_bar_width, stamina_height);

            // Draw the gold bar on top of it
            fill_rectangle(gold, WINDOW_WIDTH - 50 - max_bar_width, p2_stamina_y, max_bar_width * p2_stamina_pct, stamina_height);
        }

        /**
         * @brief Renders the ability icons and shrinking cooldown indicators for Player 1.
         * @param center_offset The offset to perfectly center the cooldown circle.
         * @param max_radius The full radius of the cooldown circle.
         */
        void draw_player_1_abilities(double center_offset, double max_radius) const
        {
            // Figure out which images to use
            bitmap p1_icon_1 = (p1_choice == 1) ? corrupted_bmp : immunoshield_bmp;
            bitmap p1_icon_2 = (p1_choice == 1) ? glitch_bmp : toxic_bmp;

            // Set the positions for the 2 icons
            double p1_icon1_x = 30.0;
            double p1_icon_y = WINDOW_HEIGHT - 80.0;
            double p1_icon2_x = 100.0;

            // Scale down the images
            drawing_options opt = option_scale_bmp(1, 1);

            // Draw the icons
            draw_bitmap(p1_icon_1, p1_icon1_x, p1_icon_y, opt);
            draw_bitmap(p1_icon_2, p1_icon2_x, p1_icon_y, opt);

            // Draw the keys (E and R)
            draw_text("E", COLOR_WHITE, p1_icon1_x, p1_icon_y - 15.0);
            draw_text("R", COLOR_WHITE, p1_icon2_x, p1_icon_y - 15.0);

            // Draw the shrinking circle cooldown
            ability p1_ability1 = fighter1->get_ability(0);
            ability p1_ability2 = fighter1->get_ability(1);

            double p1_ability1_pct = (double) p1_ability1.get_current_cooldown() / p1_ability1.get_max_cooldown();
            double p1_ability2_pct = (double) p1_ability2.get_current_cooldown() / p1_ability2.get_max_cooldown();

            if (p1_ability1_pct > 0)
            {
                // Draws a dark, 75% transparent circle that shrinks as the pct goes down from 1.0 to 0.0
                fill_circle(rgba_color(0, 0, 0, 180), p1_icon1_x + center_offset, p1_icon_y + center_offset, max_radius * p1_ability1_pct);
            }

            if (p1_ability2_pct > 0)
            {
                // Draws a dark, 75% transparent circle that shrinks as the pct goes down from 1.0 to 0.0
                fill_circle(rgba_color(0, 0, 0, 180), p1_icon2_x + center_offset, p1_icon_y + center_offset, max_radius * p1_ability2_pct);
            }
        }

        /**
         * @brief Renders the ability icons and shrinking cooldown indicators for Player 2.
         * @param center_offset The offset to perfectly center the cooldown circle.
         * @param max_radius The full radius of the cooldown circle.
         */
        void draw_player_2_abilities(double center_offset, double max_radius) const
        {
            // Figure out which images to use
            bitmap p2_icon_1 = (p2_choice == 1) ? glitch_bmp : immunoshield_bmp;
            bitmap p2_icon_2 = (p2_choice == 1) ? corrupted_bmp : toxic_bmp;

            // Set the positions for the 2 icons
            double p2_icon1_x = WINDOW_WIDTH - 150.0;
            double p2_icon_y = WINDOW_HEIGHT - 80.0;
            double p2_icon2_x = WINDOW_WIDTH - 80.0;

            // Draw the icons
            draw_bitmap(p2_icon_1, p2_icon1_x, p2_icon_y);
            draw_bitmap(p2_icon_2, p2_icon2_x, p2_icon_y);

            // Draw the keys (E and R)
            draw_text("R-Shift", COLOR_WHITE, p2_icon1_x, p2_icon_y - 15.0);
            draw_text("R-Alt", COLOR_WHITE, p2_icon2_x, p2_icon_y - 15.0);

            // Draw the shrinking circle cooldown
            ability p2_ability1 = fighter2->get_ability(0);
            ability p2_ability2 = fighter2->get_ability(1);

            double p2_ability1_pct = (double) p2_ability1.get_current_cooldown() / p2_ability1.get_max_cooldown();
            double p2_ability2_pct = (double) p2_ability2.get_current_cooldown() / p2_ability2.get_max_cooldown();

            if (p2_ability1_pct > 0)
            {
                // Draws a dark, 75% transparent circle that shrinks as the pct goes down from 1.0 to 0.0
                fill_circle(rgba_color(0, 0, 0, 180), p2_icon1_x + center_offset, p2_icon_y + center_offset, max_radius * p2_ability1_pct);
            }

            if (p2_ability2_pct > 0)
            {
                // Draws a dark, 75% transparent circle that shrinks as the pct goes down from 1.0 to 0.0
                fill_circle(rgba_color(0, 0, 0, 180), p2_icon2_x + center_offset, p2_icon_y + center_offset, max_radius * p2_ability2_pct);
            }
        }

        /**
         * @brief Renders the 3-2-1-FIGHT countdown sequence before the match begins.
         */
        void draw_countdown() const
        {
            string text = "";

            // Convert the 60 frames per second timer into seconds
            if (countdown_timer > 180)
            {
                text = "3";
            }
            else if (countdown_timer > 120)
            {
                text = "2";
            }
            else if (countdown_timer > 60)
            {
                text = "1";
            }
            else
            {
                text = "FIGHT";
            }

            // Center the text
            double text_w = text_width(text, "arcade", 150);
            draw_text(text, COLOR_RED, "arcade", 150, (WINDOW_WIDTH / 2) - (text_w / 2), (WINDOW_HEIGHT / 2) - 75);
        }

        /**
         * @brief Renders a visual multiplier indicator for Player 1 when a damage buff is active.
         * @param p1_stamina_y The y-coordinate for Player 1's stamina bar, used for positioning.
         * @param stamina_height The height of the stamina bar.
         */
        void draw_player_1_multiplier(double p1_stamina_y, double stamina_height) const
        {
            // Player 1 Multiplier
            double p1_multiplier = fighter1->get_damage_multiplier();

            // Only draw it if the multiplier is active (greater than 1.0)
            if (p1_multiplier > 1.0)
            {
                double p1_multiplier_y = p1_stamina_y + stamina_height + 5.0;

                // Draw a small black background box
                fill_rectangle(COLOR_BLACK, 50, p1_multiplier_y, 60, 20);

                // Convert the multiplier into a string with 2 decimal places
                string multiplier_text = "x" + to_string(p1_multiplier, 2);

                // Draw the text in white inside the box
                draw_text(multiplier_text, COLOR_WHITE, "arial", 14, 55, p1_multiplier_y + 2);
            }
        }

        /**
         * @brief Renders a visual multiplier indicator for Player 2 when a damage buff is active.
         * @param p2_stamina_y The y-coordinate for Player 2's stamina bar, used for positioning.
         * @param stamina_height The height of the stamina bar.
         */
        void draw_player_2_multiplier(double p2_stamina_y, double stamina_height) const
        {
            // Player 2 Multiplier
            double p2_multiplier = fighter2->get_damage_multiplier();

            // Only draw it if the multiplier is active (greater than 1.0)
            if (p2_multiplier > 1.0)
            {
                double p2_multiplier_y = p2_stamina_y + stamina_height + 5.0;

                // Align the box to the right side of the screen
                double box_x = WINDOW_WIDTH - 50 - 60; // 60 is the width of the box

                // Draw a small black background box
                fill_rectangle(COLOR_BLACK, box_x, p2_multiplier_y, 60, 20);

                // Convert the multiplier into a string with 2 decimal places
                string multiplier_text = "x" + to_string(p2_multiplier, 2);

                // Draw the text in white inside the box
                draw_text(multiplier_text, COLOR_WHITE, "arial", 14, box_x + 8, p2_multiplier_y + 2);
            }
        }

        /**
         * @brief Renders the game over screen, announcing the winner.
         */
        void draw_game_over() const
        {
            string text = "";
            double text_w = 0.0;

            if (fighter1->get_health() <= 0)
            {
                text = "PLAYER 2 WINS!";
                text_w = text_width(text, "arial", 50);
                draw_text(text, COLOR_RED, "arial", 50, (WINDOW_WIDTH / 2) - (text_w / 2), WINDOW_HEIGHT / 2);
            }
            else
            {
                text = "PLAYER 1 WINS!";
                text_w = text_width(text, "arial", 50);
                draw_text(text, COLOR_RED, "arial", 50, (WINDOW_WIDTH / 2) - (text_w / 2), WINDOW_HEIGHT / 2);
            }
        }

        /**
         * @brief Master draw function. Clears the screen and draws the background and all characters.
         */
        void draw() const
        {
            clear_screen(COLOR_WHITE);

            if (background_image != nullptr)
            {
                draw_bitmap(background_image, 0, 0);
            }

            fighter1->draw();
            fighter2->draw();

            double max_bar_width = 400.0;  // The size of a full health bar
            double bar_height = 30.0;

            // --- DRAW HEALTH BARS ---
            draw_health_bars(max_bar_width, bar_height);

            // --- DRAW THE GAME TIMER ---
            draw_game_timer();

            // --- DRAW SHIELD BARS (If Active) ---
            draw_shield_bars(max_bar_width, bar_height);

            double stamina_height = 10.0;

            // Player 1 Stamina
            double p1_stamina_y = 85.0;  // Default stamina bar y position

            // Player 2 Stamina
            double p2_stamina_y = 85.0;

            // --- DRAW STAMINA BARS ---
            draw_stamina_bars(stamina_height, max_bar_width, p1_stamina_y, p2_stamina_y);

            
            // Draw the circle perfectly over the center of the icon
            double center_offset = 25.0;
            double max_radius = 25.0;

            // --- DRAW PLAYER 1 ABILITIES ---
            draw_player_1_abilities(center_offset, max_radius);
            // --- DRAW PLAYER 2 ABILITIES ---
            draw_player_2_abilities(center_offset, max_radius);

            // --- DRAW COUNTDOWN TEXT ---
            if (state == COUNTDOWN)
            {
                draw_countdown();
            }

            // --- DRAW DAMAGE MULTIPLIERS (If Activated) ---
            draw_player_1_multiplier(p1_stamina_y, stamina_height);
            draw_player_2_multiplier(p2_stamina_y, stamina_height);

            if (state == GAME_OVER)
            {
                draw_game_over();
            }
        }

        /**
         * @brief Gets the current game state.
         * @return The current game state.
         */
        game_state get_state() const
        {
            return state;
        }
};