#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pokemon.h"

// Type effectiveness chart
float type_effectiveness(PokemonType move_type, PokemonType defender_type) {
  // [attacker][defender]
  static float chart[5][5] = {
    // NRM  FIRE  WTR  GRS  ELC
    { 1.0, 1.0, 1.0, 1.0, 1.0 }, // Normal
    { 1.0, 0.5, 0.5, 2.0, 1.0 }, // Fire
    { 1.0, 2.0, 0.5, 0.5, 1.0 }, // Water
    { 1.0, 0.5, 2.0, 0.5, 1.0 }, // Grass
    { 1.0, 1.0, 2.0, 1.0, 0.5 }, // Electric
  };
  return chart[move_type][defender_type];
}

int calculate_damage(Pokemon *attacker, Pokemon *defender, Move *move) {
  float effectiveness = type_effectiveness(move->type, defender->type);
  int damage = (int)(((float)attacker->attack / (float)defender->defense) * move->power * effectiveness);
  if (damage < 1)
    damage = 1;
  return damage;
}

// Pool of 20 pokemon to pick from
static Pokemon pokemon_pool[NUM_POKEMON] = {
  { "Charmander", TYPE_FIRE, 39, 39, 52, 43, 65, { { "Scratch", TYPE_NORMAL, 40 }, { "Ember", TYPE_FIRE, 40 }, { "Flamethrower", TYPE_FIRE, 90 }, { "Slash", TYPE_NORMAL, 70 } } },
  { "Squirtle", TYPE_WATER, 44, 44, 48, 65, 43, { { "Tackle", TYPE_NORMAL, 40 }, { "Water Gun", TYPE_WATER, 40 }, { "Bite", TYPE_NORMAL, 60 }, { "Surf", TYPE_WATER, 90 } } },
  { "Bulbasaur", TYPE_GRASS, 45, 45, 49, 49, 45, { { "Tackle", TYPE_NORMAL, 40 }, { "Vine Whip", TYPE_GRASS, 45 }, { "Razor Leaf", TYPE_GRASS, 55 }, { "Solar Beam", TYPE_GRASS, 120 } } },
  { "Pikachu", TYPE_ELECTRIC, 35, 35, 55, 40, 90, { { "Quick Attack", TYPE_NORMAL, 40 }, { "Thunder Shock", TYPE_ELECTRIC, 40 }, { "Thunderbolt", TYPE_ELECTRIC, 90 }, { "Thunder", TYPE_ELECTRIC, 110 } } },
  { "Geodude", TYPE_NORMAL, 40, 40, 80, 100, 20, { { "Tackle", TYPE_NORMAL, 40 }, { "Rock Throw", TYPE_NORMAL, 50 }, { "Rock Blast", TYPE_NORMAL, 25 }, { "Rollout", TYPE_NORMAL, 30 } } },
  { "Growlithe", TYPE_FIRE, 55, 55, 70, 45, 60, { { "Bite", TYPE_NORMAL, 60 }, { "Ember", TYPE_FIRE, 40 }, { "Flame Wheel", TYPE_FIRE, 60 }, { "Flamethrower", TYPE_FIRE, 90 } } },
  { "Psyduck", TYPE_WATER, 50, 50, 52, 48, 55, { { "Scratch", TYPE_NORMAL, 40 }, { "Water Gun", TYPE_WATER, 40 }, { "Confusion", TYPE_NORMAL, 50 }, { "Surf", TYPE_WATER, 90 } } },
  { "Oddish", TYPE_GRASS, 45, 45, 50, 55, 30, { { "Absorb", TYPE_GRASS, 20 }, { "Mega Drain", TYPE_GRASS, 40 }, { "Razor Leaf", TYPE_GRASS, 55 }, { "Solar Beam", TYPE_GRASS, 120 } } },
  { "Voltorb", TYPE_ELECTRIC, 40, 40, 30, 50, 100, { { "Tackle", TYPE_NORMAL, 40 }, { "Thunder Shock", TYPE_ELECTRIC, 40 }, { "Thunderbolt", TYPE_ELECTRIC, 90 }, { "Thunder", TYPE_ELECTRIC, 110 } } },
  { "Meowth", TYPE_NORMAL, 40, 40, 45, 35, 90, { { "Scratch", TYPE_NORMAL, 40 }, { "Bite", TYPE_NORMAL, 60 }, { "Slash", TYPE_NORMAL, 70 }, { "Fury Swipes", TYPE_NORMAL, 18 } } },
  { "Machop", TYPE_NORMAL, 70, 70, 80, 50, 35, { { "Karate Chop", TYPE_NORMAL, 50 }, { "Low Kick", TYPE_NORMAL, 50 }, { "Seismic Toss", TYPE_NORMAL, 60 }, { "Cross Chop", TYPE_NORMAL, 100 } } },
  { "Gastly", TYPE_NORMAL, 30, 30, 35, 30, 80, { { "Lick", TYPE_NORMAL, 30 }, { "Night Shade", TYPE_NORMAL, 60 }, { "Shadow Ball", TYPE_NORMAL, 80 }, { "Hex", TYPE_NORMAL, 65 } } },
  { "Magmar", TYPE_FIRE, 65, 65, 95, 57, 93, { { "Ember", TYPE_FIRE, 40 }, { "Fire Punch", TYPE_FIRE, 75 }, { "Flamethrower", TYPE_FIRE, 90 }, { "Fire Blast", TYPE_FIRE, 110 } } },
  { "Vaporeon", TYPE_WATER, 130, 130, 65, 60, 65, { { "Water Gun", TYPE_WATER, 40 }, { "Quick Attack", TYPE_NORMAL, 40 }, { "Surf", TYPE_WATER, 90 }, { "Hydro Pump", TYPE_WATER, 110 } } },
  { "Flareon", TYPE_FIRE, 65, 65, 130, 60, 65, { { "Tackle", TYPE_NORMAL, 40 }, { "Ember", TYPE_FIRE, 40 }, { "Fire Spin", TYPE_FIRE, 35 }, { "Flamethrower", TYPE_FIRE, 90 } } },
  { "Jolteon", TYPE_ELECTRIC, 65, 65, 65, 60, 130, { { "Quick Attack", TYPE_NORMAL, 40 }, { "Thunder Shock", TYPE_ELECTRIC, 40 }, { "Pin Missile", TYPE_NORMAL, 25 }, { "Thunderbolt", TYPE_ELECTRIC, 90 } } },
  { "Weepinbell", TYPE_GRASS, 65, 65, 90, 50, 55, { { "Vine Whip", TYPE_GRASS, 45 }, { "Razor Leaf", TYPE_GRASS, 55 }, { "Slam", TYPE_NORMAL, 80 }, { "Solar Beam", TYPE_GRASS, 120 } } },
  { "Poliwag", TYPE_WATER, 40, 40, 50, 40, 90, { { "Bubble", TYPE_WATER, 40 }, { "Water Gun", TYPE_WATER, 40 }, { "Body Slam", TYPE_NORMAL, 85 }, { "Surf", TYPE_WATER, 90 } } },
  { "Ponyta", TYPE_FIRE, 50, 50, 85, 55, 90, { { "Tackle", TYPE_NORMAL, 40 }, { "Ember", TYPE_FIRE, 40 }, { "Flame Wheel", TYPE_FIRE, 60 }, { "Fire Blast", TYPE_FIRE, 110 } } },
  { "Electrode", TYPE_ELECTRIC, 60, 60, 50, 70, 140, { { "Tackle", TYPE_NORMAL, 40 }, { "Thunderbolt", TYPE_ELECTRIC, 90 }, { "Thunder", TYPE_ELECTRIC, 110 }, { "Hyper Beam", TYPE_NORMAL, 150 } } },
};

void assign_random_party(Party *party) {
  static int seeded = 0;
  if (!seeded) {
    srand(time(NULL));
    seeded = 1;
  }

  int chosen[PARTY_SIZE] = { -1, -1, -1, -1 };
  for (int i = 0; i < PARTY_SIZE; i++) {
    int idx;
    int duplicate;
    do {
      duplicate = 0;
      idx = rand() % NUM_POKEMON;
      for (int j = 0; j < i; j++) {
        if (chosen[j] == idx) {
          duplicate = 1;
          break;
        }
      }
    } while (duplicate);
    chosen[i] = idx;
    party->party[i] = pokemon_pool[idx];
  }
  party->active = 0;
}
