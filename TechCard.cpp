#include "TechCard.h"
#include <iostream>
#include <random>
#include <sstream>

void TechCard::random_unique_dmg(){
    std::random_device rd;

    std::mt19937 gen(rd());

    std::uniform_int_distribution<> interval_1(0, (card::getCost() * 2));

    unique_dmg_buff = interval_1(gen);

    card::updateDmgHit(card::getDmgHit() + unique_dmg_buff);
}

void TechCard::hit(IDamageable* target) {
    random_unique_dmg();

    target->takeDamage(card::getDmgHit());

    if(unique_dmg_buff > card::getCost()/2){
        std::cout << "ENEMY HAS BEEN CURSED" << std::endl;

        switch(card::getDamageType()){

            case ICE:

            std::cout << "düşman donduruldu" << std::endl;
            break;

            case FIRE:

            std::cout << "düşman yakıldı" << std::endl;
            target->takeDamage(2);
            break;

            case NATURE:

            std::cout << "ENEMY HAS BEEN ROOTED" << std::endl;
            break;

            case WIND:

            std::cout << "ENEMY HAS BEEN SLICED WİTH WİND" << std::endl;
            target->takeDamage(card::getDmgHit());

            break;

            case MAGIC:

            std::cout << "YOU HİT EXTRA MAGIC DAMAGE AND GAINED 2 MANA" << std::endl;
            target->takeDamage(card::getDmgHit());
            break;

            case ANTIMAGIC:

            std::cout << "YOU HİT EXTRA ANTIMAGIC DAMAGE AND WASTED ENEMY'S 2 MANA" << std::endl;
            target->takeDamage(card::getDmgHit());
            break;

            case DARKNESS:

            std::cout << "ENEMY BECAME FEARED" << std::endl;
            break;

            case LIGHT:

            std::cout << "ENEMY BECAME BLİND" << std::endl;
            break;

            case SLICE:

            std::cout << "ENEMY STARTED BLEEDİNG " << std::endl;
            target->takeDamage(2);
            break;

            case NORMAL:

            break;
        }
    }
    card::updateDmgHit(card::getDmgHit() - unique_dmg_buff);
}

void TechCard::useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) {
    if(uniquePower){
        uniquePower(target, battlefield, this, allyTeam, enemyTeam);
    }
}

void TechCard::activateTypeBuff() {
    std::cout << "Every tech card on the table gets high voltage extra " << card::getCost()/2 << "extra electrical damage" << std::endl;
}

std::string TechCard::getCardInfo() {
    std::ostringstream info;
    info << "\n=== [" << getName() << "] CARD INFO ===\n";
    info << "Category       : Tech Card\n";
    info << "Card ID        : " << getPrivateCardId() << "\n";
    info << "Energy Cost    : " << getCost() << "\n";
    info << "Health (Armor) : " << getHealth() << "\n";
    info << "Damage Output  : " << getDmgHit() << " (+ Buff: " << unique_dmg_buff << ")\n";

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

    info << "Model Type     : ";
    switch(tType) {
        case ROBOT: info << "ROBOT"; break;
        case MECHANIC: info << "MECHANIC"; break;
        case SOLDIER: info << "CYBER SOLDIER"; break;
        case TITAN: info << "TITAN"; break;
    }
    info << "\n";

    info << "Hardware       : " << unique_ability << " (Effect Power: " << unique_ability_effect_power << ")\n";
    info << "======================================\n";

    return info.str();
}
