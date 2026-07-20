#ifndef TARGETSELECTION_H
#define TARGETSELECTION_H

#include <vector>
#include <functional>
#include "Deck.h"
#include "Card.h"

// ============================================================================
// GUI'YE UYGUN HEDEF SECIMI
//
// Fikir: KARAR (hangi hedefler secildi) ile ETKI (o hedeflere ne olacagi) ayrilir.
// Yetenek lambda'larin sadece selectTargets(...) cagirir ve girdinin NEREDEN
// geldigini bilmez. Boylece:
//
//   Konsol : varsayilan chooser std::cin okur          -> su an calisir
//   GUI/WASM: setTargetChooser(...) ile kendi chooser'ini takarsin
//             (JS'ten gelen indeksleri dondurur)       -> YETENEK KODU DEGISMEZ
//
// Tum dogrulama (olu kart eleme, tekrar secim, gecersiz indeks) selectTargets
// icinde yapilir; chooser sadece "hangi indeksler" sorusunu cevaplar. Bu sayede
// GUI tarafi kural bozamaz.
// ============================================================================

namespace targeting {

// Aday kartlari ve kac adet istendigini alir; secilen INDEKSLERI dondurur.
using TargetChooser = std::function<std::vector<int>(const std::vector<card*>& candidates, int count)>;

// Secim kaynagini degistir (GUI burayi kullanir).
// Hic cagrilmazsa varsayilan konsol surumu kullanilir.
void setTargetChooser(TargetChooser chooser);

// Havuzdan 'count' adet FARKLI ve YASAYAN hedef sec.
// - Olu kartlar (health <= 0) aday listesine alinmaz.
// - Aday sayisi count'tan azsa, hepsi dondurulur (sonsuz dongu olmaz).
// - Gecersiz / tekrar eden indeksler reddedilir, eksik kalan kadar tekrar sorulur.
std::vector<card*> selectTargets(Deck<card*>& pool, int count);

} // namespace targeting

#endif // TARGETSELECTION_H
