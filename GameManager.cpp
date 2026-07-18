#include "GameManager.h"
#include "FantasyCard.h"
#include "TechCard.h"
#include "SeaSonCard.h"
#include <iostream>
#include <string>
#include <vector>
#include <memory>    // std::shared_ptr, std::make_shared (yetenek lambda'larinin ortak state'i icin)
#include <utility>   // std::pair

// NOT: TimedAbility yapisi artik GameManager.h icinde tanimli (buradaki kopyasi kaldirildi,
// yoksa ayni ceviri biriminde "redefinition" hatasi olurdu).

void gameManager::setPlayers(std::string player1_name, std::string player2_name){
    player1 = Player(player1_name);
    player2 = Player(player2_name);
}

void gameManager::startGame() {

    //===================
    //CHARACTER SELECTİON
    //===================

    // Her karakterin KENDI 'originals' listesi olmali -> her biri ayri kapsamda (blok icinde)
    // olusturuluyor. apply ve revert lambda'lari ayni shared_ptr'i yakaladigi icin ayni listeyi
    // paylasirlar: apply eski degeri kaydeder, revert birebir geri yazar.
    {
        auto originals = std::make_shared<std::vector<std::pair<card*, int>>>();

        character_pool.emplace("Hope of the Algeron", new Character(
            "Knight of the Light", 1500, 30, 3, 1, 3,

            // APPLY: once eski dmgHit'i kaydet, sonra degistir
            [originals](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                originals->clear();
                for(card* c : allyTeam.getCards()){
                    originals->push_back({ c, c->getDmgHit() });   // eski deger
                    c->takeDamage(-10);                            // iyilestirme: kalici (geri alinmaz)
                    c->setDmgHit(c->getDmgHit() + 5);
                }
            },

            // REVERT: kaydedilen listeden birebir geri yaz (canli takima bakmaz)
            [originals](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                for(auto& p : *originals) p.first->setDmgHit(p.second);
                originals->clear();
            }));
    }

    {
        auto originals = std::make_shared<std::vector<std::pair<card*, int>>>();

        character_pool.emplace("First Prince of Kangreya", new Character(
            "Arrival of the Throne", 1300, 40, 4, 2, 5,

            // APPLY: rakip takimin gucunu yariya bol (eski degerleri kaydederek)
            [originals](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                originals->clear();
                for(card* c : enemyTeam.getCards()){
                    originals->push_back({ c, c->getDmgHit() });
                    c->setDmgHit(c->getDmgHit() / 2);
                }
            },

            [originals](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                for(auto& p : *originals) p.first->setDmgHit(p.second);
                originals->clear();
            }));
    }

    {
        auto self = std::make_shared<Character*>(nullptr);

        Character* ch = new Character(
            "Princess of Kangreya", 900, 60, 5, 2, 7, [self](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                            for(card* c : allyTeam.getCards()){c->setInvulnerable(true);}
                            (*self)->setInvulnerable(true); 
            },
            [self](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                            for(card* c: allyTeam.getCards()){
                                c->setInvulnerable(false);
                                (*self)->setInvulnerable(false);
                            }
                        } 
        );

        *self = ch;
        character_pool.emplace("First Daughter of Loras", ch);
    }


    {

        character_pool.emplace("Queen of Kangreya", new Character("Loras", 1000, 55, 5, 1, 6, [](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                    for(card* c: enemyTeam.getCards()){c->setStunned(true);}        
        },
    [](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                            for(card* c: enemyTeam.getCards()){c->setStunned(false);}
                        }));
    }

    {
        auto knife_counter = std::make_shared<int>(0);
        auto hit_by_knife = std::make_shared<std::vector<std::pair<IDamageable*, int>>>();

        character_pool.emplace("Knife Master", new Character("Kont", 1100, 40, 1, 0, 3, [knife_counter, hit_by_knife](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                            target->takeDamage(5);
                            bool found = false;
                            (*knife_counter)++;
                            for(auto& t : *hit_by_knife){
                                if(t.first==target){
                                    t.second++;
                                    found == true;
                                }}
                                if(!found){hit_by_knife->push_back({target, 1});}

                            if((*knife_counter) == 5){
                                for(auto& t : *hit_by_knife){
                                    t.first->takeDamage(t.second * 10);
                                    hit_by_knife->clear();
                                }
                            }
                        },
                    [knife_counter, hit_by_knife](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                    }));
    }


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
                std::cout << "This ability only can be used on a card" << std::endl;
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

    // [DEGISIKLIK] Kart dagitma dongusu duzeltildi:
    // Onceki halde "removeTopCard()" cagrildiktan SONRA getTopCard()->setOwner(...) yapiliyordu;
    // bu yuzden owner yanlis karta atanıyor ve deste bosaldiginda nullptr->setOwner ile cokuyordu.
    for (int i = 0; i < 30; ++i) {
        if (!master_deck.isEmpty()) {
            card* c1 = master_deck.getTopCard();
            c1->setOwner(&player1);
            player1.addCardToPlayerDeck(c1);
            master_deck.removeTopCard();
        }
        if (!master_deck.isEmpty()) {
            card* c2 = master_deck.getTopCard();
            c2->setOwner(&player2);
            player2.addCardToPlayerDeck(c2);
            master_deck.removeTopCard();
        }
    }

    player1.shuffleDeck();
    player2.shuffleDeck();

    std::cout << "--- Desteler Hazirlandi ve Karistirildi (Her oyuncuda 30 kart var) ---\n" << std::endl;
}

