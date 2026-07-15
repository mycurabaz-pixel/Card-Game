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
class Character : IDamageable {
    using Ability = std::function<void ( IDamageable* target,
        Deck<card*>& battlefield,
        Deck<card*>& allyTeam,
        Deck<card*>& enemyTeam)>;

        private:

        std::string name;
        int health, maxHealth, mana, maxMana, abilityCoolDown, abilityDuration, abilityCost;
        Ability ability;

        Character(std::string name, int Maxhealth, int maxMana, int abilityCooldown, int abilityDuration, int abilityCost):
            name(std::move(name)),
            maxHealth(std::move(maxHealth)),
            health(std::move(maxHealth)),
            maxMana(std::move(maxMana)),
            mana(std::move(maxMana)),
            abilityCoolDown(std::move(abilityCoolDown)),
            abilityDuration(std::move(abilityDuration)),
            abilityCost(std::move(abilityCost)) {}

            void useAbility(IDamageable* target, Deck<card*> battlefield, Deck<card*> allyTeam, Deck<card*> enemyTeam){
                if(ability) ability(target, battlefield, allyTeam, enemyTeam);
            }

            int getHealth(){
                return health;
            }

            void setHealth(int health){
                this->health = health;
            }

            void takeDamage(int damage) override{
                health -=  damage; 
            }

            std::string getName(){
                return name;
            }
};

#endif // CHARACTER_H
