#ifndef _POKEMON_H_
#define _POKEMON_H_

#include <stdint.h>

#define NUM_POKEMON 20
#define PARTY_SIZE 4
#define NUM_MOVES 4

typedef enum {
    TYPE_NORMAL = 0,
    TYPE_FIRE,
    TYPE_WATER,
    TYPE_GRASS,
    TYPE_ELECTRIC
} PokemonType;

typedef struct {
    char *name;
    PokemonType type;
    int power;
} Move;


typedef struct {
    char *name;
    PokemonType type;
    int max_hp;
    int hp;
    int attack;
    int defense;
    int speed;
    Move moves[NUM_MOVES];
} Pokemon;

typedef struct {
    Pokemon party[PARTY_SIZE];
    int active;  // index of active pokemon
} Party;

float type_effectiveness(PokemonType move_type, PokemonType defender_type);
int calculate_damage(Pokemon *attacker, Pokemon *defender, Move *move);
void assign_random_party(Party *party);

#endif

