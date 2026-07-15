#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <vector>
#include "IDamageable.h"
#include "Deck.h"
#include "Card.h"

class Player : public IDamageable {

    private:

    std::string player_name;
    int health = 1000;
    inline static int playerId = 1;
    Deck<card*> yourDeck;
    std::vector<card*> hand;
    Deck<card*> team;

    public:

    Player(std::string name) : player_name(name) {}

    int getHealth() override{
        return health;
    }

    void setHealth(int health) override {
        this->health = health;
    }

    std::string getName() override {
        return player_name;
    }

    void addCardToPlayerDeck(card* c){
        yourDeck.addCardToDeck(c);
    }

    void takeDamage(int dmg) override {
        health -= dmg;
    }

    // Govdesi cout/cin veya gameManager kullandigi icin tanimlar Player.cpp'de:
    void drawCard();
    void playCard(card* played_card, Deck<card*>& battlefield);
    card* selectCard();
    void hitTarget(IDamageable* target, card* usedCard);
    void useCardUniqueAbility(IDamageable* target, Deck<card*>& battlefield, card* used_card,
                              Deck<card*>& allyTeam, Deck<card*>& enemyTeam);

    void shuffleDeck(){
        yourDeck.shuffle();
    }

    void addToTeam(card* c) { team.addCardToDeck(c); }
    Deck<card*>& getTeam() { return team; }

};

#endif // PLAYER_H
