# include "character.h"
# include "ability.h"

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