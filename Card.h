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

    // DmgType artik Effect.h'de global tanimli (tek kaynak).
    // Bu alias sayesinde mevcut 'card::DmgType::FIRE' kullanimlari aynen calismaya devam eder.
    using DmgType = ::DmgType;

    private:

    int health;
    int maxHealth;
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
    bool invulnerable = false;
    bool stunned = false;
    bool reflection_marked = false;
    int powerCost;
    int powerDuration;
    int armor;


    public:

    card(int range, DmgType dmgType, int type, std::string cardName, int cost, int dmgHit, int health, DmgType immunity, DmgType vulnurability, int powerCost, int powerDuration){
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
        maxHealth = health;
        this->powerCost = powerCost;
        this->powerDuration = powerDuration;
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

    void setDmgHit(int dmgHit){
        this->dmgHit = dmgHit;
    }

    void takeDamage(IDamageable* attacker, int damage) override{
                if(invulnerable && damage > 0) return;

                if(maxHealth >= health - damage){
                    if(damage > armor){damage -= armor; armor = 0;}
                    else if(damage > 0){armor -= damage; return;}
                    health -= damage;
                }
                    else if((health - damage) % maxHealth > damage){
                health = maxHealth;
                }
                if(attacker && damage > 0 && isReflectionMarked()){ reflect(attacker, damage); }
            }

    int getHealth() override {
        return health;
    }

    void setHealth(int health) override {
        if(maxHealth>health){
             this->health = health;
        }
        else{
            health = maxHealth;
        }
    }

    void setMaxHealth(int maxHealth){
        this->maxHealth = maxHealth;
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

    int getAbilityCost(){return powerCost;}
    void setInvulnerable(bool invulnerable){this->invulnerable = invulnerable;} // [DUZELTME] == degil =
    bool isInvulnerable(){return invulnerable;}
    void setStunned(bool stunned){this->stunned = stunned;}
    bool isStunned(){return stunned;}
    bool isReflectionMarked(){return reflection_marked;}
    void setReflectionMarked(bool reflection_marked){this->reflection_marked = reflection_marked;}
    void reflect(IDamageable* target, int damage){
        if(target->isReflectionMarked()){
            return;
    }
    target->takeDamage(this, damage/3);
}

    void addEffect(DmgType hitType, Effect effect){

        if(hitType == NORMAL){return;}
        
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

    void addArmor(int armor){
        this->armor += armor;
    }

    int getArmor() override{
        return armor;
    }

    virtual ~card();

    virtual void hit(IDamageable* target) = 0;

    virtual void useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) = 0;

    virtual void activateTypeBuff() = 0;

    virtual std::string getCardInfo() = 0;

    virtual  void useReverseUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) = 0;

};

#endif // CARD_H
