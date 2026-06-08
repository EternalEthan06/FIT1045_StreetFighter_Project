# pragma once
# include "splashkit.h"

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