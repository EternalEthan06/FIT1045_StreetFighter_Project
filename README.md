# Ethan's Street Fighter

A 2D fighting game developed in C++ using the [SplashKit](https://splashkit.io/) framework for FIT1045 Introduction to Programming.

## Features

* **Graphical Menu System**: A fully interactive, mouse-driven GUI for selecting game modes, character rosters, and AI difficulty levels.
* **Multiple Game Modes**:
  * **Singleplayer**: Fight against an advanced Machine Learning AI.
  * **Multiplayer**: Classic local 1v1 combat on the same keyboard.
* **Unique Roster**:
  * **Computer Science Student**: Uses abilities like *Corrupted Combo* (Buffs attack power randomly) and *Glitch Protocol* (Takes over the enemy's controls for 3 seconds).
  * **Pharmacy Student**: Uses abilities like *Immunoshield* (Blocks incoming damage) and *Toxic Dose* (Poisons the enemy over time).
* **Save & Resume System**: Need a break? Press `Escape` during a match to save your exact position, health, and stamina, and seamlessly resume from the main menu later!

## The AI Architecture

This game features a custom built **Hybrid AI Architecture** designed to study and emulate a real human player:

1. **Imitation Learning (1-Nearest Neighbor)**: During combat, the game records the human's actions in the background, mapping a continuous 3D state space (Distance, AI Stamina, Player Stamina) to discrete choices. To make a decision, the AI calculates the Euclidean distance against every memory it has ever seen to find the most mathematically similar scenario, and executes the exact same action the human did!
2. **Decision Tree Fallback**: A safety-net system that temporarily overrides the ML model during critical edge cases (e.g., aggressively pushing when the round timer is extremely low, or defensively retreating when health drops below 30%).

## Controls

### Player 1
* **Movement**: `A` / `D`
* **Jump/Crouch**: `W` / `S`
* **Basic Attack**: `Spacebar`
* **Ability 1**: `E`
* **Ability 2**: `R`

### Player 2
* **Movement**: `Left Arrow` / `Right Arrow`
* **Jump/Crouch**: `Up Arrow` / `Down Arrow`
* **Basic Attack**: `Right Ctrl`
* **Ability 1**: `Right Shift`
* **Ability 2**: `Right Alt`

## Setup & Running

1. Ensure you have the **SplashKit** framework installed on your system.
2. Navigate to this directory in your terminal.
3. Compile the game by running: `skm clang++ *.cpp -o fighter`
4. Execute the game to begin playing!
