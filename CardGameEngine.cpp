#include<iostream>
#include<stack>
#include<random>
#include<map>
#include <functional>
#include<memory> // for smart pointers
#include<algorithm>//for std::shuffle
#include<sstream>

//====================
//1.FOWARD DECLARATIONS
//====================

class Player;
template <typename T> class Deck;

struct Effect
{
    int damage = 2;
    int duration = 3;
    int playedTour = 0;
};


//========
//INTERFACE
//========


class IDamageable {
public:
    virtual void takeDamage(int dmg) = 0;
    virtual std::string getName() = 0;
    virtual int getHealth() = 0;
    virtual void setHealth(int health) = 0;
    virtual ~IDamageable() = default;   
};

template <typename T>
class Deck{

    private:

    std::vector<T> active_deck;
    std::vector<T> deck;

    public:

    void addCardToDeck(T item){
        deck.push_back(item);
    }

    void removeCardFromDeck(T target_card){
        for(auto it= deck.begin(); it != deck.end(); ++it){
            if((*it)->getPrivateCardId() == target_card->getPrivateCardId()){
                deck.erase(it);
                break;
            }
        }
    }

    void shuffle(){
        active_deck = deck;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(active_deck.begin(), active_deck.end(), gen);
    }

    T getTopCard(){
        if(!active_deck.empty()){
        return active_deck.back();
        }
        return nullptr;
    }

    void removeTopCard(){
        if(!active_deck.empty()){
            active_deck.pop_back();
        }
    }

    bool isEmpty(){
        return active_deck.empty();
    }

    const std::vector<T>& getCards() const {
    return deck;
}

};

class card : public IDamageable{

    public: 

    enum DmgType{
        ICE,
        FIRE,
        NATURE,
        WIND,
        MAGIC,
        ANTIMAGIC,
        DARKNESS,
        LIGHT,
        SLICE,
        NORMAL
    };

    private: 

    int health;
    inline static int id_counter = 1;
    int private_card_id;
    int private_deck_id;
    int range;
    DmgType dmgType;
    int type;
    std::string cardName;
    int cost;
    int dmgHit;
    std::map<DmgType, Effect> effects;
    Effect hitEffect;
    DmgType immunity;
    DmgType vulnurability;
    Player* owner;

    public:

    card(int range, DmgType dmgType, int type, std::string cardName, int cost, int dmgHit, int health, DmgType immunity, DmgType vulnurability){
        this->range = range;
        this->dmgType = dmgType;
        this->type = type;
        this->cardName = cardName;
        this->cost = cost;
        this->dmgHit=dmgHit;
        private_card_id = id_counter;
        id_counter++;
        this->health = health;
        this->immunity = immunity;
        this->vulnurability = vulnurability;
    }

    DmgType getDamageType(){
        return dmgType;
    }

    void setDamageType(DmgType dmgType){
        this->dmgType = dmgType;
    }

    int getPrivateCardId(){
        return private_card_id;
    }

    int getType(){
        return type;
    }

    int getCost(){
        return cost;
    }
         
    void updateDmgHit(int dmgHit){
        this->dmgHit = dmgHit;
    }

    int getDmgHit(){
        return dmgHit;
    }

    void takeDamage(int dmg) override {
        health -= dmg;
    }

    int getHealth(){
        return health;
    }

    void setHealth(int health){
        this->health = health;
    }

    std::string getName() override {
        return cardName;
    }

    Effect getHitEffect(){
        return hitEffect;
    }

    void setOwner(Player* owner){
        this->owner = owner;
    }

    Player* getOwner(){
        return owner;
    }

    void addEffect(DmgType hitType, Effect effect){
        if(hitType == vulnurability){
            effect.damage *= 2;
        }
        if(hitType != immunity){
            if(auto it = effects.find(hitType);it != effects.end()){
            effects.at(hitType).duration += effect.duration;
        }
        else{
            effects.emplace(hitType, effect);
        }
        }
    }

    std::map<DmgType, Effect>& getEffects(){
        return effects;
    }

    virtual ~card(){
        std::cout<<"card has been killed"<<std::endl;
    }

    virtual void hit(IDamageable* target) = 0;