void gameManager::playTurn(){
    std::cout << "--------------------------\n" << std::endl;
    std::cout << "--- Card Drawing Phase ---" << std::endl;

    player1.drawCard();
    player2.drawCard();

    card* p1_card = player1.selectCard();
    if(p1_card != nullptr){
        std::cout << player1.getName() << "is playing..." << std::endl;
        player1.playCard(p1_card, battle_field);
        turn_counter++;
    }

    card* p2_card = player2.selectCard();
    if(p2_card != nullptr){
        std::cout << "\n" << player2.getName() << " is playing..." << std::endl;
        player2.playCard(p2_card, battle_field);
        turn_counter++;
    }

    // [DUZELTME] Karakter atanmamissa getCharacter() nullptr doner -> once kontrol et,
    // yoksa nullptr uzerinden setMana cagrilir ve program coker.
    if(player1.hasCharacter())
        player1.getCharacter()->setMana(player1.getCharacter()->getMana() + tour_counter);
    if(player2.hasCharacter())
        player2.getCharacter()->setMana(player2.getCharacter()->getMana() + tour_counter);

        executeEffects();

        tickCharacters(); // her tur sonunda: karakter cooldown'lari azalir + suresi dolan etkiler geri alinir
        tour_counter++;   // tam bir tur bitti
}

void gameManager::executeEffects(){
    // [DEGISIKLIK] Ic dongude iterator hic ilerlemiyordu (sonsuz dongu) ve erase sonrasi
    // iterator gecersizlesiyordu. Iterator artik erase'in donusuyle guvenli ilerliyor.
    for(card* c: battle_field.getCards()){
        auto& current_effects = c->getEffects();
        for(auto it = current_effects.begin(); it != current_effects.end(); ){
            if((turn_counter - it->second.playedTour) % 2 == 0){
                c->takeDamage(it->second.damage);
                it->second.duration--;
            }
            if(it->second.duration <= 0){
                it = current_effects.erase(it);
            }
            else{
                ++it;
            }
        }
    }
}

void gameManager::run() {
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
                std::cout << "1. Oyuncu ismi: "; std::cin >> n1; std::cin.ignore(256, '\n');
                std::cout << "2. Oyuncu ismi: "; std::cin >> n2; std::cin.ignore(256, '\n');
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

void gameManager::checkTargetHealth(IDamageable* target) {

    if (target == nullptr) {
        std::vector<card*> deads;
        for (card* c : battle_field.getCards()) {
            if (c->getHealth() <= 0) deads.push_back(c);
        }
        for (card* c : deads) {
            std::cout << c->getName() << " died!" << std::endl;
            battle_field.removeCardFromDeck(c);
            // [DEGISIKLIK] owner nullptr olabilir (orn. Turret'in sahibi yok) -> once kontrol
            if (c->getOwner()) c->getOwner()->getTeam().removeCardFromDeck(c);
        }
        return;
    }

    if (target->getHealth() <= 0) {
        card* deadCard = dynamic_cast<card*>(target);
        if (deadCard != nullptr) {
            std::cout << deadCard->getName() << " died!" << std::endl;
            battle_field.removeCardFromDeck(deadCard);
            // [DEGISIKLIK] owner nullptr guvenlik kontrolu
            if (deadCard->getOwner()) deadCard->getOwner()->getTeam().removeCardFromDeck(deadCard);
        }
    }
}

// gameManager karaktere DOGRUDAN dokunmaz; her sey Player uzerinden gecer.
// user  = yetenegi kullanan oyuncu (karakteri onun icinde, allyTeam = user.getTeam())
// enemy = rakip oyuncu (enemyTeam = enemy.getTeam())
void gameManager::useCharacterAbility(Player& user, Player& enemy, IDamageable* target){
    if(!user.hasCharacter()){
        std::cout << user.getName() << " icin secili karakter yok." << std::endl;
        return;
    }

    // Player kendi karakterini kullanir; cooldown doluysa false doner.
    // (Eski degerleri kaydetme isini karakterin apply lambda'si kendi yapiyor.)
    if(!user.useCharacterAbility(target, battle_field, enemy.getTeam())){
        std::cout << user.getName() << "'in karakteri su an cooldownda ("
                  << user.getCharacter()->getcurrentCooldown() << " tur kaldi)" << std::endl;
        return;
    }

    // Sureli etkiyse sadece NE ZAMAN geri alinacagini kaydet; NE geri alinacagini lambda biliyor.
    const int duration = user.getCharacterAbilityDuration();
    if(duration > 0){
        timed_abilities.push_back(TimedAbility{ &user, &enemy, tour_counter + duration, target });
    }
}

void gameManager::tickCharacters(){
    // 1) Karakter cooldown'larini oyuncular uzerinden azalt.
    player1.tickCharacterCooldown();
    player2.tickCharacterCooldown();

    // 2) Suresi dolan etkileri karakterin kendi revert lambda'si geri alir.
    //    Lambda, apply sirasinda kaydettigi listeden calistigi icin canli takimlari
    //    kullanmaz -> sonradan takima katilan kartlar etkilenmez. Yine de imza geregi
    //    takimlari geciyoruz (lambda isterse kullanabilir).
    for(auto it = timed_abilities.begin(); it != timed_abilities.end(); ){
        if(tour_counter >= it->expireTurn){
            it->source->revertCharacterAbility(it->target, battle_field,
                                               it->source->getTeam(), it->enemy->getTeam());
            it = timed_abilities.erase(it);
        } else {
            ++it;
        }
    }
}
