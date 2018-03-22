/*enum {
    SPELL_FIRE,
    SPELL_ICE,
    SPELL_LIGHTNING,
    SPELL_WIND,
    MAX_SPELL
};
//not sure if the player file is the best place for this.
*/

struct Player {
    v2 pos,
       vel;

    i8 currSpell;

    r32 health,
        mana;
};
