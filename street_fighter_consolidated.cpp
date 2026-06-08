#include "splashkit.h"
#include "splashkit-arrays.h"
#include "utilities.h"
#include <math.h>
#include <iostream>
#include <fstream>

using std::ofstream;
using std::ifstream;
using std::string;

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

/**
 * @class ability
 * @brief Represents a special move or ability a character can perform.
 * Contains information about its damage, cooldown, and description.
 */
class ability
{
    string name;
    string type;          // The type of ability that it could be (e.g. Defense or Attack)
    double power_value;   // How much it heals or damages
    int max_cooldown;     // How many frames the cooldown takes
    int current_cooldown; // The active timer counting down

    public:
        /**
         * @brief Default constructor. Initializes an empty ability with zero cooldown and damage.
         */
        ability()
        {
            name = "None";
            type = "None";
            power_value = 0.0;
            max_cooldown = 0;
            current_cooldown = 0;
        }

        /**
         * @brief Parameterized constructor. Creates a new ability with specific properties.
         * @param name The name of the ability.
         * @param type The type of the ability ("Defense" or "Attack").
         * @param power_value The amount of damage or healing this ability does.
         * @param max_cooldown The cooldown duration in frames.
         */
        ability(string name, string type, double power_value, int max_cooldown)
        {
            this->name = name;
            this->type = type;
            this->power_value = power_value;
            this->max_cooldown = max_cooldown;
            this->current_cooldown = 0;
        }

        /**
         * @brief Gets the maximum cooldown duration of the ability.
         * @return The maximum cooldown in frames.
         */
        int get_max_cooldown() const
        {
            return max_cooldown;
        }

        /**
         * @brief Gets the current active cooldown timer of the ability.
         * @return The remaining cooldown in frames.
         */
        int get_current_cooldown() const
        {
            return current_cooldown;
        }

        /**
         * @brief Checks if the ability can be used.
         * @return Boolean value if the ability can be used.
         */
        bool is_ready() const
        {
            return current_cooldown <= 0;
        }

        /**
         * @brief Puts the ability on cooldown
         */
        void start_cooldown()
        {
            current_cooldown = max_cooldown;
        }

        /**
         * @brief Counts down the cooldown which will be called every frame.
         */
        void update_cooldown()
        {
            if (current_cooldown > 0)
            {
                current_cooldown--;
            }
        }

        /**
         * @brief Activates the ability if it is not on cooldown.
         * @return The bonus damage provided by the ability.
         */
        double activate()
        {
            return 0.0;
        }

        /**
         * @brief Gets the name of the ability.
         * @return String of the name of the ability.
         */
        string get_name() const
        {
            return name;
        }

        /**
         * @brief Gets the power value of the ability.
         * @return Double value of the power value of the ability.
         */
        double get_power() const
        {
            return power_value;
        }
};

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

/**
 * @class computer_science
 * @brief Represents the computer science student as a character.
 */
class computer_science : public character
{
    public:
    /**
     * @brief Constructs a new computer_science character with all required sprites.
     * @param start_x The starting X position.
     * @param start_y The starting Y position.
     * @param right_sprite Sprite for facing right.
     * @param left_sprite Sprite for facing left.
     * @param jump_right Sprite for jumping right.
     * @param jump_left Sprite for jumping left.
     * @param crouch_right Sprite for crouching right.
     * @param crouch_left Sprite for crouching left.
     * @param attack_right Sprite for attacking right.
     * @param attack_left Sprite for attacking left.
     * @param active_sprite The initial active sprite.
     */
    computer_science(double start_x, double start_y,
        sprite right_sprite, sprite left_sprite,
        sprite jump_right, sprite jump_left,
        sprite crouch_right, sprite crouch_left,
        sprite attack_right, sprite attack_left,
        sprite active_sprite) : 
        character(10.0, 5, 100.0, "Computer Science", start_x, start_y,
        right_sprite, left_sprite,
        jump_right, jump_left,
        crouch_right, crouch_left,
        attack_right, attack_left,
        active_sprite)
    {
        // Initialise their two unique abilites
        abilities[0] = ability("Corrupted Combo", "Buff", 0.0, 600); // 10 second cooldown
        abilities[1] = ability("Glitch Protocol", "Debuff", 0.0, 900); // 15 second cooldown
    }

    /** 
     * @brief Glitch Protocol: Takes over the opponent's controls for 3 seconds.
     */
    void glitch_protocol(character* opponent)
    {
        opponent->apply_glitch(180); // 3 seconds * 60 fps = 180 frames
    }

    /**
     * @brief Corrupted combo ability which applies a damage multiplier to the character's attack damage.
     */
    void corrupted_combo()
    {
        double base_attack_power = attack_power;

        double multiplier = 1.0 + rnd();

        attack_power = base_attack_power * multiplier;
    }

