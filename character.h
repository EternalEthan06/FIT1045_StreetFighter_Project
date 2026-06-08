# pragma once
# include "splashkit.h"
# include "splashkit-arrays.h"
# include "ability.h"
# include "common.h"

/**
 * @class character
 * @brief Represents a fighter in the game.
 * Manages the character's stats, position, animation state, and combat mechanics.
 */
class character
{
    // Position and Movement
    double x;
    double y;
    double dx;
    double dy;

    // Rendering (Splashkit) — stays private, subclasses don't need direct access

    protected:
        double attack_power;
        int defense;
        double health;
        double max_health;
        string name;

        dynamic_array<damage_text> floating_texts; // Stores all active damage numbers

        double stamina;
        double max_stamina;
        timer stamina_timer;

        // Ability needs
        ability abilities[2];
        double base_attack_power;
        bool is_glitched;
        int glitch_timer;
        bool is_poisoned;
        int poison_timer;

    private:
        sprite sprite_right;
        sprite sprite_left;
        sprite sprite_jump_right;
        sprite sprite_jump_left;
        sprite sprite_crouch_right;
        sprite sprite_crouch_left;
        sprite sprite_attack_right;
        sprite sprite_attack_left;
        sprite active_sprite;
        bool is_facing_right;
        bool is_jumping;
        bool is_crouching;
        bool is_attacking;
        int attack_frames;
        bool attack_has_hit;

    public:
        /**
         * @brief Default constructor for a character.
         * Initializes a blank character with default stats and zero position.
         */
        character()
        {
            attack_power = 0.0;
            base_attack_power = attack_power;
            defense = 0;
            health = 0.0;
            max_health = health;
            name = "Unknown";

            max_stamina = 0.0;
            stamina = max_stamina;
            static int character_count = 0;
            character_count++;
            stamina_timer = create_timer("Stamina_" + to_string(character_count));
            start_timer(stamina_timer);

            is_glitched = false;
            glitch_timer = 0;
            is_poisoned = false;
            poison_timer = 0;

            x = 0.0;
            y = 0.0;
            dx = 0.0;
            dy = 0.0;

            sprite_right = nullptr;
            sprite_left = nullptr;
            sprite_jump_right = nullptr;
            sprite_jump_left = nullptr;
            sprite_crouch_right = nullptr;
            sprite_crouch_left = nullptr;
            sprite_attack_right = nullptr;
            sprite_attack_left = nullptr;
            active_sprite = nullptr;
            is_facing_right = true;
            is_jumping = false;
            is_crouching = false;
            is_attacking = false;

            attack_frames = 0;
            attack_has_hit = false;
        }

        /**
         * @brief Parameterized constructor for a character.
         * Sets up the character's stats and assigns all directional and action sprites.
         */
        character(double attack_power, int defense, double health, string name, 
                  double x, double y,
                  sprite right_sprite, sprite left_sprite,
                  sprite jump_right, sprite jump_left,
                  sprite crouch_right, sprite crouch_left, 
                  sprite attack_right, sprite attack_left,
                  sprite active_sprite)
        {
            this->attack_power = attack_power;
            this->base_attack_power = attack_power;
            this->defense = defense;
            this->health = health;
            this->max_health = health;
            this->name = name;

            this->max_stamina = 100.0;
            this->stamina = max_stamina;
            static int character_count = 0;
            character_count++;
            this->stamina_timer = create_timer("Stamina_" + to_string(character_count));
            start_timer(stamina_timer);

            this->is_glitched = false;
            this->glitch_timer = 0;
            this->is_poisoned = false;
            this->poison_timer = 0;

            this->x = x;
            this->y = y;

            this->sprite_right = right_sprite;
            this->sprite_left = left_sprite;
            this->sprite_jump_right = jump_right;
            this->sprite_jump_left = jump_left;
            this->sprite_crouch_right = crouch_right;
            this->sprite_crouch_left = crouch_left;
            this->sprite_attack_right = attack_right;
            this->sprite_attack_left = attack_left;
            this->active_sprite = active_sprite;
            
            // Set initial x and y to match the active sprite's starting position
            if (this->active_sprite != nullptr)
            {
                sprite_set_x(this->active_sprite, this->x);
                sprite_set_y(this->active_sprite, this->y);
            }
            
            this->dx = 0.0;
            this->dy = 0.0;
            
            this->is_facing_right = (this->active_sprite == this->sprite_right);
            this->is_jumping = false;
            this->is_crouching = false;
            this->is_attacking = false;
            this->attack_frames = 0;
            this->attack_has_hit = false;
        }

