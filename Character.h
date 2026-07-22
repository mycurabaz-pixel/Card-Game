#ifndef CHARACTER_H
#define CHARACTER_H

#include"Effect.h"
#include <string>
#include <functional>
#include <utility>   // std::move
#include <algorithm> // std::min, std::max (baslangic manasi kirpmasi icin)
#include "IDamageable.h"
#include "Deck.h"
#include "Card.h"

// DmgType artik Effect.h'den geliyor (yukarida include edildi) -> tek tanim.

// NOT: Bu sinif su an hicbir yerde kullanilmiyor (main akisi hala card tabanli calisiyor).
// Icinde birkac hata var (asagida mesajda anlattim) ama senin verini bozmamak icin oldugu gibi biraktim.
class Character : public IDamageable {
    using Ability = std::function<void ( IDamageable* target,
        Deck<card*>& battlefield,
        Deck<card*>& allyTeam,
        Deck<card*>& enemyTeam)>;
        std::map<DmgType, Effect> effects;


        private:

        std::string name;
        int health, maxHealth, armor = 0, mana, maxMana, abilityCoolDown, currentCooldown = 0, abilityDuration, abilityCost;
        Ability ability, reverseAbility;
        bool invulnerable = false;
        bool reflection_marked = false;
        bool stunned;
        Player* owner;

        public:

        Character(std::string name, int maxHealth, int maxMana, int abilityCooldown, int abilityDuration, int abilityCost, Ability ability, Ability reverseAbility):
            name(std::move(name)),
            maxHealth(maxHealth),
            health(maxHealth),
            maxMana(maxMana),
            // Baslangic manasi: maxMana/3, taban 4 (min). maxMana 4'ten kucukse tavani asmasin diye min ile kirpiliyor.
            mana(std::min(maxMana, std::max(maxMana / 3, 4))),
            abilityCoolDown(abilityCoolDown),
            abilityDuration(abilityDuration),
            abilityCost(abilityCost),
            ability(std::move(ability)),
            reverseAbility(std::move(reverseAbility)){}

            bool tryUseAbility(IDamageable* target, Deck<card*> battlefield, Deck<card*> allyTeam, Deck<card*> enemyTeam){
            if(currentCooldown > 0) return false;         // hazır değil
            if(!canAfford(abilityCost)) return false;     // [YENI] mana yetmiyorsa yetenek kullanilamaz
            if(ability) ability(target, battlefield, allyTeam, enemyTeam);
            currentCooldown = abilityCoolDown;            // beklemeyi başlat
            useMana(abilityCost);
            return true;
            }

            void revert(IDamageable* target, Deck<card*> battlefield, Deck<card*> allyTeam, Deck<card*> enemyTeam){
        if(reverseAbility) reverseAbility(target, battlefield, allyTeam, enemyTeam);
    }

            int getHealth(){
                return health;
            }

            void setHealth(int health){
                this->health = health;
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

            std::string getName(){
                return name;
            }

            void setMana(int mana){if(mana > maxMana){
                this->mana = maxMana;}
            else if(mana < 0){          // [DUZELTME] alt sinir yoktu -> mana eksiye dusuyordu
                this->mana = 0;}
            else{
                this->mana = mana;
            }}
            void useMana(int mana){
                setMana(this->mana - mana);   // [DUZELTME] setMana uzerinden -> 0'in altina inmez
            }
            int getMana(){return mana;}

            // [YENI] Yeterli mana var mi? GUI de bunu kullanip oynanamayan karti soluk gosterir.
            bool canAfford(int cost) const { return mana >= cost; }

            bool isAbilityReady(){return currentCooldown == 0;}
            void tickCoolDown(){currentCooldown --;}

            int getcurrentCooldown(){return currentCooldown;}
            int getAbilityDuration(){return abilityDuration;}

            void setInvulnerable(bool invulnerable){this->invulnerable = invulnerable;}
            bool isInvulnerable(){return invulnerable;}

            bool isReflectionMarked(){return reflection_marked;}
            void setReflectionMarked(bool reflection_marked){this->reflection_marked = reflection_marked;}
            void reflect(IDamageable* target, int damage){
                if(target->isReflectionMarked()){
                    return;
                }
                target->takeDamage(this, damage/3);
            }

            void setStunned(bool stunned){this->stunned = stunned;}
            bool isStunned(){return stunned;}

            Player* getOwner(){return owner;}
            void setOwner(Player* owner){this->owner = owner;}


        void addEffect(DmgType hitType, Effect effect){

        if(hitType == NORMAL){return;}

        effect.damage *= 2;
        if(auto it = effects.find(hitType); it != effects.end()){
        effects.at(hitType).duration += effect.duration;
    }
        else{
            effects.emplace(hitType, effect);
        }
    }

    std::map<DmgType, Effect>& getEffects(){
        return effects;
    }

    void addArmor(int armor){
        this->armor += armor;
    }

    int getArmor() override {
        return armor;
    }
};

#endif // CHARACTER_H
