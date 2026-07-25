#include "SeaSonCard.h"
#include <iostream>
#include <random>
#include <sstream>

void SeaSonCard::random_unique_dmg(){
    std::random_device rd;

    std::mt19937 gen(rd());

    std::uniform_int_distribution<> interval_1(0, getMaxRandomBonus());

    unique_dmg_buff = interval_1(gen);

    card::updateDmgHit(card::getDmgHit() + unique_dmg_buff);
}

// Rastgele bonusun ust siniri - TEK KAYNAK (zar + GUI ayni yeri okur).
// SENIN DEGISIKLIGIN: cost*2 degil, cost.
int SeaSonCard::getMaxRandomBonus(){
    return card::getCost();
}

void SeaSonCard::hit(IDamageable* target) {

    if(getOwner() == target->getOwner()){return;}

    if(!card::isStunned() && isActive()){
    setActive(false);
    random_unique_dmg();

    target->takeDamage(this, card::getDmgHit());

    if(unique_dmg_buff > card::getCost()/2){
        std::cout << "ENEMY HAS BEEN CURSED" << std::endl;

        if(card* tempTarget = dynamic_cast<card*>(target)){
            tempTarget->addEffect(getHitEffect());
        }
    }
    card::updateDmgHit(card::getDmgHit() - unique_dmg_buff);
    }
}

void SeaSonCard::useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) {

    if(uniquePower && !card::isStunned() && isActive()){
        setActive(false);
        uniquePower(target, battlefield, this, allyTeam, enemyTeam);
    }
}

void SeaSonCard::useReverseUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self,
                                        Deck<card*> allyTeam, Deck<card*> enemyTeam){
    if(reversePower && !card::isStunned()){
        reversePower(target, battlefield, this, allyTeam, enemyTeam);
    }
}

void SeaSonCard::activateTypeBuff() {
    std::cout << "Every sea son card on the table gets extra " << card::getCost()/2 << "damage to marked enemies" << std::endl;
}

std::string SeaSonCard::getCardInfo() {
    std::ostringstream info;
    info << "\n=== [" << getName() << "] CARD INFO ===\n";
    info << "Category       : Sea Son Card\n";
    info << "Card ID        : " << getPrivateCardId() << "\n";
    info << "Silver Cost    : " << getCost() << "\n";
    info << "Durability     : " << getHealth() << "\n";
    info << "Attack Power   : " << getDmgHit() << " (+ Buff: " << unique_dmg_buff << ")\n";

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

    info << "Crew Type      : ";
    switch(sType) {
        case KRAKEN: info << "KRAKEN"; break;
        case MEDIC: info << "MEDIC"; break;
        case PIRATE: info << "PIRATE"; break;
        case ORACLE: info << "ORACLE"; break;
    }
    info << "\n";

    info << "Sea Curse      : " << unique_ability << "\n";
    info << "======================================\n";

    return info.str();
}
