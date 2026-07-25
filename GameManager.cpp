#include "GameManager.h"
#include "FantasyCard.h"
#include "TechCard.h"
#include "SeaSonCard.h"
#include "TargetSelection.h"
#include"Effect.h"
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <memory>    // std::shared_ptr, std::make_shared (yetenek lambda'larinin ortak state'i icin)
#include <utility>   // std::pair

// NOT: TimedAbility yapisi artik GameManager.h icinde tanimli (buradaki kopyasi kaldirildi,
// yoksa ayni ceviri biriminde "redefinition" hatasi olurdu).

void gameManager::setPlayers(std::string player1_name, std::string player2_name){
    player1 = Player(player1_name);
    player2 = Player(player2_name);
}

void gameManager::startGame() {

    // [YENI] YENIDEN BASLATMA TEMIZLIGI
    // "Ana menuye don -> yeni oyun" ile startGame ikinci kez cagrilabildigi icin,
    // onceki oyundan kalan her sey temizlenmeli. Aksi halde:
    //   - master_deck.deck cleanmezse kartlar 60 -> 120 diye birikir,
    //   - character_pool.emplace ayni anahtari EZMEZ -> eski (cooldown'lu) karakterler kalir
    //     ve yeni new'lenenler sizar.
    for(auto& kv : character_pool){ delete kv.second; }   // eski karakterleri sil (sizinti olmasin)
    character_pool.clear();
    master_deck.clear();
    battle_field.clear();
    timed_abilities.clear();
    timed_card_abilities.clear();
    all_cards_snapshot.clear();
    turn_counter = 0;
    tour_counter = 0;
    isRunning = true;

    //===================
    //CHARACTER SELECTİON
    //===================

    // Her karakterin KENDI 'originals' listesi olmali -> her biri ayri kapsamda (blok icinde)
    // olusturuluyor. apply ve revert lambda'lari ayni shared_ptr'i yakaladigi icin ayni listeyi
    // paylasirlar: apply eski degeri kaydeder, revert birebir geri yazar.
    {
        auto originals = std::make_shared<std::vector<std::pair<card*, int>>>();

        character_pool.emplace("Hope of the Algeron", new Character(
            "Knight of the Light", 150, 10, 3, 2, 3,

            // APPLY: once eski dmgHit'i kaydet, sonra degistir
            [originals](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                originals->clear();
                for(card* c : allyTeam.getCards()){
                    originals->push_back({ c, c->getDmgHit() });   // eski deger
                    c->takeDamage(nullptr, -10);                   // iyilestirme (karakter kaynakli, attacker=null)
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
            "Arrival of the Throne", 130, 13, 4, 2, 5,

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
            "Princess of Kangreya", 90, 18, 5, 1, 7, [self](IDamageable* target, Deck<card*>& battlefield,
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

        character_pool.emplace("Queen of Kangreya", new Character("Loras", 100, 15, 5, 1, 6, [](IDamageable* target, Deck<card*>& battlefield,
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

        character_pool.emplace("Knife Master", new Character("Kont", 110, 13, 1, 0, 1, [knife_counter, hit_by_knife](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                            target->takeDamage(nullptr, 2);
                            bool found = false;
                            (*knife_counter)++;
                            for(auto& t : *hit_by_knife){
                                if(t.first==target){
                                    t.second++;
                                    found = true;
                                }}
                                if(!found){hit_by_knife->push_back({target, 1});}

                            if((*knife_counter) == 5){
                                for(auto& t : *hit_by_knife){
                                    t.first->takeDamage(nullptr, t.second * 5);
                                }
                                hit_by_knife->clear();
                            }
                        },
                    [knife_counter, hit_by_knife](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                    }));
    }

    {
        auto protected_cards = std::make_shared<std::queue<card*>>();
        
        character_pool.emplace("Blessed of the Twins", new Character("Octopus", 115, 12, 1, 4, 2, [protected_cards](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                            if(card* tc = dynamic_cast<card*>(target)){   // ayri card* degisken (target'i ezme)
                                tc->setReflectionMarked(true);
                                protected_cards->emplace(tc);
                            }
                            if(protected_cards->size() > 3){

                                protected_cards->front()->setReflectionMarked(false);
                                protected_cards->pop();
                            }
        },
    [protected_cards](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                            protected_cards->front()->setReflectionMarked(false);
                            protected_cards->pop();
                        }));
    }

    character_pool.emplace("Trickster", new Character("Bad Omen", 105, 15, 3, 4, 4, [](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                            
        },
    [](IDamageable* target, Deck<card*>& battlefield,
                        Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                            
    }));

    // ==========================================
    // [YENI] KARAKTER YETENEKLERININ HEDEF TARAFI
    // NONE  -> hedef secilmez (lambda allyTeam/enemyTeam uzerinden toplu calisir); GUI'de USE butonu cikar
    // ALLY  -> sadece kendi tarafi        ENEMY -> sadece rakip taraf (varsayilan)
    // Yeni karakter eklersen yalnizca buraya bir satir yazman yeterli.
    // ==========================================
    character_pool.at("Hope of the Algeron")     ->setTargetSide(TargetSide::NONE); // tum takimi iyilestirir + hasar buffi
    character_pool.at("First Prince of Kangreya")->setTargetSide(TargetSide::NONE); // tum rakip kartlarin hasarini yariya boler
    character_pool.at("First Daughter of Loras") ->setTargetSide(TargetSide::NONE); // tum takimi + kendini dokunulmaz yapar
    character_pool.at("Queen of Kangreya")       ->setTargetSide(TargetSide::NONE); // tum rakip kartlari sersemletir
    character_pool.at("Blessed of the Twins")    ->setTargetSide(TargetSide::ALLY); // Octopus: dost karta reflection isareti
    // "Knife Master" -> varsayilan ENEMY (tek hedefe bicak atar)

    // ==========================================
    // 20 FANTASY CARDS (With Lambda Abilities)
    // ==========================================
    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::FIRE, fantasyCard::FantasyType::DRAGON, "Flame Dragon", 7, 7, "Breath", 25, card::DmgType::ICE, card::DmgType::FIRE, 0, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Set the battlefield on fire!" << std::endl;
            for(card* c: pitch.getCards()) {
                c->takeDamage(self, 7);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){

        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::FIRE, fantasyCard::FantasyType::DRAGON, "Red Dragon", 6, 5, "Ember", 30, card::DmgType::ICE, card::DmgType::FIRE, 0, 4,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Shot a critical ember!" << std::endl;
            if(target != nullptr){
            target->takeDamage(self, 17);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::ICE, fantasyCard::FantasyType::HEALER, "Ice Healer", 3, 5, "Polar", 8, card::DmgType::FIRE, card::DmgType::ICE, 0, 2,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Healed ally cards by freezing them!" << std::endl;
           for(card* c: allyTeam.getCards()) {
                c->takeDamage(self, -3);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::ICE, fantasyCard::FantasyType::HEALER, "Frost Guardian", 3, 4, "Ice Shield", 15, card::DmgType::FIRE, card::DmgType::ICE, 0, 2,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Granted ice shields to an ally!" << std::endl;
            if(target != nullptr){
            target->takeDamage(self, -10);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::SLICE, fantasyCard::FantasyType::SOLDIER, "Cruel Soldier", 4, 7, "Dance", 9, card::DmgType::MAGIC, card::DmgType::NORMAL, 1, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Sword speed doubled!" << std::endl;
            self->updateDmgHit(self->getDmgHit()*2);
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            self->updateDmgHit(7);
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::SLICE, fantasyCard::FantasyType::SOLDIER, "Dawn Knight", 4, 6, "Strike of justice", 15, card::DmgType::DARKNESS, card::DmgType::LIGHT, 0, 4,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Struck with a strike of justice!" << std::endl;
             if(target != nullptr){
                target->takeDamage(self, 15);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        3, card::DmgType::MAGIC, fantasyCard::FantasyType::MAGICIAN, "Ancient Mage", 6, 7, "Meteor", 12, card::DmgType::ANTIMAGIC, card::DmgType::MAGIC, 0, 5,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Rained meteors from the sky!" << std::endl;
            for(card* c: enemyTeam.getCards()) {
                c->takeDamage(self, 5);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    {
        auto savedTarget = std::make_shared<card*>(nullptr);
        auto amount = std::make_shared<int>();

    master_deck.addCardToDeck(new fantasyCard(
        3, card::DmgType::MAGIC, fantasyCard::FantasyType::MAGICIAN, "Sky Mage", 5, 7, "Magic breaker", 10, card::DmgType::ANTIMAGIC, card::DmgType::WIND, 2, 3,
        [savedTarget, amount](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Broke magic resistance!" << std::endl;
            if(card* tempTarget = dynamic_cast<card*>(target)){
                (*savedTarget) = tempTarget;
                (*amount) = (*savedTarget)->getDmgHit();
                (*savedTarget)->setDmgHit((*savedTarget)->getDmgHit()/2);
            }
        },
        [savedTarget, amount](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            if(*savedTarget){
               (*savedTarget)->setDmgHit(*amount);
            }
        }
    ));
    }
    {
        auto savedTarget = std::make_shared<card*>(nullptr);
    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::NATURE, fantasyCard::FantasyType::SOLDIER, "Forest Guardian", 5, 6, "Vine Trap", 16, card::DmgType::FIRE, card::DmgType::NATURE, 1, 4,
        [savedTarget](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Rooted enemies with vines!" << std::endl;
            if(card* tempTarget = dynamic_cast<card*>(target)){
                (*savedTarget) = tempTarget;
                tempTarget->setStunned(true);
            }
        },
        [savedTarget](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            (*savedTarget)->setStunned(false);
        }
    ));
    }

    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::NATURE, fantasyCard::FantasyType::HEALER, "Dryad", 2, 4, "Blessing", 8, card::DmgType::FIRE, card::DmgType::NATURE, 0, 1,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Called upon Mother Nature's blessing!" << std::endl;
            if(target != nullptr){
                target->takeDamage(self, -5);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::LIGHTNING, fantasyCard::FantasyType::DRAGON, "Storm Dragon", 7, 7, "Lightning Breath", 25, card::DmgType::MAGIC, card::DmgType::LIGHTNING, 0, 6,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Created a hurricane!" << std::endl;
            auto targets = targeting::selectTargets(enemyTeam, 3);
            int dmg = 12;
            for(card* c : targets){ c->takeDamage(self, dmg); dmg/= 2;}
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        3, card::DmgType::DARKNESS, fantasyCard::FantasyType::MAGICIAN, "Dark Mage", 6, 10, "transfusion", 12, card::DmgType::LIGHT, card::DmgType::DARKNESS, 0, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Cast life drain!" << std::endl;
            if(target){
                target->takeDamage(self, 15);
            }
            if(self){
                self->takeDamage(self, -6);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

{
    auto original = std::make_shared<card*>();
    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::LIGHT, fantasyCard::FantasyType::HEALER, "Light Priest", 4, 5, "Blinding Light", 10, card::DmgType::DARKNESS, card::DmgType::LIGHT, 1, 5,
        [original](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Blinded a card with light!" << std::endl;
            card* tempTarget = dynamic_cast<card*>(target);
            (*original) = tempTarget;
            tempTarget->setStunned(true);
        },
        [original](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            (*original)->setStunned(false);
        }
    ));
}

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::NORMAL, fantasyCard::FantasyType::SOLDIER, "Village Vanguard", 1, 4, "headbutt", 13, card::DmgType::MAGIC, card::DmgType::NORMAL, 0, 1,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Performed a simple but effective headbutt!" << std::endl;
            if(target){
                target->takeDamage(self, 7);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::ANTIMAGIC, fantasyCard::FantasyType::MAGICIAN, "Spellbreaker", 5, 2, "Cleanse", 10, card::DmgType::NORMAL, card::DmgType::MAGIC, 0, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Cleared all spells in the area!" << std::endl;
            for(card* c: pitch.getCards()){
                c->getEffects().clear();
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::FIRE, fantasyCard::FantasyType::SOLDIER, "Ember Guardian", 3, 6, "Ember", 13, card::DmgType::ICE, card::DmgType::FIRE, 2, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Infused weapon with fire!" << std::endl;
            self->updateDmgHit(self->getDmgHit() * 2);
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            self->updateDmgHit(self->getDmgHit()/2);
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        3, card::DmgType::ICE, fantasyCard::FantasyType::MAGICIAN, "Cryomancer", 5, 7, "Blizzard", 15, card::DmgType::FIRE, card::DmgType::ICE, 0, 4,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Started a blizzard!" << std::endl;
            for(card* c: pitch.getCards()){
                c->takeDamage(self, 2);
                Effect effect = {self->getDamageType(), 2,4,0};
                c->addEffect(effect);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        2, card::DmgType::WIND, fantasyCard::FantasyType::HEALER, "Wind Healer", 3, 2, "Wind Cure", 8, card::DmgType::MAGIC, card::DmgType::WIND, 0, 2,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Cleared poison with wind!" << std::endl;
            target->takeDamage(self, -5);
            if(card* tempTarget = dynamic_cast<card*>(target)){
                tempTarget->getEffects().clear();
            }

            if(Character* tempTarget = dynamic_cast<Character*>(target)){
                tempTarget->getEffects().clear();
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::DARKNESS, fantasyCard::FantasyType::DRAGON, "Shadow Dragon", 8, 8, "Dark Breath", 25, card::DmgType::LIGHT, card::DmgType::DARKNESS, 0, 4,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Cast dark fear!" << std::endl;
           if(card* tempTarget = dynamic_cast<card*>(target)){
            tempTarget->takeDamage(self, 10);
            Effect effect = {self->getDamageType(), 2,2,0};
            tempTarget->addEffect(effect);
           }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        1, card::DmgType::NORMAL, fantasyCard::FantasyType::SOLDIER, "Barbarian", 4, 4, "Rage", 10, card::DmgType::MAGIC, card::DmgType::SLICE, 2, 2,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Entered rage mode!" << std::endl;
            if(self){
                self->updateDmgHit(self->getDmgHit()*2);
                self->setDamageType(SLICE);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            if(self){
                self->updateDmgHit(self->getDmgHit()/2);
                self->setDamageType(NORMAL);
            }
        }
    ));

    master_deck.addCardToDeck(new fantasyCard(
        3, card::DmgType::DARKNESS, fantasyCard::FantasyType::MAGICIAN, "Necromancer", 1, 4, "Revive", 16, card::DmgType::LIGHT, card::DmgType::DARKNESS, 0, 6,
        [&](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            self->getOwner()->getGraveyard().top()->setHealth(self->getOwner()->getGraveyard().top()->getMaxHealth()/2);
            battle_field.addCardToDeck(self->getOwner()->getGraveyard().top());
            self->getOwner()->getGraveyard().pop();
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            
        }
    ));

    // ==========================================
    // 20 TECH CARDS (With Lambda Abilities)
    // ==========================================
    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::LIGHT, TechCard::TechnoType::ROBOT, "Cyber Observer", 5, 4, "Laser", 15, card::DmgType::LIGHTNING, card::DmgType::LIGHT, 0, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Locked onto the target!" << std::endl;
            auto targets = targeting::selectTargets(enemyTeam, 3);
            for(card* c : targets){ c->takeDamage(self, 3);}
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::LIGHT, TechCard::TechnoType::ROBOT, "High Tech Robot", 5, 5, "Plasma Shot", 20, card::DmgType::LIGHTNING, card::DmgType::LIGHT, 0, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Fired a plasma ball!" << std::endl;
            if(target){
                target->takeDamage(self, 10);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::NORMAL, TechCard::TechnoType::MECHANIC, "Chief Engineer", 3, 2, "Turret", 10, card::DmgType::MAGIC, card::DmgType::NORMAL, 0, 1,
        [&](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
         std::cout << "-> Ability: Deployed an automatic turret!" << std::endl;
         card* turret = new TechCard(1 ,card::DmgType::NORMAL, TechCard::TechnoType::MECHANIC, "Turret", 0, 2, "NONE", 5, card::DmgType::MAGIC, card::DmgType::NORMAL, 0, 0,
             [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){},[](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){});
            battle_field.addCardToDeck(turret);
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

{
    auto originalTarget = std::make_shared<IDamageable*>(nullptr);

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::NORMAL, TechCard::TechnoType::MECHANIC, "Blacksmith Robot", 3, 2, "Repairment", 9, card::DmgType::MAGIC, card::DmgType::SLICE, 2, 3,
        [originalTarget](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Repaired armor!" << std::endl;
            target->addArmor(5);
            (*originalTarget) = target;
        },
        [originalTarget](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            (*originalTarget)->addArmor(-5);
        }
    ));
}

    master_deck.addCardToDeck(new TechCard(
        2, card::DmgType::ANTIMAGIC, TechCard::TechnoType::SOLDIER, "Pulse Guardian", 5, 4, "Shield", 10, card::DmgType::SLICE, card::DmgType::MAGIC, 0, 4,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability:  Absorb electromagnetic waves!" << std::endl;
            if(self){
                self->addArmor(2);
            }
            auto& effects = self->getEffects();
            for(int i = 0 ;i<effects.size();i++){
                Effect e = effects[i];
            if(e.effectType == LIGHTNING){
                self->addArmor(e.duration * e.damage);
            }
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        2, card::DmgType::ANTIMAGIC, TechCard::TechnoType::SOLDIER, "Cyber Protector", 4, 6, "firewall shield", 9, card::DmgType::SLICE, card::DmgType::MAGIC, 4, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Activated firewall shield!" << std::endl;
            self->addArmor(8);
            Effect effect = {FIRE, 3,4,6};
            self->addEffect(effect);
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            self->addArmor(-self->getArmor());
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::DARKNESS, TechCard::TechnoType::TITAN, "Doomsday Tank", 8, 7, "Shock", 15, card::DmgType::LIGHT, card::DmgType::DARKNESS, 0, 5,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Fired a nuclear missile!" << std::endl;
            for(card* c : pitch.getCards()){c->takeDamage(self, 7);}
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::DARKNESS, TechCard::TechnoType::TITAN, "Goliath V2", 8, 7, "Pierceing Shot", 30, card::DmgType::LIGHT, card::DmgType::DARKNESS, 0, 5,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Pierced through with a railgun!" << std::endl;
            if(target){
                target->takeDamage(self, 15);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

{
    auto originalTarget = std::make_shared<card*>();

    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::WIND, TechCard::TechnoType::ROBOT, "Light Drone", 2, 2, "Wave", 1, card::DmgType::FIRE, card::DmgType::WIND, 1, 2,
        [originalTarget](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Activated the signal jammer!" << std::endl;
            (*originalTarget) = dynamic_cast<card*>(target);
            if(originalTarget){
                (*originalTarget)->updateDmgHit((*originalTarget)->getDmgHit()/2);
            }
            else{
                std::cout << "This ability only can be used on a card" << std::endl;
            }
        },
        [originalTarget](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            (*originalTarget)->updateDmgHit((*originalTarget)->getDmgHit()*2);
        }
    ));
}

    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::WIND, TechCard::TechnoType::ROBOT, "Swift Jet", 3, 3, "Wave", 12, card::DmgType::ICE, card::DmgType::WIND, 0, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Caused a supersonic blast!" << std::endl;
            if(target){
                target->takeDamage(self, 7);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        2, card::DmgType::FIRE, TechCard::TechnoType::MECHANIC, "Pyro-Technician", 4, 4, "Explosion", 9, card::DmgType::ICE, card::DmgType::FIRE, 0, 4,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Launched a napalm bomb!" << std::endl;
            for(card* c:enemyTeam.getCards()){
                c->takeDamage(self, 5);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

{
auto originalTarget = std::make_shared<IDamageable*>();

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::ICE, TechCard::TechnoType::SOLDIER, "Freezing Android", 4, 4, "Nitrogen", 10, card::DmgType::FIRE, card::DmgType::ICE, 1, 3,
        [originalTarget](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Sprayed liquid nitrogen!" << std::endl;
            (*originalTarget) = target;
            target->setStunned(true);
            target->setInvulnerable(true);
        },
        [originalTarget](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            (*originalTarget)->setStunned(false);
            (*originalTarget)->setInvulnerable(false);
        }
    ));
}

    master_deck.addCardToDeck(new TechCard(
        2, card::DmgType::NATURE, TechCard::TechnoType::ROBOT, "Bio-Mechanical Golem", 5, 3, "Organic Energy", 15, card::DmgType::FIRE, card::DmgType::NATURE, 2, 4,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Activated organic batteries!" << std::endl;
            if(self){
                self->updateDmgHit(self->getDmgHit()*3);
                self->addArmor(3);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            self->updateDmgHit(self->getDmgHit()/3);
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::MAGIC, TechCard::TechnoType::TITAN, "Rune Circuit", 7, 4, "Rune Buff", 12, card::DmgType::ANTIMAGIC, card::DmgType::MAGIC, 0, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Triggered a magical overload!" << std::endl;

            auto targets = targeting::selectTargets(allyTeam, 3);

            for(card* c: targets){
                c->updateDmgHit(c->getDmgHit() + 1);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::SLICE, TechCard::TechnoType::SOLDIER, "Saw Robot", 3, 4, "Slice and Dice", 8, card::DmgType::MAGIC, card::DmgType::NORMAL,0,4,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Overclocked the blades!" << std::endl;
            Effect bleed = {SLICE,2,4,0};
            if(card* tempTarget = dynamic_cast<card*>(target)){
                tempTarget->takeDamage(self, 2);
                tempTarget->addEffect(bleed);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::LIGHT, TechCard::TechnoType::ROBOT, "Quantum Reflector", 5, 2, "Laser", 6, card::DmgType::DARKNESS, card::DmgType::LIGHT, 2 , 4,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Bent light to become invisible!" << std::endl;
            if(self->isInvulnerable() == true){return;}

            self->setInvulnerable(true);
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            if(self->isDead()){return;}
            self->setInvulnerable(false);
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        1, card::DmgType::NORMAL, TechCard::TechnoType::MECHANIC, "Repair Unit", 2, 1, "Nanorobot treatment", 7, card::DmgType::MAGIC, card::DmgType::NORMAL, 0, 2,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Started repairs with nanobots!" << std::endl;
            for(card* c:allyTeam.getCards()){
                c->takeDamage(self, -2);
            }
        },
           [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }     
   ));

{
    auto originals = std::make_shared<std::vector<card*>>();

    master_deck.addCardToDeck(new TechCard(
        2, card::DmgType::DARKNESS, TechCard::TechnoType::TITAN, "Dark Matter Armor", 9, 0, "black shield", 30, card::DmgType::LIGHT, card::DmgType::DARKNESS, 1, 9,
        [originals](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Gave his Dark Energy to Allies" << std::endl;
            auto targets = targeting::selectTargets(allyTeam, 3);
            (*originals) = targets;
            for(card* c: targets){c->addArmor(self->getHealth()/3); c->setInvulnerable(true);}
            self->takeDamage(self, self->getHealth());
            getInstance().checkTargetHealth(self);
        },
        [originals](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            for(card* c : (*originals)){if(c->isDead()){continue;} c->setInvulnerable(false);}
        }
    ));
}

    master_deck.addCardToDeck(new TechCard(
        3, card::DmgType::WIND, TechCard::TechnoType::ROBOT, "Sonic Copter", 3, 2, "Breeze", 5, card::DmgType::ICE, card::DmgType::WIND, 0, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Blew away with propeller wind!" << std::endl;
            for(card* c: enemyTeam.getCards()){
                c->takeDamage(self, 1);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new TechCard(
        2, card::DmgType::SLICE, TechCard::TechnoType::SOLDIER, "Cybernetic Ninja", 4, 5, "Techno Slash", 8, card::DmgType::MAGIC, card::DmgType::NORMAL, 2, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Emitted a katana shockwave!" << std::endl;
            if(self){
                self->updateDmgHit(self->getDmgHit()*2);
                self->setDamageType(LIGHTNING);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            if(self->isDead()){return;}
            self->updateDmgHit(self->getDmgHit()/2);
            self->setDamageType(SLICE);
        }
    ));

    // ==========================================
    // 20 SEA SON CARDS (With Lambda Abilities)
    // ==========================================
    {
        auto originals = std::make_shared<std::vector<card*>>();

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::DARKNESS, SeaSonCard::SeaSonType::KRAKEN, "Nightmare Kraken", 9, 6, "Vortex", 30, card::DmgType::LIGHT, card::DmgType::DARKNESS, 2, 6,
        [originals](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Cast a deep sea curse!" << std::endl;
            (*originals) = enemyTeam.getCards();
            for(card* c:enemyTeam.getCards()){
                c->takeDamage(self, 5);
                c->setMaxHealth(c->getMaxHealth() - 5);
            }
        },
        [originals](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            for(card* c: (*originals)){c->setMaxHealth(c->getMaxHealth() + 5);}
        }
    ));
}

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::DARKNESS, SeaSonCard::SeaSonType::KRAKEN, "Leviathan", 9, 7, "Tsunami", 35, card::DmgType::LIGHT, card::DmgType::DARKNESS, 0, 7,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Started a tsunami!" << std::endl;
            for(card* c: pitch.getCards()){
                c->takeDamage(self, 7);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::NATURE, SeaSonCard::SeaSonType::MEDIC, "Coral Healer", 3, 2, "Wave", 10, card::DmgType::FIRE, card::DmgType::NATURE, 0, 2,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Active coral healing!" << std::endl;
            target->takeDamage(self, -7);
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::NATURE, SeaSonCard::SeaSonType::MEDIC, "Mermaid", 3, 1, "Song of the sea", 11, card::DmgType::FIRE, card::DmgType::NATURE, 0, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Sang the current song!" << std::endl;
            for(card* c: allyTeam.getCards()){
                c->setMaxHealth(c->getMaxHealth() + 2);
                c->takeDamage(self, -2);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::SLICE, SeaSonCard::SeaSonType::PIRATE, "Blackbeard", 4, 4, "Boarding", 12, card::DmgType::MAGIC, card::DmgType::NORMAL, 0, 5,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Executed a boarding attack!" << std::endl;
            for(card* c: allyTeam.getCards()){
                int tempDmg = c->getDmgHit();
                c->setActive(true);
                c->updateDmgHit(c->getDmgHit()/4);
                c->hit(target);
                c->updateDmgHit(tempDmg);
                c->setActive(true);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::SLICE, SeaSonCard::SeaSonType::PIRATE, "Captain Barbarossa", 5, 5, "Blast", 10, card::DmgType::MAGIC, card::DmgType::NORMAL, 0, 4,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Subjected the fleet to cannon fire!" << std::endl;
            for(card* c : enemyTeam.getCards()){
                c->takeDamage(self, 3);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        3, card::DmgType::MAGIC, SeaSonCard::SeaSonType::ORACLE, "Sea Oracle", 5, 1, "Deluge", 7, card::DmgType::ANTIMAGIC, card::DmgType::MAGIC, 0, 5,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Saw the storm of the future!" << std::endl;
            if(Character* tempTarget = dynamic_cast<Character*>(target)){
                for(card* c :tempTarget->getOwner()->getHand()){
                    c->getCardInfo();
                }
        }
    },
    [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
    }
    ));

{
    auto tempTarget = std::make_shared<card*>(nullptr);
    // [DUZELTME] Geri alirken kartin ESKI sahibini bilmek gerekiyor; onceden saklanmiyordu,
    // revert 'c->setOwner((*tempTarget)->getOwner())' yaziyordu -> kart o an BIZE ait oldugu
    // icin bu bir NO-OP'tu ve kart karsi tarafa hic donmuyordu. Artik orijinal sahip saklaniyor.
    auto origOwner = std::make_shared<Player*>(nullptr);

    master_deck.addCardToDeck(new SeaSonCard(
        3, card::DmgType::MAGIC, SeaSonCard::SeaSonType::ORACLE, "Siren Song", 4, 1, "Deluge", 12, card::DmgType::ANTIMAGIC, card::DmgType::MAGIC, 2, 8,
        [tempTarget, origOwner](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Charmed the enemy !" << std::endl;
            if(card* tTarget = dynamic_cast<card*>(target)){(*tempTarget) = tTarget;}
            for(card* c: enemyTeam.getCards()){
                if(target == c){
                    (*origOwner) = c->getOwner();      // [YENI] eski sahibi sakla (geri donus icin)
                    c->setOwner(self->getOwner());
                    allyTeam.addCardToDeck(c);
                    enemyTeam.removeCardFromDeck(c);
                }
            }
        },
        [tempTarget, origOwner](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
                for(card* c: allyTeam.getCards()){
                if((*tempTarget) == c){
                    c->setOwner(*origOwner);           // [DUZELTME] ESKI sahibe geri ver (no-op degil)
                    enemyTeam.addCardToDeck(c);
                    allyTeam.removeCardFromDeck(c);
                }
            }
        }
    ));
}


{
    auto original = std::make_shared<IDamageable*>(nullptr);

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::ICE, SeaSonCard::SeaSonType::PIRATE, "Arctic Raider", 4, 4, "Anchor", 10, card::DmgType::FIRE, card::DmgType::ICE, 1, 2,
        [original](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Fired a frozen harpoon!" << std::endl;
            (*original) = target;
            target->takeDamage(self, 4);
            target->setStunned(true);
            target->setInvulnerable(true);
        },
        [original](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            (*original)->setStunned(false);
            (*original)->setInvulnerable(false);
        }
    ));
}

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::ICE, SeaSonCard::SeaSonType::KRAKEN, "Frozen Giant Octopus", 7, 7, "Deep frozen", 25, card::DmgType::FIRE, card::DmgType::ICE, 0, 6,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Threw an ice sheet with tentacles!" << std::endl;
            target->takeDamage(self, 20);
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::FIRE, SeaSonCard::SeaSonType::PIRATE, "Pirate Cannoneer", 4, 2, "Blast", 9, card::DmgType::ICE, card::DmgType::FIRE, 0, 5,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Used Greek fire!" << std::endl;
            for(card* c: enemyTeam.getCards()){
                Effect effect ={FIRE, 2,3,0};
                c->addEffect(effect);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        3, card::DmgType::WIND, SeaSonCard::SeaSonType::ORACLE, "Stormbringer", 5, 2, "Deluge", 7, card::DmgType::MAGIC, card::DmgType::WIND, 0, 2,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Blew a capsizing wind!" << std::endl;
            for(card* c: enemyTeam.getCards()){c->takeDamage(self, 2);}
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
    }
    ));


{
    auto original = std::make_shared<IDamageable*>(nullptr);

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::LIGHT, SeaSonCard::SeaSonType::MEDIC, "Sea Glow Sprite", 3, 3, "Night Glow", 8, card::DmgType::DARKNESS, card::DmgType::LIGHT, 1, 3,
        [original](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            (*original) = target;
            std::cout << "-> Ability: Blinded with a night glow!" << std::endl;
            target->setStunned(true);
        },
        [original](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            (*original)->setStunned(false);
        }
    ));
}

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::NORMAL, SeaSonCard::SeaSonType::PIRATE, "Ship Crew", 1, 2, "Drunken sailor", 5, card::DmgType::MAGIC, card::DmgType::NORMAL, 0, 1,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Threw a rum bottle!" << std::endl;
            target->takeDamage(self, 5);
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::ANTIMAGIC, SeaSonCard::SeaSonType::ORACLE, "Deepwater Guardian", 6, 6, "Deluge", 14, card::DmgType::NORMAL, card::DmgType::MAGIC, 0, 5,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Will drown the magic shield in a vortex!" << std::endl;
            for(card* c : allyTeam.getCards()){c->addArmor(4);}
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::SLICE, SeaSonCard::SeaSonType::KRAKEN, "Razor Tentacle", 6, 5, "Triple Slash", 30, card::DmgType::MAGIC, card::DmgType::NORMAL, 0, 4,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Sharp tentacle slash!" << std::endl;
            auto targets = targeting::selectTargets(enemyTeam, 3);
            Effect effect = {SLICE, 2,3,0};
            for(card* c: targets){c->takeDamage(self, 6); c->addEffect(effect);}
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){  
            
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::ICE, SeaSonCard::SeaSonType::MEDIC, "Glacial Savior", 4, 1, "Healing Wave", 12, card::DmgType::FIRE, card::DmgType::ICE, 0, 3,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Performed cold water shock therapy!" << std::endl;
            for(card* c: allyTeam.getCards()){
                c->takeDamage(self, -3);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    {
        auto original = std::make_shared<IDamageable*>(nullptr);
    master_deck.addCardToDeck(new SeaSonCard(
        3, card::DmgType::DARKNESS, SeaSonCard::SeaSonType::ORACLE, "Abyss Witch", 5, 4, "Darkness of the deep sea", 9, card::DmgType::LIGHT, card::DmgType::DARKNESS, 1, 5,
        [original](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Mariana trench darkness!" << std::endl;
            (*original) = target;
            target->takeDamage(self, 4);
            target->setStunned(true);
            
        },
        [original](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            (*original)->setStunned(false);
        }
    ));
    }

    master_deck.addCardToDeck(new SeaSonCard(
        1, card::DmgType::FIRE, SeaSonCard::SeaSonType::PIRATE, "Lava Captain", 5, 4, "Lava shot", 11, card::DmgType::ICE, card::DmgType::FIRE, 0, 5,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Volcanic pirate attack!" << std::endl;
            if(card* tempTarget = dynamic_cast<card*>(target)){
            target->takeDamage(self, 6);
            Effect effect = {FIRE, 2, 3, 0};
            tempTarget->addEffect(effect);
            }
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    master_deck.addCardToDeck(new SeaSonCard(
        2, card::DmgType::WIND, SeaSonCard::SeaSonType::MEDIC, "Monsoon Healer", 3, 2, "Wind therapy", 6, card::DmgType::MAGIC, card::DmgType::WIND, 0, 2,
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam) {
            std::cout << "-> Ability: Humid storm healing!" << std::endl;
            target->takeDamage(self, 3);
            target->addArmor(3);
        },
        [](IDamageable* target, Deck<card*>& pitch, card* self, Deck<card*>& allyTeam, Deck<card*>& enemyTeam){
            
        }
    ));

    // ==========================================
    // [YENI] KART YETENEKLERININ HEDEF TARAFI
    //
    // NONE -> HEDEFSIZ. Yetenek lambda'si 'target' parametresine hic dokunmuyor; etkisini
    //         pitch / allyTeam / enemyTeam / self uzerinden yapiyor (alan hasari, tum takimi
    //         iyilestirme, kendine buff...). Hedef secme adimi atlanir, arayuzde USE butonu cikar.
    // ALLY -> tek hedefli ama SADECE kendi takimina (heal / armor).
    // Listede olmayan her kart varsayilan ENEMY'de kalir (tek hedefe hasar verenler).
    //
    // Kural basit: lambda'da 'target' kullanilmiyorsa NONE, kullaniliyor ve faydaliysa ALLY.
    // Bir kartin yetenegini degistirirsen sadece bu iki listeyi guncelle.
    // NOT: Normal saldiri (hit) bundan etkilenmez, o her zaman sadece rakibe vurur.
    // ==========================================
    {
        // --- Hedef almayan (AoE / toplu / kendine) yetenekler ---
        static const std::vector<std::string> no_target_cards = {
            // alan hasari / tum rakip takima vuranlar (tek cagrida hepsi)
            "Flame Dragon", "Ancient Mage", "Cryomancer", "Doomsday Tank", "Pyro-Technician",
            "Sonic Copter", "Nightmare Kraken", "Leviathan", "Captain Barbarossa",
            "Pirate Cannoneer", "Stormbringer",
            "Spellbreaker",
            // tum dost takima iyilestirme / kalkan / buff
            "Ice Healer", "Repair Unit", "Mermaid", "Deepwater Guardian", "Glacial Savior",
            // sadece kendine buff atanlar (hedef gerekmiyor)
            "Cruel Soldier", "Barbarian", "Ember Guardian", "Pulse Guardian", "Cyber Protector",
            "Quantum Reflector", "Bio-Mechanical Golem", "Cybernetic Ninja"
        };
        // --- Tek hedefli ama sadece KENDI takimina kullanilabilenler ---
        static const std::vector<std::string> ally_target_cards = {
            "Frost Guardian",      // hedefe +10 can (buz kalkani)
            "Dryad",               // hedefe +5 can
            "Wind Healer",         // hedefe +5 can
            "Blacksmith Robot",    // hedefe +5 armor
            "Coral Healer",        // hedefe +7 can
            "Monsoon Healer"       // hedefe armor
        };
        // --- COK HEDEFLI: lambda icinde selectTargets(pool, N) cagirir; oyuncu N kart secer.
        //     side = secim havuzu (ALLY = kendi takimindan, ENEMY = rakip takimdan). ---
        struct MultiTag { const char* name; TargetSide side; int count; };
        static const std::vector<MultiTag> multi_target_cards = {
            {"Rune Circuit",      TargetSide::ALLY,  3},   // 3 dost karta +1 hasar
            {"Dark Matter Armor", TargetSide::ALLY,  3},   // 3 dost karta armor + dokunulmazlik
            {"Storm Dragon",      TargetSide::ENEMY, 3},   // 3 rakip karta azalan hasar
            {"Cyber Observer",    TargetSide::ENEMY, 3},   // 3 rakip karta hasar
            {"Razor Tentacle",    TargetSide::ENEMY, 3},   // 3 rakip karta hasar + kanama
        };
        for(card* c : master_deck.getCards()){
            for(const std::string& n : no_target_cards){
                if(c->getName() == n){ c->setTargetSide(TargetSide::NONE); break; }
            }
            for(const std::string& n : ally_target_cards){
                if(c->getName() == n){ c->setTargetSide(TargetSide::ALLY); break; }
            }
            for(const MultiTag& m : multi_target_cards){
                if(c->getName() == m.name){ c->setTargetSide(m.side); c->setTargetCount(m.count); break; }
            }
        }
    }

    master_deck.shuffle();

    // [YENI] Kart galerisi icin anlik kopya: dagitim BASLAMADAN once master_deck'teki
    // tum kart pointer'larini sakla. Dagitimdan sonra master_deck bosaldigi icin,
    // GUI'deki "tum kartlari goruntule" bu listeyi kullanir. Yeni kart eklersen burada gorunur.
    all_cards_snapshot.clear();
    for(card* c : master_deck.getCards()){
        all_cards_snapshot.push_back(c);
    }

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

    advanceTurn();   // tur sonu islemleri (cin icermez -> GUI de ayni fonksiyonu kullanir)
}

// Turu ilerleten, ETKILESIM ICERMEYEN kisim.
// playTurn (konsol) ve GUI'nin "Turu Bitir" dugmesi ayni mantigi kullansin diye ayrildi.
void gameManager::advanceTurn(){
    // [DUZELTME] Karakter atanmamissa getCharacter() nullptr doner -> once kontrol et,
    // yoksa nullptr uzerinden setMana cagrilir ve program coker.
    if(player1.hasCharacter())
        player1.getCharacter()->setMana(player1.getCharacter()->getMana() + tour_counter + 2);
    if(player2.hasCharacter())
        player2.getCharacter()->setMana(player2.getCharacter()->getMana() + tour_counter + 2);

    // [DUZELTME] Asagidaki uc satir yukaridaki if'in govdesiymis gibi girintilenmisti
    // (-Wmisleading-indentation). Aslinda her turda calismalari gerekiyor; girinti duzeltildi.
    executeEffects();

    tickCharacters(); // her tur sonunda: karakter cooldown'lari azalir + suresi dolan etkiler geri alinir
    tickCards();
        player1.setActionAndAbilityPoint(3, 1);
        player2.setActionAndAbilityPoint(3, 1);
        for(card* c : battle_field.getCards()){c->setActive(true);}
        tour_counter++;   // tam bir tur bitti
}

void gameManager::executeEffects(){
    // [DEGISIKLIK] Ic dongude iterator hic ilerlemiyordu (sonsuz dongu) ve erase sonrasi
    // iterator gecersizlesiyordu. Iterator artik erase'in donusuyle guvenli ilerliyor.
    for(card* c: battle_field.getCards()){
        auto& current_effects = c->getEffects();
        // [DUZELTME] Onceki dongude uc ayri hata vardi:
        //   1) 'Effect e = current_effects[i];' bir KOPYA aliyor, 'e.duration--' sadece kopyayi
        //      azaltiyordu -> depodaki sure hic dusmuyor, etki KALICI hale geliyordu (sonsuz zehir).
        //   2) erase(begin()+i) sonrasi i++ yapilinca erase edilenin YERINE gelen eleman atlaniyordu.
        //   3) her tur tik atmasi gerekirken '% 2' ile sadece bazi turlarda tik atiyordu.
        // Artik: her tur hasar uygulanir, sure depoda azalir, biten etki guvenli silinir.
        for(size_t i = 0; i < current_effects.size(); ){
            Effect& e = current_effects[i];   // REFERANS -> depoyu gercekten degistirir
            c->takeDamage(nullptr, e.damage);
            e.duration--;
            if(e.duration <= 0){
                current_effects.erase(current_effects.begin() + i);   // i'yi ARTIRMA (kayma)
            } else {
                ++i;
            }
        }
    }
}

void gameManager::run() {
    int choice;
    bool isStarted = false;   // [DUZELTME] 'true' idi -> "oyun baslatildi mi" bayragi anlamsizdi
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
                // [DUZELTME] 'break' if'in ICINDEydi: kosul saglanmazsa case 1 sessizce
                // case 2'ye dusuyordu (-Wimplicit-fallthrough) ve istenmeden playTurn() calisiyordu.
                // Ayrica isStarted 'true' baslatilip 'if(isStarted)' ile kontrol ediliyordu,
                // yani bayrak hicbir sey yapmiyordu; artik oyun bir kez baslatiliyor.
                if(!isStarted){
                    std::string n1, n2;
                    std::cout << "1. Oyuncu ismi: "; std::cin >> n1; std::cin.ignore(256, '\n');
                    std::cout << "2. Oyuncu ismi: "; std::cin >> n2; std::cin.ignore(256, '\n');
                    setPlayers(n1, n2);
                    startGame();
                    isStarted = true;
                }
                else{
                    std::cout << "Oyun zaten basladi." << std::endl;
                }
                break;
            }
            case 2:
                playTurn();
                break;
            case 3:
                endGame();
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
            c->getEffects().clear();
            if (c->getOwner()) c->getOwner()->getTeam().removeCardFromDeck(c);
            if(c->getOwner()) c->getOwner()->getGraveyard().push(c);
            c->setDead(true);
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
        // [DUZELTME] Eskiden sebep ne olursa olsun "cooldownda" yaziyordu; artik gercek sebebi soyluyor.
        Character* ch = user.getCharacter();
        if(user.getAbilityPoint() <= 0)
            std::cout << user.getName() << " already used a hero ability this turn." << std::endl;
        else if(ch->getcurrentCooldown() > 0)
            std::cout << user.getName() << "'s hero is on cooldown ("
                      << ch->getcurrentCooldown() << " turn(s) left)" << std::endl;
        else if(!ch->canAfford(ch->getAbilityCost()))
            std::cout << user.getName() << ": not enough mana for hero ability ("
                      << ch->getMana() << "/" << ch->getAbilityCost() << ")" << std::endl;
        else
            std::cout << user.getName() << ": hero ability unavailable." << std::endl;
        return;
    }

    // Sureli etkiyse sadece NE ZAMAN geri alinacagini kaydet; NE geri alinacagini lambda biliyor.
    const int duration = user.getCharacterAbilityDuration();
    if(duration > 0){
        // [DUZELTME] 'usedCard' alani hic verilmiyordu (-Wmissing-field-initializers).
        // Karakter yeteneginde kullanilan bir kart yok -> acikca nullptr yaziliyor.
        timed_abilities.push_back(TimedAbility{ &user, &enemy, tour_counter + duration, target, nullptr });
    }
}

void gameManager::useCardAbility(Player& user, Player& enemy, IDamageable* target, card* usedCard){
    if(usedCard){
            user.useCardUniqueAbility(target, battle_field, usedCard, user.getTeam(), enemy.getTeam());

            // [DUZELTME] Sureli kart etkileri HIC geri alinmiyordu: timed_card_abilities'e
            // ekleme yoktu, tickCards da bos listeyi geziyordu. Bu yuzden kart kaynakli
            // stun / invulnerable / vulnerability gibi bufflar KALICI olarak kaliyordu.
            // Artik (karakter yetenegindeki gibi) sure > 0 ise geri alma zamani kaydediliyor.
            // Geri NE alinacagini kartin kendi reversePower lambda'si biliyor (bos olabilir).
            const int duration = usedCard->getPowerDuration();
            if(duration > 0){
                timed_card_abilities.push_back(
                    TimedAbility{ &user, &enemy, tour_counter + duration, target, usedCard });
            }
    }
    else{
        return;
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
            // [DUZELTME] Cagri bozuktu: parantez erken kapaniyor, noktali virgul yok ve
            // argumanlar yarida kalmisti. revertCharacterAbility 4 parametre bekliyor:
            // (target, battlefield, allyTeam, enemyTeam) -> allyTeam = etkiyi kullanan oyuncunun takimi.
            it->source->revertCharacterAbility(it->target, battle_field,
                                               it->source->getTeam(), it->enemy->getTeam());
            it = timed_abilities.erase(it);
        } else {
            ++it;
        }
    }
}

void gameManager::tickCards(){

    for(auto it = timed_card_abilities.begin(); it!=timed_card_abilities.end();){
        if(tour_counter >= it->expireTurn){
            it->source->useCardReverseAbility(it->target, battle_field, it->usedCard,
             it->source->getTeam(), it->enemy->getTeam());

             it = timed_card_abilities.erase(it);
        }
        else{
            ++it;
        }
    }
}

void gameManager::endGame(){
    // [DUZELTME 1] 'isRunning == false;' idi -> ATAMA degil KARSILASTIRMA. Hicbir sey yapmiyordu.
    // [DUZELTME 2] '== 0' idi -> can eksiye dusebiliyor (takeDamage cikariyor), '<= 0' olmali.
    // [DUZELTME 3] getCharacter() nullptr olabilir -> once kontrol, yoksa cokerdi.
    Character* c1 = player1.getCharacter();
    Character* c2 = player2.getCharacter();
    if((c1 && c1->getHealth() <= 0) || (c2 && c2->getHealth() <= 0)){
        isRunning = false;
    }
}
