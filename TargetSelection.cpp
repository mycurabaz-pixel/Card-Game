#include "TargetSelection.h"
#include <iostream>
#include <utility>

namespace targeting {

namespace {

// ---------------------------------------------------------------------------
// VARSAYILAN (KONSOL) CHOOSER
// GUI'ye gecince setTargetChooser(...) ile degistirilir; bu fonksiyon silinmez,
// sadece devre disi kalir. Yalnizca "hangi indeksler" sorusunu cevaplar --
// gecerlilik kontrolu selectTargets'ta yapildigi icin burasi sade kalabilir.
// ---------------------------------------------------------------------------
std::vector<int> consoleChooser(const std::vector<card*>& candidates, int count){
    std::vector<int> picked;

    while((int)picked.size() < count){
        std::cout << "\n--- Hedef sec (" << picked.size() + 1 << "/" << count << ") ---\n";
        for(size_t i = 0; i < candidates.size(); ++i){
            std::cout << i << ". " << candidates[i]->getName()
                      << "  (HP: " << candidates[i]->getHealth() << ")\n";
        }
        std::cout << "Seciminiz: ";

        int sel;
        if(!(std::cin >> sel)){                 // harf vb. girildi -> akis bozulur
            std::cin.clear();                   // hata bayragini temizle
            std::cin.ignore(256, '\n');         // bozuk girdiyi at (yoksa SONSUZ DONGU)
            std::cout << "Gecersiz giris!\n";
            continue;
        }
        std::cin.ignore(256, '\n');

        picked.push_back(sel);                  // dogrulamayi selectTargets yapar
    }
    return picked;
}

TargetChooser g_chooser = consoleChooser;

} // anonim namespace

void setTargetChooser(TargetChooser chooser){
    if(chooser) g_chooser = std::move(chooser);
}

std::vector<card*> selectTargets(Deck<card*>& pool, int count){
    std::vector<card*> chosen;
    if(count <= 0) return chosen;

    // 1) Aday listesi: sadece yasayan kartlar
    std::vector<card*> candidates;
    for(card* c : pool.getCards()){
        if(c && c->getHealth() > 0) candidates.push_back(c);
    }

    if(candidates.empty())                  return chosen;      // hic hedef yok
    if((int)candidates.size() <= count)     return candidates;  // yeterli yoksa hepsi

    // 2) Eksik kalan kadar chooser'a sor, gelen indeksleri dogrula
    while((int)chosen.size() < count){
        std::vector<int> idx = g_chooser(candidates, count - (int)chosen.size());

        if(idx.empty()) break;              // chooser vazgecti -> sonsuz dongu koruma

        for(int i : idx){
            if((int)chosen.size() >= count) break;
            if(i < 0 || i >= (int)candidates.size()) continue;   // gecersiz indeks

            card* picked = candidates[i];

            bool already = false;                                // ayni kart iki kez secilemez
            for(card* c : chosen){ if(c == picked){ already = true; break; } }
            if(already) continue;

            chosen.push_back(picked);
        }
    }

    return chosen;
}

} // namespace targeting
