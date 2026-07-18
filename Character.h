#ifndef CHARACTER_H
#define CHARACTER_H

#include <string>
#include <functional>
#include <utility> // std::move
#include "IDamageable.h"
#include "Deck.h"
#include "Card.h"

// NOT: Bu sinif su an hicbir yerde kullanilmiyor (main akisi hala card tabanli calisiyor).
// Icinde birkac hata var (asagida mesajda anlattim) ama senin verini bozmamak icin oldugu gibi biraktim.
class Character : public IDamageable {
    using Ability = std::function<void ( IDamageable* target,
        Deck<card*>& battlefield,
        Deck<card*>& allyTeam,
        Deck<card*>& enemyTeam)>;

        private:

        std::string name;
        int health, maxHealth, mana, maxMana, abilityCoolDown, currentCooldown = 0, abilityDuration, abilityCost;
        Ability ability, reverseAbility;
        bool invulnerable = false;

        public:

        Character(std::string name, int maxHealth, int maxMana, int abilityCooldown, int abilityDuration, int abilityCost, Ability ability, Ability reverseAbility):
            name(std::move(name)),
            maxHealth(maxHealth),
            health(maxHealth),
            maxMana(maxMana),
            mana(maxMana),
            abilityCoolDown(abilityCoolDown),
            abilityDuration(abilityDuration),
            abilityCost(abilityCost),
            ability(std::move(ability)),
            reverseAbility(std::move(reverseAbility)){}

            bool tryUseAbility(IDamageable* target, Deck<card*> battlefield, Deck<card*> allyTeam, Deck<card*> enemyTeam){
            if(currentCooldown > 0) return false;         // hazır değil
            if(ability) ability(target, battlefield, allyTeam, enemyTeam);
            currentCooldown = abilityCoolDown;            // beklemeyi başlat
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

            void takeDamage(int damage) override{
                if(invulnerable && damage > 0) return; 
                
                if(maxHealth >= health - damage){
                    health -= damage;
                }
                    else if((health - damage) % maxHealth > damage){
                health = maxHealth;
                }
            }

            std::string getName(){
                return name;
            }

            void setMana(int mana){if(mana > maxMana){
                this->mana = maxMana;}
            else{
                this->mana = mana;
            }}
            int getMana(){return mana;}
            bool isAbilityReady(){return currentCooldown == 0;}
            void tickCoolDown(){currentCooldown --;}
            int getcurrentCooldown(){return currentCooldown;}
            int getAbilityDuration(){return abilityDuration;}
            void setInvulnerable(bool invulnerable){this->invulnerable = invulnerable;}
            bool isInvulnerable(){return invulnerable;}
};

#endif // CHARACTER_H
