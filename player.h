# pragma once
# include "splashkit.h"
# include "character.h"

/**
 * @class player
 * @brief Represents the human mapping their keyboard inputs to a character.
 */
class player
{
    public:
        character* controlled_character;
        
        key_code left_key;
        key_code right_key;
        key_code jump_key;
        key_code crouch_key;
        key_code attack_key;
        key_code ability_key_1;
        key_code ability_key_2;

        /**
         * @brief Default constructor for a player. Assigns default keys.
         */
        player()
        {
            controlled_character = nullptr;
            left_key = A_KEY;
            right_key = D_KEY;
            jump_key = W_KEY;
            crouch_key = S_KEY;
            attack_key = SPACE_KEY;
            ability_key_1 = E_KEY;
            ability_key_2 = R_KEY;
        }

        /**
         * @brief Parameterized constructor for a player.
         * @param c Pointer to the character this player controls.
         * @param left The key to move left.
         * @param right The key to move right.
         * @param jump The key to jump.
         * @param crouch The key to crouch.
         * @param attack The key to attack.
         * @param ability The key to use an ability.
         */
        player(character* c, key_code left, key_code right, key_code jump, key_code crouch, key_code attack, key_code ability_1, key_code ability_2)
        {
            this->controlled_character = c;
            this->left_key = left;
            this->right_key = right;
            this->jump_key = jump;
            this->crouch_key = crouch;
            this->attack_key = attack;
            this->ability_key_1 = ability_1;
            this->ability_key_2 = ability_2;
        }

        /**
         * @brief Reads keyboard input from SplashKit and calls the appropriate action methods on the character.
         */
        void handle_input(character* opponent)
        {
            if (controlled_character == nullptr) return;

            // Wrap controls to handle glitch status (does not read user inputs)
            if (controlled_character->get_is_glitched())
            {
                // --- GLITCHED: Random controls take over ---

                // Random movement (pick one each frame)
                int move_roll = rnd(0, 2); // 0, 1, or 2
                if (move_roll == 0)
                {
                    controlled_character->move_left();
                }
                else if (move_roll == 1)
                {
                    controlled_character->move_right();
                }
                else
                {
                    controlled_character->stop_moving();
                }

                // Random jump (small chance per frame so it's not constant)
                if (rnd() < 0.05)
                {
                    controlled_character->jump();
                }

                // Random crouch / stand
                if (rnd() < 0.03)
                {
                    controlled_character->crouch();
                }
                else
                {
                    controlled_character->stand();
                }

                // Random attack
                if (rnd() < 0.05)
                {
                    controlled_character->perform_attack();
                }

                // Random ability usage (very rare, uses their abilities against them)
                if (rnd() < 0.02)
                {
                    controlled_character->use_ability(rnd(0, 1), opponent);
                }
            }
            else
            {
                // Handle Crouching
                // We use key_down because you stay crouched as long as you hold the key
                if (key_down(crouch_key))
                {
                    controlled_character->crouch();
                }
                else
                {
                    controlled_character->stand(); // Stand up if the key is released
                }

                // Handle Jumping
                // We use key_typed so they only jump once per key press, instead of flying by holding it
                if (key_typed(jump_key))
                {
                    controlled_character->jump();
                }

                // Handle Attacking
                // We use key_typed so they must press it for each attack
                if (key_typed(attack_key))
                {
                    controlled_character->perform_attack();
                }

                // Handle ability usage
                // We use key_typed so they must press it for each ability usage
                // Handle first ability usage
                if (key_typed(ability_key_1))
                {
                    controlled_character->use_ability(0, opponent);
                }

                // Handle second ability usage
                if (key_typed(ability_key_2))
                {
                    controlled_character->use_ability(1, opponent); 
                }

                // Handle Left/Right Movement
                if (key_down(left_key))
                {
                    controlled_character->move_left();
                }
                else if (key_down(right_key))
                {
                    controlled_character->move_right();
                }
                else
                {
                    controlled_character->stop_moving();
                }
            }
        }
};