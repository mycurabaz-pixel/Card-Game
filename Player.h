#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <vector>
#include<stack>
#include "IDamageable.h"
#include "Deck.h"
#include "Card.h"
#include "Character.h"

class Player {

    private:

    std::string player_name;
    int health = 1000;
    inline static int playerId = 1;
    Deck<card*> yourDeck;
    std::vector<card*> hand;
    Deck<card*> team;
    Character* activeCharacter = nullptr;
    int actionPoint = 3, abilityPoint = 1;
    std::stack<card*> graveyard;
    

    public:

    Player(std::string name) : player_name(name) {}


    std::string getName() {
        return player_name;
    }

    void addCardToPlayerDeck(card* c){
        yourDeck.addCardToDeck(c);
    }

    std::vector<card*> getHand(){
        return hand;
    }

    // Govdesi cout/cin veya gameManager kullandigi icin tanimlar Player.cpp'de:
    void drawCard();
    bool playCard(card* played_card, Deck<card*>& battlefield);   // [DEGISIKLIK] void -> bool (mana yetmezse false)
    card* selectCard();
    void hitTarget(IDamageable* target, card* usedCard);
    void useCardUniqueAbility(IDamageable* target, Deck<card*>& battlefield, card* used_card,
                              Deck<card*>& allyTeam, Deck<card*>& enemyTeam);

    void useCardReverseAbility(IDamageable* target, Deck<card*>& battlefield, card* used_card,
                              Deck<card*>& allyTeam, Deck<card*>& enemyTeam);

    void shuffleDeck(){
        yourDeck.shuffle();
    }

    void addToTeam(card* c) { team.addCardToDeck(c); }
    Deck<card*>& getTeam() { return team; }

    // [GUI ICIN EKLENDI] GUI elindeki kartlari cizebilmek icin okumali.
    const std::vector<card*>& getHand() const { return hand; }

    // [GUI ICIN EKLENDI] Yandaki deste sayacini gosterebilmek icin.
    Deck<card*>& getDeck() { return yourDeck; }

    void setCharacter(Character* activeCharacter) {this->activeCharacter = activeCharacter; }

    Character* getCharacter(){return activeCharacter;}

    bool hasCharacter(){ return activeCharacter != nullptr; }

    
    bool useCharacterAbility(IDamageable* target, Deck<card*>& battlefield, Deck<card*>& enemyTeam){
        if(activeCharacter == nullptr && abilityPoint == 0) return false;
        actionPoint--;
        return activeCharacter->tryUseAbility(target, battlefield, team, enemyTeam);
    }

    void revertCharacterAbility(IDamageable* target, Deck<card*>& battlefield,
                            Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
    if(activeCharacter) activeCharacter->revert(target, battlefield, allyTeam, enemyTeam);
}

    void tickCharacterCooldown(){
        if(activeCharacter && activeCharacter->getcurrentCooldown() > 0)
            activeCharacter->tickCoolDown();
    }

    int getCharacterAbilityDuration(){
        return activeCharacter ? activeCharacter->getAbilityDuration() : 0;
    }

    void setActionAndAbilityPoint(int actionPoint, int abilityPoint){
        this->actionPoint = actionPoint;
        this->abilityPoint = abilityPoint;
    }

    int getActionPoint(){return actionPoint;}
    int getAbilityPoint(){return abilityPoint;}

    std::stack<card*>& getGraveyard(){return graveyard;}

};

#endif // PLAYER_H