    virtual void useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) = 0;

    virtual void activateTypeBuff() = 0;

    virtual std::string getCardInfo() = 0;

};

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
    int unique_ability_effect_power;
    FantasyType fType;
    int unique_dmg_buff=0;
    AbilityFunction uniquePower;

     public:

    fantasyCard(int range, DmgType dmgType, FantasyType type, std::string cardName, int cost, int dmgHit, std::string unique_ability, int unique_ability_effect_power, int health, DmgType vulnurability, DmgType immunity, AbilityFunction  power)
    : card(range, dmgType, type, cardName, cost, dmgHit, health, immunity, vulnurability){

        this->unique_ability = unique_ability;
        this->unique_ability_effect_power = unique_ability_effect_power;
        this->fType = type;
        this->uniquePower = power;
    }

    void random_unique_dmg(){
         std::random_device rd;

         std::mt19937 gen(rd());

         std::uniform_int_distribution<> interval_1(0, (card::getCost() * 2));

         unique_dmg_buff = interval_1(gen);

         card::updateDmgHit(card::getDmgHit() + unique_dmg_buff);
    }

    void hit(IDamageable* target) override {
        random_unique_dmg();
    
    target->takeDamage(card::getDmgHit()); 

    std::cout << card::getName() << ", " << target->getName() << " hedefine " << card::getDmgHit() << " vurdu!" << std::endl;

    if(unique_dmg_buff > card::getCost()/2){
        std::cout << "ENEMY HAS BEEN CURSED" << std::endl;
        if(card* tempTarget = dynamic_cast<card*>(target)){
            tempTarget->addEffect(getDamageType(), getHitEffect());
        }
        }
        card::updateDmgHit(card::getDmgHit() - unique_dmg_buff);
    }

    void useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) override {
   if(uniquePower){
    uniquePower(target, battlefield, this, allyTeam, enemyTeam);
   }
}

    void activateTypeBuff() override {
        std::cout<<"healing every fantasy card on the table and giving extra "<<card::getCost()<<"damage"<<std::endl;
    }

// 1. fantasyCard Sınıfı İçin
std::string getCardInfo() override {
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

};


class TechCard : public card{

    public:

    enum TechnoType{
        ROBOT,
        MECHANIC,
        SOLDIER,
        TITAN
    };

    using AbilityFunction = std::function<void(IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam)>;

    private:

    std::string unique_ability;
    int unique_ability_effect_power;
    TechnoType tType;
    int unique_dmg_buff=0;
    AbilityFunction uniquePower;

    public:

    TechCard(int range, DmgType dmgType, TechnoType type, std::string cardName, int cost, int dmgHit, std::string unique_ability, int unique_ability_effect_power, int health, DmgType vulnurability, DmgType immunity, AbilityFunction  power)
    : card(range, dmgType, type, cardName, cost, dmgHit, health, immunity, vulnurability){
        this->unique_ability = unique_ability;
        this->unique_ability_effect_power = unique_ability_effect_power;
        this->tType = type;
        this->uniquePower = power;
    }

    void random_unique_dmg(){
         std::random_device rd;

         std::mt19937 gen(rd());

         std::uniform_int_distribution<> interval_1(0, (card::getCost() * 2));

         unique_dmg_buff = interval_1(gen);

         card::updateDmgHit(card::getDmgHit() + unique_dmg_buff);
    }

    void hit(IDamageable* target) override {
        random_unique_dmg();
    
    target->takeDamage(card::getDmgHit());  

    if(unique_dmg_buff > card::getCost()/2){
        std::cout << "ENEMY HAS BEEN CURSED" << std::endl;

            switch(card::getDamageType()){

                case ICE:

                std::cout<<"düşman donduruldu"<<std::endl;
                break;

                case FIRE:

                std::cout<<"düşman yakıldı"<<std::endl;
               target->takeDamage(2);
                break;

                case NATURE:

                std::cout<<"ENEMY HAS BEEN ROOTED"<<std::endl;
                break;

                case WIND:

                std::cout<<"ENEMY HAS BEEN SLICED WİTH WİND"<<std::endl;
                 target->takeDamage(card::getDmgHit());

                break;

                case MAGIC:

                std::cout<<"YOU HİT EXTRA MAGIC DAMAGE AND GAINED 2 MANA"<<std::endl;
                target->takeDamage(card::getDmgHit());
                break;

                case ANTIMAGIC:

                std::cout<<"YOU HİT EXTRA ANTIMAGIC DAMAGE AND WASTED ENEMY'S 2 MANA"<<std::endl;
                 target->takeDamage(card::getDmgHit());
                break;

                case DARKNESS:

                std::cout<<"ENEMY BECAME FEARED"<<std::endl;
                break;

                case LIGHT:

                std::cout<<"ENEMY BECAME BLİND"<<std::endl;
                break;

                case SLICE:

                std::cout<<"ENEMY STARTED BLEEDİNG "<<std::endl;
                 target->takeDamage(2);
                break;

                case NORMAL:

                break;
            }
        }
        card::updateDmgHit(card::getDmgHit() - unique_dmg_buff);
    }

   void useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) override {
   if(uniquePower){
    uniquePower(target, battlefield, this, allyTeam, enemyTeam);
   }
}

    void activateTypeBuff() override{
        std::cout<<"Every tech card on the table gets high voltage extra "<<card::getCost()/2<<"extra electrical damage"<<std::endl;
    }

std::string getCardInfo() override {
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

};

class SeaSonCard: public card {

    public:

    enum SeaSonType{
        KRAKEN,
        MEDIC,
        PIRATE,
        ORACLE
    };

    using AbilityFunction = std::function<void(IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam)>;

    private:

    std::string unique_ability;
    int unique_ability_effect_power;
    SeaSonType sType;
    int unique_dmg_buff=0;
    AbilityFunction uniquePower;