        /**
         * @brief Spawns a floating damage number above the character.
         */
        void spawn_damage_text(double damage)
        {
            damage_text text;
            text.amount = damage;
            text.x = this->x + 30; // Offset it so it floats near their head
            text.y = this->y -20; // Start slightly above them
            text.lifetime = 60; // Display the text for 60 frames (1 second)

            floating_texts.add(text); // Add it to the list
        }

        /**
         * @brief Updates the active sprite based on the character's current state.
         * Resolves whether to display jumping, crouching, or idle sprites, and which direction to face.
         */
        void update_animation()
        {
            if (is_attacking)
            {
                active_sprite = is_facing_right ? sprite_attack_right : sprite_attack_left;
            }
            else if (is_crouching)
            {
                active_sprite = is_facing_right ? sprite_crouch_right : sprite_crouch_left;
            }
            else if (is_jumping)
            {
                active_sprite = is_facing_right ? sprite_jump_right : sprite_jump_left;
            }
            else
            {
                active_sprite = is_facing_right ? sprite_right : sprite_left;
            }
        }

        /**
         * @brief Updates the character's stamina.
         * Drains stamina while crouching, regenerates stamina while idle.
         */
        void update_stamina()
        {
            if (is_crouching)
                {
                    stamina -= 2.0; // Drain 2 stamina every 50ms while crouching

                    if (stamina <= 0)
                    {
                        stamina = 0;
                        stand(); // Force to stand
                    }
                }
                // Regenerate stamina if not jumping ans attacking
                else if (!is_jumping && !is_attacking)
                {
                    // Regenerate 1 stamina every 50ms if resting
                    stamina += 1.0;

                    if (stamina > max_stamina)
                    {
                        stamina = max_stamina;
                    }
                }
                // Reset the stopwatch so it can count the next 50ms

                reset_timer(stamina_timer);
        }

        /**
         * @brief Handles the toxic dose damage over time effect.
         * Ticks down the poison timer and deals damage every second.
         */
        void update_poison_timer()
        {
            poison_timer--;

            // Only deal damage every 60 frames (1 second)
            if (poison_timer % 60 == 0)
            {
                health -= 2.0; // Deal 2.0 attack damage for every second of the character being poisoned
                spawn_damage_text(2.0);
            }

            // Stop the poison effect after the timer has run out
            if (poison_timer <= 0)
            {
                is_poisoned = false;
            }
        }

        /**
         * @brief Handles the glitch protocol effect.
         * Ticks down the glitch timer and restores normal control when finished.
         */
        void update_glitch_timer()
        {
            glitch_timer--; // Start counting down the timer of the glitch
            // Reset the boolean back to false to prevent the character from 
            // continuously being randomly controlled
            if (glitch_timer <= 0)
            {
                is_glitched = false; 
            }
        }

        /**
         * @brief Handles the duration of an attack.
         * Ticks down the attack timer and resets attack state and power when finished.
         */
        void update_attack_timer()
        {
            attack_frames--;
                if (attack_frames <= 0)
                {
                    is_attacking = false;
                    attack_power = base_attack_power; // Reset the attack power for (CS Student's ability)
                }
        }

        /**
         * @brief Animates the floating damage numbers.
         * Moves all damage texts upwards and decreases their lifetime.
         */
        void update_damage_texts()
        {
            // Move all the damage text upwards and decrease their timer
            for (int i = 0; i < length(floating_texts); i++)
            {
                floating_texts[i].lifetime--;
                floating_texts[i].y -= 0.75; // Float upwards by 0.75 pixel every frame
            }

            // Removing the damage texts that have their lifetime ended
            for (int i = 0; i < length(floating_texts); i++)
            {
                if (floating_texts[i].lifetime <= 0)
                {
                    floating_texts.remove(i);
                }
            }
        }

        /**
         * @brief Prevents the character from moving off the edges of the screen.
         */
        void prevent_collision()
        {
            // Prevent the character from going out of the screen (Left boundary)
            if (x <= 0)
            {
                x = 0;  // Stop at that position
                dx = 0; // Stop moving
            }

            // Prevent the character from going out of the screen (Right boundary)
            if (x >= 1100)
            {
                x = 1100;  // Stop at that position
                dx = 0;    // Stop moving
            }
        }

