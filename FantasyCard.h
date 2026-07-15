#ifndef FANTASYCARD_H
#define FANTASYCARD_H

#include <string>
#include <functional>
#include "Card.h"
#include "Deck.h"

class fantasyCard : public card {

    public:

    enum FantasyType{
        DRAGON,
        SOLDIER,
        MAGICIAN,
        HEALER
    };

    using AbilityFunction = std::function<void(IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam)>;

    private:

    std::string unique_ability;
    int unique_ability_effect_power;
    FantasyType fType;
    int unique_dmg_buff = 0;
    AbilityFunction uniquePower;

    public:

    fantasyCard(int range, DmgType dmgType, FantasyType type, std::string cardName, int cost, int dmgHit, std::string unique_ability, int unique_ability_effect_power, int health, DmgType vulnurability, DmgType immunity, AbilityFunction power)
    : card(range, dmgType, type, cardName, cost, dmgHit, health, immunity, vulnurability){
        this->unique_ability = unique_ability;
        this->unique_ability_effect_power = unique_ability_effect_power;
        this->fType = type;
        this->uniquePower = power;
    }

    void random_unique_dmg();
    void hit(IDamageable* target) override;
    void useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) override;
    void activateTypeBuff() override;
    std::string getCardInfo() override;

};

#endif // FANTASYCARD_H
