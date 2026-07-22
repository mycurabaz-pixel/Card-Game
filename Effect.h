#ifndef EFFECT_H
#define EFFECT_H

// Tek kaynak: hem card hem Character ayni DmgType'i kullanir.
// (Onceden card'in icinde ve Character.h'de ayri ayri tanimliydi -> iki farkli tip olusuyordu.)
enum DmgType{
    ICE,
    FIRE,
    NATURE,
    WIND,
    MAGIC,
    ANTIMAGIC,
    DARKNESS,
    LIGHT,
    SLICE,
    LIGHTNING,
    NORMAL
};

struct Effect
{
    int damage = 2;
    int duration = 2;
    int playedTour = 0;
};

#endif // EFFECT_H