        /**
         * @brief Updates the character's physics (gravity, velocity, collisions) and syncs the sprite.
         * Should be called once per frame.
         */
        virtual void update()
        {
            // --- TIME_BASED STAMINA TICK ---
            // This block is triggered every 50 milliseconds (20 times per second)
            if (timer_ticks(stamina_timer) >= 50)
            {
                update_stamina();
            }

            // --- ABILITY COUNTDOWN TIMER ---
            abilities[0].update_cooldown();
            abilities[1].update_cooldown();

            // --- POISON TIMER LOGIC ---
            if (is_poisoned)
            {
                update_poison_timer();
            }

            // --- GLITCH TIMER LOGIC ---
            if (is_glitched)
            {
                update_glitch_timer();
            }

            // --- ATTACK TIMER LOGIC ---
            if (is_attacking)
            {
                update_attack_timer();
            }

            // --- GRAVITY AND JUMPING LOGIC ---
            // If the character is in the air (y is less than 300) or moving upwards (dy < 0)
            if (y < 300 || dy < 0) 
            {
                dy += 0.8; // Gravity constantly pulls them down by increasing their downward velocity
            }

            // --- DRAWING DAMAGE TEXTS ---
            update_damage_texts();
            
            // --- APPLY VELOCITY TO POSITION ---
            x += dx;
            y += dy;

            // --- GROUND COLLISION ---
            // Prevent the character from falling through the floor
            if (y >= 300) 
            {
                y = 300;            // Snap back to ground level
                dy = 0;             // Stop falling
                is_jumping = false; // They have landed!
            }

            // --- WINDOW COLLISION ---
            prevent_collision();

            update_animation(); // Set the correct sprite before syncing its position

            // --- SYNC SPRITE ---
            if (active_sprite != nullptr)
            {
                sprite_set_x(active_sprite, x);
                sprite_set_y(active_sprite, y);
            }
        }

        /**
         * @brief Renders the character's active sprite to the screen.
         */
        void draw() const
        {
            if (active_sprite != nullptr)
            {
                draw_sprite(active_sprite);
            }

            // Draw all floating damage numbers
            for (int i = 0; i < length(floating_texts); i++)
            {
                // Convert the double damage value into a string
                string damage_string = "-" + to_string(floating_texts[i].amount, 2);

                // Draw the damage numbers in red
                draw_text(damage_string, COLOR_RED, "arial", 20, floating_texts[i].x, floating_texts[i].y);
            }
        }

        /**
         * @brief Triggers the crouching state, stopping horizontal movement.
         */
        void crouch()
        {
            // Only allow crouching if they are on the ground (not jumping)
            if (!is_jumping && stamina >= 15.0)
            {
                is_crouching = true;
                dx = 0; // Stop horizontal movement when crouching
            }
        }

        /**
         * @brief Ends the crouching state, returning the character to standing.
         */
        void stand()
        {
            is_crouching = false; // Stand back up when the key is released
        }

        /**
         * @brief Triggers a jump by applying an upward velocity.
         */
        void jump()
        {
            // Only allow jumping if they are on the ground and NOT crouching and sufficient stamina
            if (!is_jumping && !is_crouching && stamina >= 20.0)
            {
                dy = -17.5; // Apply a strong negative velocity to shoot them upwards
                is_jumping = true; // Mark them as in the air
                stamina -= 20.0; // Subtract instant cost
            }
        }

        /**
         * @brief Moves the character to the left.
         */
        void move_left()
        {
            if (is_crouching || is_attacking) return; // Prevent movement if currently crouching or attacking

            dx = -5.0;
            is_facing_right = false;
        }

        /**
         * @brief Moves the character to the right.
         */
        void move_right()
        {
            if (is_crouching || is_attacking) return; // Prevent movement if currently crouching or attacking

            dx = 5.0;
            is_facing_right = true;
        }

        /**
         * @brief Stops the character's horizontal movement.
         */
        void stop_moving()
        {
            dx = 0.0;
        }

        /**
         * @brief Checks if an attack is hit.
         * @return A boolean whether or not has an attack hit
         */
        bool has_already_hit() const
        {
            return attack_has_hit;
        }
        
        /**
         * @brief Sets the attack_has_hit boolean to be true.
         */
        void register_hit()
        {
            attack_has_hit = true;
        }

        /**
         * @brief Initiates an attack sequence.
         */
        void perform_attack()
        {
            // Reset the attack has hit boolean to be false to prevent multiple hits
            attack_has_hit = false;

            // Only allow attack if they are on the ground, not crouching, and not already attacking and also sufficient stamina
            if (!is_jumping && !is_crouching && !is_attacking && stamina >= 15.0)
            {
                is_attacking = true;
                attack_frames = 15; // Attack lasts for 15 frames
                dx = 0.0; // Stop horizontal movement during attack
                stamina -= 15.0; // Subtract instant cost
            }
        }

        /**
         * @brief Checks if the character is currently attacking.
         */
        bool is_currently_attacking() const
        {
            return is_attacking;
        }

        /**
         * @brief Calculates and applies damage taken by the character.
         * @param raw_damage The base damage received.
         */
        virtual void take_damage(double raw_damage, double attacker_x)
        {
            double effective_damage = raw_damage - defense * 0.5;

            // Prevent accidental healing
            if (effective_damage < 0)
            {
                effective_damage = 0;
            }

            health -= effective_damage;

            // Spawn the text whenever an attack lands
            if (effective_damage > 0)
            {
                spawn_damage_text(effective_damage);
            }
        }

