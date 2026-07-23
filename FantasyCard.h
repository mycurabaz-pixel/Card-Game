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
    FantasyType fType;
    int unique_dmg_buff = 0;
    AbilityFunction uniquePower;
    AbilityFunction reversePower;


    public:

    fantasyCard(int range, DmgType dmgType, FantasyType type, std::string cardName, int cost, int dmgHit, std::string unique_ability, int health, DmgType vulnurability, DmgType immunity, int powerDuration, int powerCost, AbilityFunction power, AbilityFunction reversePower)
    : card(range, dmgType, type, cardName, cost, dmgHit, health, immunity, vulnurability, powerCost, powerDuration){
        this->unique_ability = unique_ability;
        this->fType = type;
        this->uniquePower = power;
    }

    void random_unique_dmg();
    int getMaxRandomBonus() override;   // rastgele bonus ust siniri (GUI de bunu okur)
    void hit(IDamageable* target) override;
    void activateTypeBuff() override;
    void useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) override;
    void useReverseUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) override;
    std::string getCardInfo() override;

};

#endif // FANTASYCARD_H
