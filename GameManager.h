#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <string>
#include <vector>
#include "Deck.h"
#include "Card.h"
#include "Player.h"

class gameManager {

    Deck<card*> master_deck;
    Deck<card*> battle_field;
    Player player1{"player1"};
    Player player2{"player2"};
    int turn_counter = 0;
    bool isRunning = true; // Oyunun durumunu kontrol eder

    gameManager(){}

    public:

    static gameManager& getInstance() { static gameManager instance; return instance; }

    gameManager(const gameManager&) = delete;
    gameManager& operator=(const gameManager&) = delete; // [DEGISIKLIK] parametre const referans yapildi

    void setPlayers(std::string player1_name, std::string player2_name);
    void startGame();
    void playTurn();
    void executeEffects();
    void run();
    void checkTargetHealth(IDamageable* target);

};

#endif // GAMEMANAGER_H
