#ifndef CARD_H
#define CARD_H

#include <string>
#include <map>
#include "IDamageable.h"
#include "Effect.h"
#include "Deck.h"

// card sadece Player* tuttuğu için ileri bildirim yeterli (Player.h'yi include etmiyoruz -> döngü yok)
class Player;

class card : public IDamageable {

    public:

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
        NORMAL
    };

    private:

    int health;
    inline static int id_counter = 1;
    int private_card_id;
    int private_deck_id;
    int range;
    DmgType dmgType;
    int type;
    std::string cardName;
    int cost;
    int dmgHit;
    std::map<DmgType, Effect> effects;
    Effect hitEffect;
    DmgType immunity;
    DmgType vulnurability;
    Player* owner = nullptr; // [DEGISIKLIK] owner artik nullptr ile basliyor (onceden ilklenmiyordu)

    public:

    card(int range, DmgType dmgType, int type, std::string cardName, int cost, int dmgHit, int health, DmgType immunity, DmgType vulnurability){
        this->range = range;
        this->dmgType = dmgType;
        this->type = type;
        this->cardName = cardName;
        this->cost = cost;
        this->dmgHit = dmgHit;
        private_card_id = id_counter;
        id_counter++;
        this->health = health;
        this->immunity = immunity;
        this->vulnurability = vulnurability;
    }

    DmgType getDamageType(){
        return dmgType;
    }

    void setDamageType(DmgType dmgType){
        this->dmgType = dmgType;
    }

    int getPrivateCardId(){
        return private_card_id;
    }

    int getType(){
        return type;
    }

    int getCost(){
        return cost;
    }

    void updateDmgHit(int dmgHit){
        this->dmgHit = dmgHit;
    }

    int getDmgHit(){
        return dmgHit;
    }

    void takeDamage(int dmg) override {
        health -= dmg;
    }

    int getHealth() override {
        return health;
    }

    void setHealth(int health) override {
        this->health = health;
    }

    std::string getName() override {
        return cardName;
    }

    Effect getHitEffect(){
        return hitEffect;
    }

    void setOwner(Player* owner){
        this->owner = owner;
    }

    Player* getOwner(){
        return owner;
    }

    void addEffect(DmgType hitType, Effect effect){
        if(hitType == vulnurability){
            effect.damage *= 2;
        }
        if(hitType != immunity){
            if(auto it = effects.find(hitType); it != effects.end()){
                effects.at(hitType).duration += effect.duration;
            }
            else{
                effects.emplace(hitType, effect);
            }
        }
    }

    std::map<DmgType, Effect>& getEffects(){
        return effects;
    }

    virtual ~card();

    virtual void hit(IDamageable* target) = 0;

    virtual void useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) = 0;

    virtual void activateTypeBuff() = 0;

    virtual std::string getCardInfo() = 0;

};

#endif // CARD_H
