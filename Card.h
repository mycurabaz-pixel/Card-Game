#ifndef CARD_H
#define CARD_H

#include <string>
#include <map>
#include "IDamageable.h"
#include "Effect.h"
#include "Deck.h"

// card sadece Player* tuttuğu için ileri bildirim yeterli (Player.h'yi include etmiyoruz -> döngü yok)
class Player;

class card : public IDamageable {

    public:

    // DmgType artik Effect.h'de global tanimli (tek kaynak).
    // Bu alias sayesinde mevcut 'card::DmgType::FIRE' kullanimlari aynen calismaya devam eder.
    using DmgType = ::DmgType;

    private:

    int health;
    int maxHealth;
    inline static int id_counter = 1;
    int private_card_id;
    int private_deck_id;
    int range;
    DmgType dmgType;
    int type;
    std::string cardName;
    int cost;
    int dmgHit;
    // [YENI] Kartin degistirilmemis TEMEL hasari (ctor'da bir kez ayarlanir, sabit kalir).
    // GUI su anki dmgHit ile kiyaslayip hasar artmis/azalmis mi gosterir (kilic sembolleri).
    int baseDmgHit;
    std::vector<Effect> effects;
    DmgType immunity;
    DmgType vulnurability;
    Player* owner = nullptr; // [DEGISIKLIK] owner artik nullptr ile basliyor (onceden ilklenmiyordu)
    bool invulnerable = false;
    bool stunned = false;
    bool reflection_marked = false;
    bool dead = false;
    int powerCost;
    int powerDuration;
    int armor = 0;
    bool active = false;
    // [YENI] Kart YETENEGININ hedef tarafi (normal saldiri hit() her zaman rakibe kalir).
    // Varsayilan ENEMY -> hasar veren kartlar oldugu gibi calisir; destek kartlari
    // GameManager'da tek yerden ALLY olarak isaretlenir.
    TargetSide targetSide = TargetSide::ENEMY;
    // [YENI] Yetenegin kac hedef aldigi (1 = tek, >1 = coklu selectTargets). GameManager'da isaretlenir.
    int targetCount = 1;


    public:

    card(int range, DmgType dmgType, int type, std::string cardName, int cost, int dmgHit, int health, DmgType immunity, DmgType vulnurability, int powerCost, int powerDuration){
        this->range = range;
        this->dmgType = dmgType;
        this->type = type;
        this->cardName = cardName;
        this->cost = cost;
        this->dmgHit = dmgHit;
        this->baseDmgHit = dmgHit;   // [YENI] temel hasari sakla (buff/debuff kiyasi icin)
        private_card_id = id_counter;
        id_counter++;
        this->health = health;
        this->immunity = immunity;
        this->vulnurability = vulnurability;
        maxHealth = health;
        this->powerCost = powerCost;
        this->powerDuration = powerDuration;
    }

    DmgType getDamageType(){
        return dmgType;
    }

    Effect hitEffect = {getDamageType(), 2,2,0};

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

    int getBaseDmgHit(){ return baseDmgHit; }   // [YENI] temel hasar (GUI buff/debuff kiyasi)

    void setDmgHit(int dmgHit){
        this->dmgHit = dmgHit;
    }

    void takeDamage(IDamageable* attacker, int damage) override{
                if(invulnerable && damage > 0) return;

                if(maxHealth >= health - damage){
                    if(damage > armor){damage -= armor; armor = 0;}
                    else if(damage > 0){armor -= damage; return;}
                    health -= damage;
                }
                else{
                    // [DUZELTME] Eskiden: else if((health - damage) % maxHealth > damage)
                    // Buraya sadece TASAN IYILESTIRME (negatif damage) dusuyor; yapilmasi
                    // gereken tek sey cani tavana kirpmak. Modulo hem gereksizdi hem de
                    // maxHealth 0 olursa sifira bolme ile programi cokertiyordu.
                    health = maxHealth;
                }
                if(health < 0) health = 0;   // [YENI] can eksiye dusmesin
                if(attacker && damage > 0 && isReflectionMarked()){ reflect(attacker, damage); }
            }

    int getHealth() override {
        return health;
    }

    void setHealth(int health) override {
        // [DUZELTME] else dalinda 'health = maxHealth;' yaziyordu: bu UYE degil PARAMETRE'ye
        // atama yapiyor, yani fonksiyon hicbir sey yapmadan bitiyordu. Ayrica alt sinir yoktu.
        if(health > maxHealth){ this->health = maxHealth; }
        else if(health < 0)   { this->health = 0; }
        else                  { this->health = health; }
    }

    void setMaxHealth(int maxHealth){
        this->maxHealth = maxHealth;
    }

    int getMaxHealth(){return maxHealth;}

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

    void setActive(bool active){this->active = active;}
    bool isActive(){return active;}
    bool isDead(){return dead;}
    void setDead(bool dead){this->dead = dead;}
    int getAbilityCost(){return powerCost;}
    int getPowerDuration(){return powerDuration;}   // [YENI] sureli kart etkisinin geri alinma zamani icin
    
    TargetSide getTargetSide(){return targetSide;}
    void setTargetSide(TargetSide s){targetSide = s;}
    
    int getTargetCount(){return targetCount;}
    void setTargetCount(int n){targetCount = n;}
    void setInvulnerable(bool invulnerable){this->invulnerable = invulnerable;} // [DUZELTME] == degil =
    bool isInvulnerable(){return invulnerable;}
    void setStunned(bool stunned){this->stunned = stunned;}
    bool isStunned(){return stunned;}
    bool isReflectionMarked(){return reflection_marked;}
    void setReflectionMarked(bool reflection_marked){this->reflection_marked = reflection_marked;}
    void reflect(IDamageable* target, int damage){
        if(target->isReflectionMarked()){
            return;
    }
    target->takeDamage(this, damage/3);
}

    void addEffect(Effect effect){

        if(effect.effectType == NORMAL){return;}
        
        if(effect.effectType == vulnurability){
            effect.damage *= 2;
        }
        if(effect.effectType != immunity){
            effects.push_back(effect);
        }
    }

    std::vector<Effect>& getEffects(){
        return effects;
    }

    void addArmor(int armor){
        this->armor += armor;
    }

    int getArmor() override{
        return armor;
    }

    virtual ~card();

    virtual void hit(IDamageable* target) = 0;

    virtual void useUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) = 0;

    virtual void activateTypeBuff() = 0;

    virtual std::string getCardInfo() = 0;

    // Vurusa eklenen rastgele bonusun UST siniri.
    // Her kart turu kendi random_unique_dmg()'sindeki araligi dondurur; boylece
    // araligi degistirdiginde GUI otomatik dogru gosterir (sabit formul yok).
    virtual int getMaxRandomBonus() = 0;

    virtual  void useReverseUniquePower(IDamageable* target, Deck<card*>& battlefield, card* self, Deck<card*> allyTeam, Deck<card*> enemyTeam) = 0;

};

#endif // CARD_H
