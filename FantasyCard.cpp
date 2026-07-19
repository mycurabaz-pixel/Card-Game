#include "FantasyCard.h"
#include <iostream>
#include <random>
#include <sstream>

void fantasyCard::random_unique_dmg(){
    std::random_device rd;

    std::mt19937 gen(rd());

    std::uniform_int_distribution<> interval_1(0, (card::getCost() * 2));

    unique_dmg_buff = interval_1(gen);

    card::updateDmgHit(card::getDmgHit() + unique_dmg_buff);
}

void fantasyCard::hit(IDamageable* target) {
    if(!card::isStunned()){
        random_unique_dmg();

    target->takeDamage(this, card::getDmgHit());

    std::cout << card::getName() << ", " << target->getName() << " hedefine " << card::getDmgHit() << " vurdu!" << std::endl;

    if(unique_dmg_buff > card::getCost()/2){
        std::cout << "ENEMY HAS BEEN CURSED" << std::endl;
        if(card* tempTarget = dynamic_cast<card*>(target)){
            tempTarget->addEffect(getDamageType(), getHitEffect());
        }
    }
    card::updateDmgHit(card::getDmgHit() - unique_dmg_buff);
    }
}

void fantasyCard::useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) {
    if(uniquePower && !card::isStunned()){
        uniquePower(target, battlefield, this, allyTeam, enemyTeam);
    }
}

void fantasyCard::activateTypeBuff() {
    std::cout << "healing every fantasy card on the table and giving extra " << card::getCost() << "damage" << std::endl;
}

std::string fantasyCard::getCardInfo() {
    std::ostringstream info;
    info << "\n=== [" << getName() << "] CARD INFO ===\n";
    info << "Category       : Fantasy Card\n";
    info << "Card ID        : " << getPrivateCardId() << "\n";
    info << "Mana Cost      : " << getCost() << "\n";
    info << "Health         : " << getHealth() << "\n";
    info << "Damage         : " << getDmgHit() << " (+ Buff: " << unique_dmg_buff << ")\n";

    info << "Damage Type    : ";
    switch(getDamageType()) {
        case ICE: info << "(ICE)"; break;
        case FIRE: info << "(FIRE)"; break;
        case NATURE: info << "(NATURE)"; break;
        case WIND: info << "(WIND)"; break;
        case MAGIC: info << "(MAGIC)"; break;
        case ANTIMAGIC: info << "(ANTIMAGIC)"; break;
        case DARKNESS: info << "(DARKNESS)"; break;
        case LIGHT: info << "(LIGHT)"; break;
        case SLICE: info << "(SLICE)"; break;
        case NORMAL: info << "(NORMAL)"; break;
        default: info << "UNKNOWN"; break;
    }
    info << "\n";

    info << "Class          : ";
    switch(fType) {
        case DRAGON: info << "DRAGON"; break;
        case SOLDIER: info << "SOLDIER"; break;
        case MAGICIAN: info << "MAGICIAN"; break;
        case HEALER: info << "HEALER"; break;
    }
    info << "\n";

    info << "Special Ability: " << unique_ability << " (Power: " << unique_ability_effect_power << ")\n";
    info << "======================================\n";

    return info.str();
}