        /**
         * @brief Artificially forces the character's state for loading from a save file.
         */
        void load_state(double loaded_health, double loaded_stamina, double loaded_x, double loaded_y)
        {
            health = loaded_health;
            stamina = loaded_stamina;
            x = loaded_x;
            y = loaded_y;

            // Reset any mid-air or mid-attack animations to prevent freezing
            is_jumping = false;
            is_crouching = false;
            is_attacking = false;
            dx = 0;
            dy = 0;
        }

        /**
         * @brief Gets the attack power of the character.
         * @return The attack power value
         */
        double get_attack_power() const
        {
            return attack_power;
        }
        
        /**
         * @brief Gets the current health of the character.
         * @return The health value.
         */
        double get_health() const 
        {
            return health;
        }

        /**
         * @brief Gets the max health of the character.
         * @return The max health value.
         */
        double get_max_health() const
        {
            return max_health;
        }

        /**
         * @brief Gets the current stamina of the character.
         * @return The stamina value.
         */
        double get_stamina() const
        {
            return stamina;
        }

        /**
         * @brief Gets the max stamina of the character.
         * @return The max stamina value.
         */
        double get_max_stamina() const
        {
            return max_stamina;
        }

        /**
         * @brief Gets the current damage multiplier.
         * @return The active multiplier (returns 1.0 if no ability is active).
         */
        double get_damage_multiplier() const
        {
            if (base_attack_power > 0)
            {
                return attack_power / base_attack_power;
            }

            return 1.0;
        }

        /**
         * @brief Gets the shield health. Overriden by subclasses that use shields.
         */
        virtual double get_shield_health() const
        {
            return 0.0;
        }

        /**
         * @brief Gets the shield timer. Overridden by subclasses that use shields.
         */
        virtual int get_shield_timer() const
        {
            return 0;
        }

        /**
         * @brief Gets the current x coordinate of the character.
         * @return The x coordinate.
         */
        double get_x() const
        {
            return x;
        }

        /**
         * @brief Gets the current y coordinate of the character.
         * @return The y coordinate.
         */
        double get_y() const 
        {
            return y;
        }

        /**
         * @brief Checks if the character is currently jumping.
         * @return True if the character is jumping, false otherwise.
         */
        bool get_is_jumping() const
        {
            return is_jumping;
        }

        /**
         * @brief Checks if the character is currently crouching.
         * @return True if the character is crouching, false otherwise.
         */
        bool get_is_crouching() const
        {
            return is_crouching;
        }

        /**
         * @brief Gets an ability from the character's ability slots.
         * @param index The index of the ability (0 or 1).
         * @return The requested ability.
         */
        ability get_ability(int index) const
        {
            return abilities[index];
        }

        /**
         * @brief Gets the character's facing direction.
         * @return True if the character is facing right, false otherwise.
         */
        bool is_facing_right_direction() const
        {
            return is_facing_right;
        }

        /**
         * @brief Pushes the character away from the attacker's position.
         */
        void apply_knockback(double attacker_x)
        {
            // If the attacker is to the left, push to the right
            if (attacker_x < x)
            {
                x += 40.0; // Slide back
            }

            // If the attacker is to the right, push to the left
            if (attacker_x > x)
            {
                x -= 40.0;
            }
        }

        /**
         * @brief Activates the character's special ability (Healing).
         * 
         * If the ability is off cooldown, it restores 25 health to the character,
         * ensuring their health does not exceed their maximum health limit.
         * Once used, the ability goes on a 300-frame cooldown (approx. 5 seconds).
         */
        virtual void use_ability(int slot_index, character* opponent)
        {
            // Get the requested ability
            ability& active_ability = abilities[slot_index];

            // Check if it's ready
            if (active_ability.is_ready())
            {
                if (active_ability.get_name() == "None")
                {
                    health += active_ability.get_power();

                    if (health > max_health) 
                    {
                        health = max_health; // Don't overheal
                    }
                }
                else if (active_ability.get_name() == "Damage")
                {
                    attack_power += 5;
                }

                // Start the cooldown
                active_ability.start_cooldown(); 
            }
        }

        /**
         * @brief Applies the glitch effect to this character.
         * @param duration How many frames the glitch lasts.
         */
        void apply_glitch(int duration)
        {
            is_glitched = true;
            glitch_timer = duration;
        }

        /**
         * @brief Checks if the character is currently glitched.
         * @return True if the character is glitched.
         */
        bool get_is_glitched() const
        {
            return is_glitched;
        }

        /**
         * @brief Applies a poison effect to the character.
         * @param duration The number of frames the poison will last.
         */
        void apply_poison(int duration)
        {
            is_poisoned = true;
            poison_timer = duration;
        }
};