    /**
     * @brief Overridden ability for Computer Science.
     * Slot 0: Corrupted Combo (Buff) — boosts attack power.
     * Slot 1: Glitch Protocol (Debuff) — damages the opponent.
     */
    void use_ability(int slot_index, character* opponent) override
    {
        ability& active_ability = abilities[slot_index];

        if (active_ability.is_ready())
        {
            if (active_ability.get_name() == "Corrupted Combo")
            {
                corrupted_combo();
            }
            else if (active_ability.get_name() == "Glitch Protocol")
            {
                glitch_protocol(opponent);
            }

            active_ability.start_cooldown();
        }
    }
};

/**
 * @class pharmacy
 * @brief Represents the pharmacy student as a character.
 */
class pharmacy : public character
{
    private:
        double shield_health;
        int shield_timer;

    public:
    /**
     * @brief Constructs a new pharmacy character with all required sprites.
     * @param start_x The starting X position.
     * @param start_y The starting Y position.
     * @param right_sprite Sprite for facing right.
     * @param left_sprite Sprite for facing left.
     * @param jump_right Sprite for jumping right.
     * @param jump_left Sprite for jumping left.
     * @param crouch_right Sprite for crouching right.
     * @param crouch_left Sprite for crouching left.
     * @param attack_right Sprite for attacking right.
     * @param attack_left Sprite for attacking left.
     * @param active_sprite The initial active sprite.
     */
    pharmacy(double start_x, double start_y,
        sprite right_sprite, sprite left_sprite,
        sprite jump_right, sprite jump_left,
        sprite crouch_right, sprite crouch_left,
        sprite attack_right, sprite attack_left,
        sprite active_sprite) : 
        character(10.0, 5, 100.0, "Pharmacy", start_x, start_y,
        right_sprite, left_sprite,
        jump_right, jump_left,
        crouch_right, crouch_left,
        attack_right, attack_left,
        active_sprite)
    {    
        // Initialise their two unique abilites
        abilities[0] = ability("Immunoshield", "Heal", 25.0, 600); // 10 second cooldown
        abilities[1] = ability("Toxic Dose", "Damage", 20.0, 900); // 15 second cooldown

        shield_health = 0.0;
        shield_timer = 0;
    }

    /**
     * @brief Immunoshield: Shield that blocks the character from incoming damage.
     */
    void immunoshield()
    {
        shield_health = 50.0; // Shield's health
        shield_timer = 600;   // 10 seconds * 60 fps = 600 frames
    }

    /**
     * @brief Retrieves the current health of the immunoshield.
     * @return Shield health as a double.
     */
    double get_shield_health() const override
    {
        return shield_health;
    }

    /**
     * @brief Retrieves the remaining duration of the immunoshield.
     * @return Shield timer in frames.
     */
    int get_shield_timer() const override
    {
        return shield_timer;
    }

    /**
     * @brief Overrides the character update function to handle the shield timer logic.
     */
    void update() override
    {
        // Calling the base class's update first so that gravity and movement still works
        character::update();

        // Pharmacy specific timer logic
        if (shield_timer > 0)
        {
            shield_timer--;
            if (shield_timer <= 0)
            {
                shield_health = 0.0; // Shield expires after timer runs down
            }
        }
    }

    /**
     * @brief Overrides the base take_damage function to block damage using the immunoshield.
     * @param raw_damage The amount of incoming damage.
     * @param attacker_x The X position of the attacker to calculate facing direction.
     */
    void take_damage(double raw_damage, double attacker_x) override
    {
        // Checks if the shield is active
        if (shield_timer > 0 && shield_health > 0)
        {
            // Checks if the pharmacy student facing the attacker
            bool attacker_is_right = (attacker_x > get_x());
            bool facing_correctly = (is_facing_right_direction() == attacker_is_right);

            if (facing_correctly)
            {
                // Block the damage
                if (shield_health >= raw_damage)
                {
                    shield_health -= raw_damage;
                    raw_damage = 0; // Set to 0 so the character itself is not damaged
                }
                else
                {
                    raw_damage = shield_health; // Set to be equivalent to the shield health to block damage

                    shield_health -= raw_damage; 

                    raw_damage = 0;
                }
            }
        }

        character::take_damage(raw_damage, attacker_x);
    }

    /**
     * @brief Toxic Dose ability: Poisons the opponent over time.
     * @param opponent Pointer to the enemy character.
     */
    void toxic_dose(character* opponent)
    {
        // Apply poison for 300 frames (5 seconds)
        // Deal 2 damage every second, resulting in a total of 10 damage
        opponent->apply_poison(300);
    }

    /**
     * @brief Overridden ability for Pharmacy.
     * Slot 0: Immunoshield (Heal) — provides a shield to the pharmacy
     * Slot 1: Toxic Dose (Damage) — poisons the opponent overtime
     */
    void use_ability(int slot_index, character* opponent) override
    {
        ability& active_ability = abilities[slot_index];

        if (active_ability.is_ready())
        {
            if (active_ability.get_name() == "Immunoshield")
            {
                immunoshield();
            }
            else if (active_ability.get_name() == "Toxic Dose")
            {
                toxic_dose(opponent);
            }

            active_ability.start_cooldown();
        }
    }
};

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