#ifndef TECHCARD_H
#define TECHCARD_H

#include <string>
#include <functional>
#include "Card.h"
#include "Deck.h"

class TechCard : public card {

    public:

    enum TechnoType{
        ROBOT,
        MECHANIC,
        SOLDIER,
        TITAN
    };

    using AbilityFunction = std::function<void(IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam)>;

    private:

    std::string unique_ability;
    TechnoType tType;
    int unique_dmg_buff = 0;
    AbilityFunction uniquePower;
    AbilityFunction reversePower;

    public:

    TechCard(int range, DmgType dmgType, TechnoType type, std::string cardName, int cost, int dmgHit, std::string unique_ability, int health, DmgType vulnurability, DmgType immunity, int powerDuration, int powerCost, AbilityFunction power, AbilityFunction reversePower)
    : card(range, dmgType, type, cardName, cost, dmgHit, health, immunity, vulnurability, powerDuration, powerCost){
        this->unique_ability = unique_ability;
        this->tType = type;
        this->uniquePower = power;
        this->reversePower = reversePower;
    }

    void random_unique_dmg();
    int getMaxRandomBonus() override;   // rastgele bonus ust siniri (GUI de bunu okur)
    void hit(IDamageable* target) override;
    void useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) override;
    void useReverseUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) override;
    void activateTypeBuff() override;
    std::string getCardInfo() override;

};

#endif // TECHCARD_H