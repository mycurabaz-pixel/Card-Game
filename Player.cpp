#include "Player.h"
#include "GameManager.h" // gameManager::getInstance() burada cozuluyor (kod sirasi hatasinin asil sebebi)
#include <iostream>

void Player::drawCard(){
    if(!yourDeck.isEmpty()){
        card* drawnCard = yourDeck.getTopCard();

        hand.push_back(drawnCard);

        yourDeck.removeTopCard();

        std::cout << player_name << "has drawn a card." << std::endl;
    }
    else{
        std::cout << "Deck is empty, cant drawn a card!" << std::endl;
    }
}

void Player::playCard(card* played_card, Deck<card*>& battlefield){
    for(auto it = hand.begin(); it != hand.end(); ++it){
        if((*it)->getPrivateCardId() == played_card->getPrivateCardId()){

            activeCharacter->setMana(activeCharacter->getMana() - played_card->getCost());
            battlefield.addCardToDeck(played_card);
            team.addCardToDeck(played_card);
            hand.erase(it);
            break;
        }
    }
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
    usedCard->hit(target);
    gameManager::getInstance().checkTargetHealth(nullptr);
}

void Player::useCardUniqueAbility(IDamageable* target, Deck<card*>& battlefield, card* used_card,
                                  Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
    used_card->useUniquePower(target, battlefield, used_card, allyTeam, enemyTeam);

    gameManager::getInstance().checkTargetHealth(nullptr);
}