    public:

    SeaSonCard(int range, DmgType dmgType, SeaSonType type, std::string cardName, int cost, int dmgHit, std::string unique_ability, int unique_ability_effect_power, int health, DmgType vulnurability, DmgType immunity, AbilityFunction  power)
    : card(range, dmgType, type, cardName, cost, dmgHit, health, immunity, vulnurability){
        this->unique_ability = unique_ability;
        this->unique_ability_effect_power = unique_ability_effect_power;
        this->sType = type;
        this->uniquePower = power;
    }

    void random_unique_dmg(){
         std::random_device rd;

         std::mt19937 gen(rd());

         std::uniform_int_distribution<> interval_1(0, (card::getCost() * 2));

         unique_dmg_buff = interval_1(gen);

         card::updateDmgHit(card::getDmgHit() + unique_dmg_buff);
    }

     void hit(IDamageable* target) override {
        random_unique_dmg();
    
    target->takeDamage(card::getDmgHit());  

    if(unique_dmg_buff > card::getCost()/2){
        std::cout << "ENEMY HAS BEEN CURSED" << std::endl;

           switch(card::getDamageType()){

                case ICE:

                std::cout<<"düşman donduruldu"<<std::endl;
                break;

                case FIRE:

                std::cout<<"düşman yakıldı"<<std::endl;
               target->takeDamage(2);
                break;

                case NATURE:

                std::cout<<"ENEMY HAS BEEN ROOTED"<<std::endl;
                break;

                case WIND:

                std::cout<<"ENEMY HAS BEEN SLICED WİTH WİND"<<std::endl;
                 target->takeDamage(card::getDmgHit());

                break;

                case MAGIC:

                std::cout<<"YOU HİT EXTRA MAGIC DAMAGE AND GAINED 2 MANA"<<std::endl;
                target->takeDamage(card::getDmgHit());
                break;

                case ANTIMAGIC:

                std::cout<<"YOU HİT EXTRA ANTIMAGIC DAMAGE AND WASTED ENEMY'S 2 MANA"<<std::endl;
                 target->takeDamage(card::getDmgHit());
                break;

                case DARKNESS:

                std::cout<<"ENEMY BECAME FEARED"<<std::endl;
                break;

                case LIGHT:

                std::cout<<"ENEMY BECAME BLİND"<<std::endl;
                break;

                case SLICE:

                std::cout<<"ENEMY STARTED BLEEDİNG "<<std::endl;
                 target->takeDamage(2);
                break;

                case NORMAL:

                break;
            }
        }
        card::updateDmgHit(card::getDmgHit() - unique_dmg_buff);
    }

     void useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) override {
   if(uniquePower){
    uniquePower(target, battlefield, this, allyTeam, enemyTeam);
   }
}

    void activateTypeBuff() override{
        std::cout<<"Every sea son card on the table gets extra "<<card::getCost()/2<<"damage to marked enemies"<<std::endl;
    }

    std::string getCardInfo() override {
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

    info << "Sea Curse      : " << unique_ability << " (Effect Power: " << unique_ability_effect_power << ")\n";
    info << "======================================\n";
    
    return info.str();
}

};
 

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

    void setHealth(int health){
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

    void drawCard(){
        if(!yourDeck.isEmpty()){
            card* drawnCard = yourDeck.getTopCard();

            hand.push_back(drawnCard);

            yourDeck.removeTopCard();

            std::cout<<player_name<<"has drawn a card."<<std::endl;
        }
        else{
            std::cout<<"Deck is empty, cant drawn a card!"<<std::endl;
        }
    }

    void playCard(card* played_card, Deck<card*>& battlefield){
    for(auto it = hand.begin(); it != hand.end(); ++it){
        if((*it)->getPrivateCardId() == played_card->getPrivateCardId()){
            
            
           battlefield.addCardToDeck(played_card);
           team.addCardToDeck(played_card);
            hand.erase(it);
            break;
        }
    }
}

    void shuffleDeck(){
        yourDeck.shuffle();
    }

    card* selectCard(){
        int i = 0;
        int selection;
        for(card* c: hand){
            std::cout<<i++<<".card "<<c->getName()<<std::endl;
        }
        std::cout<<"seçiminiz:"<<std::endl;
        std::cin>>selection;
        return hand.at(selection);
    }

    void hitTarget(IDamageable* target, card* usedCard){
        usedCard->hit(target);
        gameManager::getInstance().checkTargetHealth(target);
    }

    void useCardUniqueAbility(IDamageable* target, Deck<card*>& battlefield, card* used_card,
                          Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
    used_card->useUniquePower(target, battlefield, used_card, allyTeam, enemyTeam);

    gameManager::getInstance().checkTargetHealth(target);
}
    

    void addToTeam(card* c) { team.addCardToDeck(c); }
    Deck<card*>& getTeam() { return team; }

};

class Character : IDamageable{
    using Ability = std::function<void ( IDamageable* target, 
        Deck<card*>& battlefield, 
        Deck<card*>& allyTeam, 
        Deck<card*>& enemyTeam)>;

