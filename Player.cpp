#include "Player.h"
#include "GameManager.h" // gameManager::getInstance() burada cozuluyor (kod sirasi hatasinin asil sebebi)
#include <iostream>

void Player::drawCard(){
    if(!yourDeck.isEmpty()){
        card* drawnCard = yourDeck.getTopCard();

        addToHand(drawnCard);

        yourDeck.removeTopCard();

        std::cout << player_name << "has drawn a card." << std::endl;
    }
    else{
        std::cout << "Deck is empty, cant drawn a card!" << std::endl;
    }
}

bool Player::playCard(card* played_card, Deck<card*>& battlefield){
    // [YENI] Karakter yoksa mana dusurulurken cokerdi.
    if(activeCharacter == nullptr){
        std::cout << player_name << ": no character selected, cannot play a card." << std::endl;
        return false;
    }
    // [YENI] Mana yetmiyorsa kart OYNANMAZ (onceden mana eksiye dusuyordu).
    if(!activeCharacter->canAfford(played_card->getCost())){
        std::cout << player_name << ": not enough mana ("
                  << activeCharacter->getMana() << "/" << played_card->getCost() << ")" << std::endl;
        return false;
    }

    for(auto it = hand.begin(); it != hand.end(); ++it){
        if((*it)->getPrivateCardId() == played_card->getPrivateCardId()){

            activeCharacter->useMana(played_card->getCost());
            battlefield.addCardToDeck(played_card);
            team.addCardToDeck(played_card);
            hand.erase(it);
            return true;
        }
    }
    return false;   // kart elde bulunamadi
}

card* Player::selectCard(){
    int i = 0;
    int selection;
    for(card* c: hand){
        std::cout << i++ << ".card " << c->getName() << std::endl;
    }
    std::cout << "seçiminiz:" << std::endl;
    std::cin >> selection;
    return hand.at(selection);
}

void Player::hitTarget(IDamageable* target, card* usedCard){
    if(actionPoint == 0){return;}
    usedCard->hit(target);
    actionPoint--;
    gameManager::getInstance().checkTargetHealth(nullptr);
}

void Player::useCardUniqueAbility(IDamageable* target, Deck<card*>& battlefield, card* used_card,
    Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
    if(actionPoint == 0){return;}

    // [YENI] Hedef tarafi kontrolu. Onceden C++ tarafinda HIC kontrol yoktu; kural sadece
    // arayuzde duruyordu. Artik kartin kendi targetSide'i belirliyor:
    //   ENEMY (varsayilan) -> kendi takimina kullanilamaz  (hasar veren kartlar)
    //   ALLY               -> rakip takima kullanilamaz    (heal / armor / koruma kartlari)
    //   NONE               -> hedefsiz (AoE); target zaten nullptr gelir, kontrol atlanir
    if(target && used_card->getTargetSide() != TargetSide::NONE){
        const bool sameSide = (used_card->getOwner() == target->getOwner());
        if(used_card->getTargetSide() == TargetSide::ENEMY && sameSide){
            std::cout << used_card->getName() << ": this ability can only target the ENEMY team." << std::endl;
            return;
        }
        if(used_card->getTargetSide() == TargetSide::ALLY && !sameSide){
            std::cout << used_card->getName() << ": this ability can only target your OWN team." << std::endl;
            return;
        }
    }

    used_card->useUniquePower(target, battlefield, used_card, allyTeam, enemyTeam);

    activeCharacter->useMana(used_card->getAbilityCost());
    actionPoint--;

    gameManager::getInstance().checkTargetHealth(nullptr);
}

void Player::useCardReverseAbility(IDamageable* target, Deck<card*>& battlefield, card* used_card,
    Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
        used_card->useReverseUniquePower(target, battlefield, used_card, allyTeam, enemyTeam);

        gameManager::getInstance().checkTargetHealth(nullptr);
}
