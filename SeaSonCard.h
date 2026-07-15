#ifndef SEASONCARD_H
#define SEASONCARD_H

#include <string>
#include <functional>
#include "Card.h"
#include "Deck.h"

class SeaSonCard : public card {

    public:

    enum SeaSonType{
        KRAKEN,
        MEDIC,
        PIRATE,
        ORACLE
    };

    using AbilityFunction = std::function<void(IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam)>;

    private:

    std::string unique_ability;
    int unique_ability_effect_power;
    SeaSonType sType;
    int unique_dmg_buff = 0;
    AbilityFunction uniquePower;

    public:

    SeaSonCard(int range, DmgType dmgType, SeaSonType type, std::string cardName, int cost, int dmgHit, std::string unique_ability, int unique_ability_effect_power, int health, DmgType vulnurability, DmgType immunity, AbilityFunction power)
    : card(range, dmgType, type, cardName, cost, dmgHit, health, immunity, vulnurability){
        this->unique_ability = unique_ability;
        this->unique_ability_effect_power = unique_ability_effect_power;
        this->sType = type;
        this->uniquePower = power;
    }

    void random_unique_dmg();
    void hit(IDamageable* target) override;
    void useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) override;
    void activateTypeBuff() override;
    std::string getCardInfo() override;

};

#endif // SEASONCARD_H
