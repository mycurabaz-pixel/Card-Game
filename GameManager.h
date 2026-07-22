#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <string>
#include <vector>
#include <utility>   // std::pair, std::move
#include "Deck.h"
#include "Card.h"
#include "Player.h"
#include"Character.h"


    struct TimedAbility {
    Player* source;   // yetenegi kullanan oyuncu (karakter onun icinde)
    Player* enemy;    // rakip oyuncu (enemyTeam icin)
    int expireTurn;
    IDamageable* target;
    card* usedCard;
    // NOT: Neyin geri alinacagini burada tutmuyoruz. Her karakterin apply lambda'si
    // degistirdigi degerleri kendi icinde kaydeder, revert lambda'si da oradan geri yazar.
    // Bu sayede sadece dmgHit degil her turlu etki (invulnerable, mana, ...) geri alinabilir.
};

class gameManager {

    Deck<card*> master_deck;
    Deck<card*> battle_field;
    Player player1{"player1"};
    Player player2{"player2"};
    std::map<std::string, Character*> character_pool;
    int turn_counter = 0;   // her oyuncu kart oynadiginda artar -> tur basina 2 kez
    int tour_counter = 0;   // tam bir tur (her iki oyuncu oynadi) basina 1 kez artar
    bool isRunning = true; // Oyunun durumunu kontrol eder
    std::vector<TimedAbility> timed_abilities;
    std::vector<TimedAbility> timed_card_abilities;

    gameManager(){}

    public:

    static gameManager& getInstance() { static gameManager instance; return instance; }

    gameManager(const gameManager&) = delete;
    gameManager& operator=(const gameManager&) = delete; // [DEGISIKLIK] parametre const referans yapildi

    void setPlayers(std::string player1_name, std::string player2_name);
    void startGame();
    void playTurn();
    void advanceTurn();   // [YENI] tur sonu (cin yok) - hem konsol hem GUI kullanir
    void executeEffects();
    void run();
    void checkTargetHealth(IDamageable* target);
    void useCharacterAbility(Player& user, Player& enemy, IDamageable* target);
    void useCardAbility(Player& user, Player& enemy, IDamageable* target, card* usedCard);
    void tickCharacters();
    void tickCards();
    void endGame();

    // --- [GUI ICIN EKLENDI] Sadece okuma erisimi; oyun mantigini degistirmez.
    //     GUI tahtayi cizebilmek icin bu verilere ulasmak zorunda.
    Player& getPlayer1(){ return player1; }
    Player& getPlayer2(){ return player2; }
    Deck<card*>& getBattlefield(){ return battle_field; }
    std::map<std::string, Character*>& getCharacterPool(){ return character_pool; }
    int getTourCounter() const { return tour_counter; }

};

#endif // GAMEMANAGER_H
