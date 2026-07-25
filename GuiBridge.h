#ifndef GUIBRIDGE_H
#define GUIBRIDGE_H

#include <string>

// ============================================================================
// GUI KOPRUSU
//
// Amac: Oyun mantigi ile arayuz arasindaki TEK temas noktasi.
// Oyun mantigi (gameManager, card, Character...) hic degismez; bu katman
// disaridan cagrilan olaylari alir ve durumu JSON olarak geri verir.
//
// Neden gerekli: Konsol surumunde akis "C++ girdi ISTER" seklinde (cin ile
// bloklar). Arayuzde bu tersine doner: "Arayuz C++'i CAGIRIR". Bloklayan
// dongu (gameManager::run) arayuzde kullanilamaz.
//
// Bu katman hem native masaustu (WebView2) hem de WASM icin aynidir.
// ============================================================================

namespace gui {

// --- Kurulum -------------------------------------------------------------
// std::cout'u ic tampona yonlendirir. Bu sayede kodundaki ~96 cout satirinin
// HICBIRI degismeden savas gunlugu arayuze aktarilabilir.
void init();

// Biriken gunlugu dondurur ve tamponu temizler (arayuz her karede cagirir).
std::string takeLog();

// --- Durum ---------------------------------------------------------------
// Tahtanin tamami JSON olarak: oyuncular, karakterler, eller, takimlar, saha.
// Arayuz bunu JSON.parse ile okuyup ekrani cizer.
std::string stateJson();

// [YENI] Master deck'teki tum kartlar (oyun basi anlik kopyasi). GUI kart galerisi kullanir.
std::string masterDeckJson();

// --- Olaylar (arayuz bunlari cagirir) ------------------------------------
void startGame(const std::string& p1Name, const std::string& p2Name);

// Karakter havuzundan isimle secim yapar. playerIdx: 1 veya 2.
bool chooseCharacter(int playerIdx, const std::string& poolKey);

// Oyuncunun elindeki handIndex'inci karti sahaya surer.
bool playCard(int playerIdx, int handIndex);

// Oyuncuya kart cektirir.
void drawCard(int playerIdx);

// Sahadaki attackerIdx'inci kart, sahadaki targetIdx'inci karta vurur.
bool attack(int playerIdx, int attackerIdx, int targetIdx);

// Kartin OZEL YETENEGI (gameManager::useCardAbility). targetIdx < 0 -> hedefsiz.
bool useCardAbility(int playerIdx, int cardIdx, int targetIdx);

// KARAKTER yetenegi (gameManager::useCharacterAbility). targetIdx < 0 -> hedefsiz.
bool useCharacterAbility(int playerIdx, int targetIdx);

// Turu bitirir: sureli etkiler islenir, sayaclar ilerler.
void endTurn();

// playerIdx pes eder; rakip kazanir.
void surrender(int playerIdx);

// Arayuzden gelen hedef secimini bekleyen yetenekler icin: secilen indeksleri
// onceden kuyruga koyar (targeting::setTargetChooser bunlari kullanir).
void queueTargetSelection(const std::string& csvIndices);

} // namespace gui

#endif // GUIBRIDGE_H
