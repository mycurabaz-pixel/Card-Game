#ifndef DECK_H
#define DECK_H

#include <vector>
#include <random>
#include <algorithm> // for std::shuffle

// Deck bir template olduğu için tanımı tamamen header'da kalmalı.
template <typename T>
class Deck {

    private:

    std::vector<T> active_deck;
    std::vector<T> deck;

    public:

    void addCardToDeck(T item){
        deck.push_back(item);
    }

    void removeCardFromDeck(T target_card){
        for(auto it = deck.begin(); it != deck.end(); ++it){
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

    // [GUI ICIN EKLENDI] Destede kac kart kaldi (cekilebilir olanlar).
    // getCards() tum kartlari dondurur ve cekince azalmaz; bu ise azalir.
    int remaining() const { return (int)active_deck.size(); }

    const std::vector<T>& getCards() const {
        return deck;
    }

};

#endif // DECK_H
