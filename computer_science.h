# include "character.h"
# include "ability.h"

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