        private:

        std::string name;
        int health, maxHealth, mana, maxMana, abilityCoolDown, abilityDuration, abilityCost;
        Ability ability;

        Character(std::string name, int Maxhealth, int maxMana, int abilityCooldown, int abilityDuration, int abilityCost):
            name(std::move(name)), 
            maxHealth(std::move(maxHealth)), 
            health(std::move(maxHealth)), 
            maxMana(std::move(maxMana)), 
            mana(std::move(maxMana)), 
            abilityCoolDown(std::move(abilityCoolDown)), 
            abilityDuration(std::move(abilityDuration)), 
            abilityCost(std::move(abilityCost)) {}

            void useAbility(IDamageable* target, Deck<card*> battlefield, Deck<card*> allyTeam, Deck<card*> enemyTeam){
                if(ability) ability(target, battlefield, allyTeam, enemyTeam);
            }
};

class gameManager{
    Deck<card*> master_deck;
    Deck<card*> battle_field;
    Player player1{"player1"};
    Player player2{"player2"};
    int turn_counter = 0;

    private:
     gameManager(){}

    public:

    static gameManager& getInstance() { static gameManager instance; return instance; }

    gameManager(const gameManager&) = delete;
    gameManager& operator=(const gameManager) = delete;

    void setPlayers(std::string player1_name, std::string player2_name){
        player1 = Player(player1_name);
         player2 = Player(player2_name);
    }

void startGame() {
    // ==========================================
    // 20 FANTASY CARDS (With Lambda Abilities)
    // ==========================================
    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::FIRE, fantasyCard::FantasyType::DRAGON, "Flame Dragon", 7, 45, "Breath", 20, 300, card::DmgType::ICE, card::DmgType::FIRE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Set the battlefield on fire!" << std::endl;
            for(card* c: pitch.getCards()) {
                c->takeDamage(5);
            }
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::FIRE, fantasyCard::FantasyType::DRAGON, "Red Dragon", 6, 40, "Ember", 18, 400, card::DmgType::ICE, card::DmgType::FIRE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Shot a critical ember!" << std::endl; 
            if(target != nullptr){
            target->takeDamage(40);
            }
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::ICE, fantasyCard::FantasyType::HEALER, "Ice Healer", 3, 10, "Polar", 15, 50, card::DmgType::FIRE, card::DmgType::ICE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Healed ally cards by freezing them!" << std::endl;
           if(target != nullptr){
            target->takeDamage(-20);
            }
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::ICE, fantasyCard::FantasyType::HEALER, "Frost Guardian", 3, 12, "Ice Shield", 12, 70, card::DmgType::FIRE, card::DmgType::ICE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Granted ice shields to everyone!" << std::endl; 
            for(card* c: pitch.getCards()) {
                c->takeDamage(-5);
            }
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::SLICE, fantasyCard::FantasyType::SOLDIER, "Cruel Soldier", 4, 30, "Dance", 10, 30, card::DmgType::MAGIC, card::DmgType::NORMAL,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Sword speed doubled!" << std::endl;
            self->updateDmgHit(40);
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::SLICE, fantasyCard::FantasyType::SOLDIER, "Dawn Knight", 4, 35, "Strike of justice", 8, 150, card::DmgType::DARKNESS, card::DmgType::LIGHT,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Struck with a strike of justice!" << std::endl; 
             if(target != nullptr){
                target->takeDamage(25);
            } 
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        3, card::DmgType::MAGIC, fantasyCard::FantasyType::MAGICIAN, "Ancient Mage", 6, 25, "Meteor", 35, 90, card::DmgType::ANTIMAGIC, card::DmgType::MAGIC,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Rained meteors from the sky!" << std::endl; 
            for(card* c: pitch.getCards()) {
                c->takeDamage(30);
            }
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        3, card::DmgType::MAGIC, fantasyCard::FantasyType::MAGICIAN, "Sky Mage", 5, 22, "Magic breaker", 25, 45, card::DmgType::ANTIMAGIC, card::DmgType::WIND,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Broke magic resistance!" << std::endl; 
            self->updateDmgHit(40);
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::NATURE, fantasyCard::FantasyType::SOLDIER, "Forest Guardian", 5, 28, "Vine Trap", 12, 200, card::DmgType::FIRE, card::DmgType::NATURE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Rooted enemies with vines!" << std::endl; 
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::NATURE, fantasyCard::FantasyType::HEALER, "Dryad", 2, 8, "Blessing", 10, 115, card::DmgType::FIRE, card::DmgType::NATURE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Called upon Mother Nature's blessing!" << std::endl;
            if(target != nullptr){
                target->takeDamage(-30);
            } 
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::WIND, fantasyCard::FantasyType::DRAGON, "Storm Dragon", 7, 42, "Lightning Breath", 22, 300, card::DmgType::MAGIC, card::DmgType::WIND,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Created a hurricane!" << std::endl; 
            target->takeDamage(30);
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        3, card::DmgType::DARKNESS, fantasyCard::FantasyType::MAGICIAN, "Dark Mage", 6, 27, "transfusion", 30, 60, card::DmgType::LIGHT, card::DmgType::DARKNESS,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Cast life drain!" << std::endl; 
            if(target){
                target->takeDamage(30);
            }
            if(self){
                self->takeDamage(-20);
            }
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::LIGHT, fantasyCard::FantasyType::HEALER, "Light Priest", 4, 14, "Blinding Light", 20, 35, card::DmgType::DARKNESS, card::DmgType::LIGHT,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Blinded everything with light!" << std::endl; 
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::NORMAL, fantasyCard::FantasyType::SOLDIER, "Village Vanguard", 1, 15, "headbutt", 5, 130, card::DmgType::MAGIC, card::DmgType::NORMAL,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Performed a simple but effective headbutt!" << std::endl; 
            if(target){
                target->takeDamage(20);
            }
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::ANTIMAGIC, fantasyCard::FantasyType::MAGICIAN, "Spellbreaker", 5, 20, "Cleanse", 15,  50, card::DmgType::NORMAL, card::DmgType::MAGIC,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Cleared all spells in the area!" << std::endl; 
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::FIRE, fantasyCard::FantasyType::SOLDIER, "Ember Guardian", 3, 25, "Ember", 8, 85, card::DmgType::ICE, card::DmgType::FIRE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Infused weapon with fire!" << std::endl; 
            self->updateDmgHit(self->getDmgHit() * 2);
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        3, card::DmgType::ICE, fantasyCard::FantasyType::MAGICIAN, "Cryomancer", 5, 24, "Blizzard", 22,  60, card::DmgType::FIRE, card::DmgType::ICE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Started a blizzard!" << std::endl; 
            for(card* c: pitch.getCards()){
                c->takeDamage(20);
            }
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::WIND, fantasyCard::FantasyType::HEALER, "Wind Healer", 3, 11, "Wind Cure", 14, 40, card::DmgType::MAGIC, card::DmgType::WIND,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Cleared poison with wind!" << std::endl; 
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::DARKNESS, fantasyCard::FantasyType::DRAGON, "Shadow Dragon", 8, 50, "Dark Breath", 25, 300, card::DmgType::LIGHT, card::DmgType::DARKNESS,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Cast dark fear!" << std::endl; 
            for(card* c:pitch.getCards()){
                c->takeDamage(40);
            }
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::SLICE, fantasyCard::FantasyType::SOLDIER, "Barbarian", 4, 32, "Rage", 11, 40, card::DmgType::MAGIC, card::DmgType::NORMAL,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Entered rage mode!" << std::endl; 
            if(self){
                self->updateDmgHit(self->getDmgHit()*2);
                self->setDamageType(card::SLICE);
            }
        }
    ));

    // ==========================================
    // 20 TECH CARDS (With Lambda Abilities)
    // ==========================================
    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::LIGHT, TechCard::TechnoType::ROBOT, "Cyber Observer", 4, 22, "Laser", 18, 250, card::DmgType::DARKNESS, card::DmgType::LIGHT,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Locked onto the target!" << std::endl; 
            if(target){
                target->takeDamage(50);
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::LIGHT, TechCard::TechnoType::ROBOT, "High Tech Robot", 4, 25, "Plasma Shot", 15, 400, card::DmgType::DARKNESS, card::DmgType::LIGHT,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Fired a plasma ball!" << std::endl; 
            if(target){
                target->takeDamage(40);
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::NORMAL, TechCard::TechnoType::MECHANIC, "Chief Engineer", 3, 15, "Turret", 5, 45, card::DmgType::MAGIC, card::DmgType::NORMAL,
        [&](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
         std::cout << "-> Ability: Deployed an automatic turret!" << std::endl; 
         card* turret = new TechCard(1 ,card::DmgType::NORMAL, TechCard::TechnoType::MECHANIC, "Turret", 0, 5, "NONE", 0, 10, card::DmgType::MAGIC, card::DmgType::NORMAL,
             [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){});
            battle_field.addCardToDeck(turret); 
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::NORMAL, TechCard::TechnoType::MECHANIC, "Blacksmith Robot", 3, 12, "Repairment", 8, 90, card::DmgType::MAGIC, card::DmgType::NORMAL,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Repaired armor!" << std::endl; 
            target->takeDamage(-30);
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        2, card::DmgType::ANTIMAGIC, TechCard::TechnoType::SOLDIER, "Pulse Guardian", 5, 32, "Shield", 25, 60, card::DmgType::SLICE, card::DmgType::MAGIC,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Emitted an electromagnetic wave!" << std::endl; 
            if(target){
                target->takeDamage(-20);
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        2, card::DmgType::ANTIMAGIC, TechCard::TechnoType::SOLDIER, "Cyber Protector", 4, 28, "firewall shield", 20, 50, card::DmgType::SLICE, card::DmgType::MAGIC,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Activated firewall shield!" << std::endl;
            if(target){
                target->takeDamage(-20);
            } 
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::DARKNESS, TechCard::TechnoType::TITAN, "Doomsday Tank", 8, 55, "Shock", 40, 200, card::DmgType::LIGHT, card::DmgType::DARKNESS,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Fired a nuclear missile!" << std::endl; 
            if(target){
                target->takeDamage(70);
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::DARKNESS, TechCard::TechnoType::TITAN, "Goliath V2", 8, 58, "Pierceing Shot", 35, 500, card::DmgType::LIGHT, card::DmgType::DARKNESS,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Pierced through with a railgun!" << std::endl; 
            if(target){
                target->takeDamage(30);
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::WIND, TechCard::TechnoType::ROBOT, "Light Drone", 2, 12, "Wave", 8, 10, card::DmgType::FIRE, card::DmgType::WIND,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Activated the signal jammer!" << std::endl; 
            card* targetAsCard = dynamic_cast<card*>(target);
            if(targetAsCard){
                targetAsCard->updateDmgHit(targetAsCard->getDmgHit()/2);
            }
            else{
                std::cout<<"This ability only can be used on a card"<<std::endl;
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::WIND, TechCard::TechnoType::ROBOT, "Swift Jet", 3, 18, "Wave", 12, 140, card::DmgType::ICE, card::DmgType::WIND,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Caused a supersonic blast!" << std::endl; 
            if(target){
                target->takeDamage(60);
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        2, card::DmgType::FIRE, TechCard::TechnoType::MECHANIC, "Pyro-Technician", 4, 20, "Explosion", 15, 60, card::DmgType::ICE, card::DmgType::FIRE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Launched a napalm bomb!" << std::endl; 
            for(card* c:pitch.getCards()){
                c->takeDamage(40);
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::ICE, TechCard::TechnoType::SOLDIER, "Freezing Android", 4, 26, "Shield", 14, 75, card::DmgType::FIRE, card::DmgType::ICE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Sprayed liquid nitrogen!" << std::endl;
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        2, card::DmgType::NATURE, TechCard::TechnoType::ROBOT, "Bio-Mechanical Golem", 5, 30, "Laser", 22, 150, card::DmgType::FIRE, card::DmgType::NATURE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Activated organic batteries!" << std::endl; 
            if(self){
                self->updateDmgHit(self->getDmgHit()*3);
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::MAGIC, TechCard::TechnoType::TITAN, "Rune Circuit", 7, 40, "Shock", 30, 40, card::DmgType::ANTIMAGIC, card::DmgType::MAGIC,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Triggered a magical overload!" << std::endl; 
            if(self){
                self->updateDmgHit(self->getDmgHit()*3);
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::SLICE, TechCard::TechnoType::SOLDIER, "Saw Robot", 3, 24, "Slice and Dice", 10, 80, card::DmgType::MAGIC, card::DmgType::NORMAL,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Overclocked the blades!" << std::endl; 
            if(target){
                target->takeDamage(40);
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::LIGHT, TechCard::TechnoType::ROBOT, "Quantum Reflector", 5, 23, "Laser", 16, 40, card::DmgType::DARKNESS, card::DmgType::LIGHT,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Bent light to become invisible!" << std::endl; 
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::NORMAL, TechCard::TechnoType::MECHANIC, "Repair Unit", 2, 10, "Nanorobot treatment", 12, 40, card::DmgType::MAGIC, card::DmgType::NORMAL,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Started repairs with nanobots!" << std::endl; 
            for(card* c:allyTeam.getCards()){
                c->takeDamage(-10);
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        2, card::DmgType::DARKNESS, TechCard::TechnoType::TITAN, "Dark Matter Armor", 9, 60, "black shield", 45, 50, card::DmgType::LIGHT, card::DmgType::DARKNESS,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Created a black hole!" << std::endl; 
            if(target){
                target->takeDamage(-30);
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::WIND, TechCard::TechnoType::ROBOT, "Sonic Copter", 3, 15, "Wave", 10, 50, card::DmgType::ICE, card::DmgType::WIND,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Blew away with propeller wind!" << std::endl; 
            for(card* c: enemyTeam.getCards()){
                c->takeDamage(10);
            }
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        2, card::DmgType::SLICE, TechCard::TechnoType::SOLDIER, "Cybernetic Ninja", 4, 34, "Techno Slash", 18, 40, card::DmgType::MAGIC, card::DmgType::NORMAL,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Emitted a katana shockwave!" << std::endl; 
            if(self){
                self->updateDmgHit(self->getDmgHit()*2);
            }
        }
    ));

    // ==========================================
    // 20 SEA SON CARDS (With Lambda Abilities)
    // ==========================================
    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::DARKNESS, SeaSonCard::SeaSonType::KRAKEN, "Nightmare Kraken", 9, 60, "Vortex", 50, 400, card::DmgType::LIGHT, card::DmgType::DARKNESS,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Cast a deep sea curse!" << std::endl; 
            for(card* c:enemyTeam.getCards()){
                c->setHealth(c->getHealth()/2);
            }
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::DARKNESS, SeaSonCard::SeaSonType::KRAKEN, "Leviathan", 9, 65, "Tsunami", 45, 450, card::DmgType::LIGHT, card::DmgType::DARKNESS,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Started a tsunami!" << std::endl; 
            for(card* c: pitch.getCards()){
                c->takeDamage(15);
            }
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::NATURE, SeaSonCard::SeaSonType::MEDIC, "Coral Healer", 3, 8, "Wave", 18, 35, card::DmgType::FIRE, card::DmgType::NATURE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Active coral healing!" << std::endl; 
            target->takeDamage(-20);
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::NATURE, SeaSonCard::SeaSonType::MEDIC, "Mermaid", 3, 9, "Song of the sea", 16, 30, card::DmgType::FIRE, card::DmgType::NATURE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Sang the current song!" << std::endl;
            for(card* c: allyTeam.getCards()){
                c->takeDamage(-5);
            } 
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::SLICE, SeaSonCard::SeaSonType::PIRATE, "Blackbeard", 4, 35, "Boarding", 14, 70, card::DmgType::MAGIC, card::DmgType::NORMAL,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Executed a boarding attack!" << std::endl; 
            for(card* c: enemyTeam.getCards()){
                c->takeDamage(10);
            }
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::SLICE, SeaSonCard::SeaSonType::PIRATE, "Captain Barbarossa", 5, 38, "Blast", 12, 70, card::DmgType::MAGIC, card::DmgType::NORMAL,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Subjected the fleet to cannon fire!" << std::endl;
            target->takeDamage(40); 
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        3, card::DmgType::MAGIC, SeaSonCard::SeaSonType::ORACLE, "Sea Oracle", 5, 20, "Deluge", 30, 50, card::DmgType::ANTIMAGIC, card::DmgType::MAGIC,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Saw the storm of the future!" << std::endl; 
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        3, card::DmgType::MAGIC, SeaSonCard::SeaSonType::ORACLE, "Siren Song", 4, 18, "Deluge", 25, 100, card::DmgType::ANTIMAGIC, card::DmgType::MAGIC,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Charmed the enemy card!" << std::endl; 
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::ICE, SeaSonCard::SeaSonType::PIRATE, "Arctic Raider", 4, 27, "Anchor", 11, 40, card::DmgType::FIRE, card::DmgType::ICE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Fired a frozen harpoon!" << std::endl; 
            target->takeDamage(30);
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::ICE, SeaSonCard::SeaSonType::KRAKEN, "Frozen Giant Octopus", 7, 44, "Deep frozen", 28, 300, card::DmgType::FIRE, card::DmgType::ICE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Threw an ice sheet with tentacles!" << std::endl; 
            target->takeDamage(60);
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::FIRE, SeaSonCard::SeaSonType::PIRATE, "Pirate Cannoneer", 4, 33, "Blast", 15, 35, card::DmgType::ICE, card::DmgType::FIRE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Used Greek fire!" << std::endl; 
            for(card* c: enemyTeam.getCards()){
                c->takeDamage(5);
            }
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        3, card::DmgType::WIND, SeaSonCard::SeaSonType::ORACLE, "Stormbringer", 5, 21, "Deluge", 22, 55, card::DmgType::MAGIC, card::DmgType::WIND,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Blew a capsizing wind!" << std::endl; 
            target->takeDamage(30);
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::LIGHT, SeaSonCard::SeaSonType::MEDIC, "Sea Glow Sprite", 3, 10, "Night Glow", 20, 80, card::DmgType::DARKNESS, card::DmgType::LIGHT,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Blinded with a night glow!" << std::endl; 
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::NORMAL, SeaSonCard::SeaSonType::PIRATE, "Ship Crew", 1, 12, "Drunken sailor", 4, 25, card::DmgType::MAGIC, card::DmgType::NORMAL,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Threw a rum bottle!" << std::endl; 
            target->takeDamage(10);
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::ANTIMAGIC, SeaSonCard::SeaSonType::ORACLE, "Deepwater Guardian", 6, 26, "Deluge", 18, 90, card::DmgType::NORMAL, card::DmgType::MAGIC,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Will drown the magic shield in a vortex!" << std::endl; 
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::SLICE, SeaSonCard::SeaSonType::KRAKEN, "Razor Tentacle", 6, 40, "Eight Slash", 20, 350, card::DmgType::MAGIC, card::DmgType::NORMAL,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Sharp tentacle slash!" << std::endl; 
            for(card* c: enemyTeam.getCards()){
                c->takeDamage(20);
            }
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::ICE, SeaSonCard::SeaSonType::MEDIC, "Glacial Savior", 4, 12, "Healing Wave", 14, 55, card::DmgType::FIRE, card::DmgType::ICE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Performed cold water shock therapy!" << std::endl; 
            for(card* c: allyTeam.getCards()){
                c->takeDamage(-10);
            }
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        3, card::DmgType::DARKNESS, SeaSonCard::SeaSonType::ORACLE, "Abyss Witch", 5, 22, "Darkness of the deep sea", 26, 60, card::DmgType::LIGHT, card::DmgType::DARKNESS,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Mariana trench darkness!" << std::endl; 
            target->takeDamage(35);
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::FIRE, SeaSonCard::SeaSonType::PIRATE, "Lava Captain", 5, 36, "Lava shot", 16, 75, card::DmgType::ICE, card::DmgType::FIRE,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Volcanic pirate attack!" << std::endl; 
            target->takeDamage(45);
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::WIND, SeaSonCard::SeaSonType::MEDIC, "Monsoon Healer", 3, 11, "Wind therapy", 12, 40, card::DmgType::MAGIC, card::DmgType::WIND,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) { 
            std::cout << "-> Ability: Humid storm healing!" << std::endl; 
            target->takeDamage(-25);
        }
    ));

    master_deck.shuffle();

    for (int i = 0; i < 30; ++i) {
        if (!master_deck.isEmpty()) {
            player1.addCardToPlayerDeck(master_deck.getTopCard());
            master_deck.removeTopCard();
            master_deck.getTopCard()->setOwner(&player1);
        }
        if (!master_deck.isEmpty()) {
            player2.addCardToPlayerDeck(master_deck.getTopCard());
            master_deck.getTopCard()->setOwner(&player2);
            master_deck.removeTopCard();
        }
    }

    player1.shuffleDeck();
    player2.shuffleDeck();
   
    std::cout << "--- Desteler Hazirlandi ve Karistirildi (Her oyuncuda 30 kart var) ---\n" << std::endl;
}

    void playTurn(){
        std::cout << "--------------------------\n" << std::endl;
        std::cout<<"--- Card Drawing Phase ---"<<std::endl;

        player1.drawCard();
        player2.drawCard();

        card* p1_card = player1.selectCard();
        if(p1_card != nullptr){
            std::cout<<player1.getName()<<"is playing..."<<std::endl;
            player1.playCard(p1_card, battle_field);
            turn_counter++;
        }

        card* p2_card = player2.selectCard();
        if(p2_card != nullptr){
            std::cout<<"\n"<<player2.getName()<<" is playing..."<<std::endl;
            player2.playCard(p2_card, battle_field);
            turn_counter++;
        }

    }

    void executeEffects(){
        for(card* c: battle_field.getCards()){
            auto& current_effects = c->getEffects();
                for(auto it = current_effects.begin();it!=current_effects.end();){
                    if((turn_counter - it->second.playedTour) % 2 == 0){
                        c->takeDamage(it->second.damage);
                        it->second.duration--;
                    }
                    if(it->second.duration == 0){
                        c->getEffects().erase(it);
                    }
                }
            }
        }

    bool isRunning = true; // Oyunun durumunu kontrol eder

public:
    void run() {
        int choice;
        bool isStarted = true;
        while (isRunning) {
            std::cout << "\n=== ANA MENU ===" << std::endl;
            std::cout << "1. Oyunculari Kaydet ve Oyunu Baslat" << std::endl;
            std::cout << "2. Bir Tur Oyna (Play Turn)" << std::endl;
            std::cout << "3. Cikis" << std::endl;
            std::cout << "Seciminiz: ";
            std::cin >> choice;
            std::cin.ignore(256, '\n');

            switch (choice) {
                case 1: {
                    if(isStarted){
                    std::string n1, n2;
                    std::cout << "1. Oyuncu ismi: "; std::cin >> n1;std::cin.ignore(256, '\n');
                    std::cout << "2. Oyuncu ismi: "; std::cin >> n2;std::cin.ignore(256, '\n');
                    setPlayers(n1, n2);
                    startGame();
                    isStarted = true;
                    break;
                    }
                }
                case 2:
                    playTurn();
                    break;
                case 3:
                    isRunning = false;
                    std::cout << "Cikis yapiliyor..." << std::endl;
                    break;
                default:
                    std::cout << "Gecersiz secim!" << std::endl;
            }
        }
    }

    void checkTargetHealth(IDamageable* target) {

    if (target == nullptr) {
        std::vector<card*> deads;
        for (card* c : battle_field.getCards()) {
            if (c->getHealth() <= 0) deads.push_back(c);
        }
        for (card* c : deads) {                    
            std::cout << c->getName() << " died!" << std::endl;
            battle_field.removeCardFromDeck(c);
            c->getOwner()->getTeam().removeCardFromDeck(c);
        }
        return;
    }

    if (target->getHealth() <= 0) {
        card* deadCard = dynamic_cast<card*>(target);
        if (deadCard != nullptr) {                
            std::cout << deadCard->getName() << " died!" << std::endl;
            battle_field.removeCardFromDeck(deadCard);
             deadCard->getOwner()->getTeam().removeCardFromDeck(deadCard);
        }
    }
}

};

int main() {

    gameManager::getInstance().run();
    return 0;
};
