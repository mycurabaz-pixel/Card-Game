#include "GuiBridge.h"
#include "GameManager.h"
#include "TargetSelection.h"
#include "CardAbilityDesc.h"   // kart yetenek aciklamalari (senin lambda mesajlarindan uretildi)

#include <sstream>
#include <iostream>
#include <vector>
#include <deque>

namespace gui {

// ---------------------------------------------------------------------------
// 1) GUNLUK: std::cout'u kendi tamponumuza yonlendiriyoruz.
//    Boylece kodundaki tum "std::cout << ..." satirlari DEGISMEDEN arayuze akar.
// ---------------------------------------------------------------------------
static std::ostringstream g_logBuffer;
static std::streambuf*    g_oldCoutBuf = nullptr;

// Arayuzden gelen hedef secimleri (yetenekler hedef isteyince buradan okunur)
static std::deque<int> g_pendingTargets;

// SIRA: hangi oyuncunun turu (1 veya 2). Iki oyuncu ayni anda oynayamasin diye.
static int g_currentPlayer = 1;

// OYUN SONU: kazanan adi (bos = oyun suruyor)
static std::string g_winner;

// Karakterlerden biri oldu mu diye bakar; oldiyse kazanani belirler.
static void checkGameOver(){
    if(!g_winner.empty()) return;
    gameManager& gm = gameManager::getInstance();
    gm.endGame();                      // senin endGame'in isRunning'i kapatir
    Character* c1 = gm.getPlayer1().getCharacter();
    Character* c2 = gm.getPlayer2().getCharacter();
    if(c1 && c1->getHealth() <= 0){
        g_winner = gm.getPlayer2().getName();
        std::cout << "*** " << c1->getName() << " has fallen! " << g_winner << " WINS! ***" << std::endl;
    } else if(c2 && c2->getHealth() <= 0){
        g_winner = gm.getPlayer1().getName();
        std::cout << "*** " << c2->getName() << " has fallen! " << g_winner << " WINS! ***" << std::endl;
    }
}

void init(){
    if(g_oldCoutBuf) return;                       // iki kez kurma
    g_oldCoutBuf = std::cout.rdbuf(g_logBuffer.rdbuf());

    // Hedef secimini konsoldan degil ARAYUZDEN al.
    // [KOKTEN DUZELTME] g_pendingTargets artik aday-indeksi DEGIL, oyuncunun sectigi
    // kartlarin ID'sini (private_card_id) tutar. ID'yi adaylardaki dogru indekse ceviriyoruz;
    // boylece takim destesi ile arena sirasi farkli olsa bile DOGRU kartlar secilir.
    // Onceki halde bos kuyrukta "ilk N kart" aliniyordu -> hedefler rastgele gorunuyordu.
    // Artik uydurma dolgu YOK: kuyruk neyse o secilir; oyuncu az sectiyse selectTargets
    // bir sonraki bos cagrida durur (oyunu kilitlemez).
    targeting::setTargetChooser(
        [](const std::vector<card*>& candidates, int count) -> std::vector<int> {
            std::vector<int> picked;
            while((int)picked.size() < count && !g_pendingTargets.empty()){
                int wantedId = g_pendingTargets.front();
                g_pendingTargets.pop_front();
                for(int i = 0; i < (int)candidates.size(); ++i){
                    if(candidates[i]->getPrivateCardId() == wantedId){
                        bool used = false;
                        for(int p : picked) if(p == i){ used = true; break; }
                        if(!used) picked.push_back(i);
                        break;
                    }
                }
            }
            return picked;   // arayuz surer; kuyruk bitince bos doner -> selectTargets durur
        });
}

std::string takeLog(){
    std::string s = g_logBuffer.str();
    g_logBuffer.str("");                           // tamponu bosalt
    g_logBuffer.clear();
    return s;
}

// [DEGISIKLIK] Artik aday-indeksi degil, secilen kartlarin ID'leri (private_card_id) kuyruklaniyor.
void queueTargetSelection(const std::string& csvIds){
    std::stringstream ss(csvIds);
    std::string item;
    while(std::getline(ss, item, ',')){
        try { g_pendingTargets.push_back(std::stoi(item)); } catch(...) {}
    }
}

// ---------------------------------------------------------------------------
// 2) DURUM -> JSON  (harici kutuphane yok, elle uretiyoruz)
// ---------------------------------------------------------------------------
static std::string esc(const std::string& in){
    std::string out;
    for(char c : in){
        switch(c){
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

// Kart adina gore yetenek aciklamasi (yoksa bos doner)
static std::string abilityDescOf(const std::string& cardName){
    const auto& m = cardAbilityDesc();
    auto it = m.find(cardName);
    return it == m.end() ? std::string() : it->second;
}

// Kartin sahibi hangi oyuncu (1/2)? getOwner() adresini oyunculara karsi kontrol ediyoruz.
static int ownerIndex(card* c){
    gameManager& gm = gameManager::getInstance();
    Player* o = c->getOwner();
    if(o == &gm.getPlayer1()) return 1;
    if(o == &gm.getPlayer2()) return 2;
    return 0;
}

// [YENI] Hedef tarafini arayuze metin olarak gonderir ("enemy" / "ally" / "any").
// gui.html bu alana bakip hangi kartlarin hedeflenebilecegini isaretler.
static const char* sideStr(TargetSide s){
    switch(s){
        case TargetSide::ALLY: return "ally";
        case TargetSide::ANY:  return "any";
        case TargetSide::NONE: return "none";   // hedefsiz -> GUI hedef sormaz, USE butonu gosterir
        default:               return "enemy";
    }
}

// [YENI] DmgType -> kisa okunur ad (GUI efekt rozetinde gosterilir).
static const char* dmgTypeName(DmgType t){
    switch(t){
        case ICE: return "Ice"; case FIRE: return "Fire"; case NATURE: return "Nature";
        case WIND: return "Wind"; case MAGIC: return "Magic"; case ANTIMAGIC: return "Antimagic";
        case DARKNESS: return "Dark"; case LIGHT: return "Light"; case SLICE: return "Slice";
        case LIGHTNING: return "Lightning"; default: return "Normal";
    }
}

// [YENI] Sureli etkileri (zehir/kanama/DOT) JSON dizisine cevirir.
// Effect vektorunu alir; hem card->getEffects() hem Character->getEffects() ayni tipi
// dondurdugu icin AYNI fonksiyon ikisi icin de kullanilir (kopya kod yok).
static std::string effectsJson(const std::vector<Effect>& effs){
    std::ostringstream o;
    o << "[";
    bool first = true;
    for(const Effect& e : effs){
        if(!first) o << ",";
        first = false;
        o << "{\"type\":\"" << dmgTypeName(e.effectType) << "\""
          << ",\"dmg\":"      << e.damage
          << ",\"duration\":" << e.duration << "}";
    }
    o << "]";
    return o.str();
}

static std::string cardJson(card* c){
    std::ostringstream o;
    o << "{"
      << "\"id\":"      << c->getPrivateCardId()
      << ",\"name\":\"" << esc(c->getName()) << "\""
      << ",\"hp\":"     << c->getHealth()
      << ",\"dmg\":"    << c->getDmgHit()
      << ",\"cost\":"   << c->getCost()
      << ",\"armor\":"  << c->getArmor()
      << ",\"stunned\":"     << (c->isStunned() ? "true" : "false")
      << ",\"invulnerable\":"<< (c->isInvulnerable() ? "true" : "false")
      << ",\"marked\":"      << (c->isReflectionMarked() ? "true" : "false")
      << ",\"active\":"      << (c->isActive() ? "true" : "false")   // summoning sickness: false = bu tur oynayamaz
      << ",\"owner\":"       << ownerIndex(c)                        // arenayi sahibe gore ayirmak icin
      << ",\"abilityCost\":" << c->getAbilityCost()
      // [YENI] Kart yeteneginin hedef tarafi -> GUI hedef isaretlemesi bunu kullanir
      << ",\"targetSide\":\"" << sideStr(c->getTargetSide()) << "\""
      // [YENI] Kac hedef secilecek (>1 ise GUI coklu-secim modu acar)
      << ",\"targetCount\":" << c->getTargetCount()
      // HASAR ARALIGI: artik SABIT formul yok. Kartin kendi getMaxRandomBonus()'u okunuyor,
      // yani random_unique_dmg'deki araligi degistirirsen GUI otomatik dogru gosterir.
      << ",\"dmgMin\":"      << c->getDmgHit()
      << ",\"dmgMax\":"      << (c->getDmgHit() + c->getMaxRandomBonus())
      << ",\"randMax\":"     << c->getMaxRandomBonus()
      // [YENI] Temel hasar -> GUI su anki hasarla kiyaslayip artmis/azalmis gosterir (kilic sembolu)
      << ",\"baseDmg\":"     << c->getBaseDmgHit()
      // ACIKLAMA: kullanicinin kendi yazdigi getCardInfo() kullaniliyor.
      // Boylece kart metinlerini kodda degistirdiginde GUI otomatik guncellenir.
      << ",\"info\":\""      << esc(c->getCardInfo()) << "\""
      // YETENEK ACIKLAMASI: lambda'daki "-> Ability: ..." mesajindan
      << ",\"abilityDesc\":\"" << esc(abilityDescOf(c->getName())) << "\""
      // [YENI] Kartin uzerindeki sureli etkiler (zehir/kanama/DOT) -> GUI rozet olarak cizer
      << ",\"effects\":" << effectsJson(c->getEffects())
      << "}";
    return o.str();
}

static std::string cardListJson(const std::vector<card*>& v){
    std::ostringstream o;
    o << "[";
    for(size_t i = 0; i < v.size(); ++i){
        if(i) o << ",";
        o << cardJson(v[i]);
    }
    o << "]";
    return o.str();
}

static std::string characterJson(Character* ch){
    if(!ch) return "null";
    std::ostringstream o;
    o << "{"
      << "\"name\":\""      << esc(ch->getName()) << "\""
      << ",\"hp\":"         << ch->getHealth()
      << ",\"mana\":"       << ch->getMana()
      << ",\"cooldown\":"   << ch->getcurrentCooldown()
      << ",\"abilityCost\":"<< ch->getAbilityCost()
      << ",\"duration\":"   << ch->getAbilityDuration()
      // [YENI] Karakter yeteneginin hedef tarafi -> GUI hedef isaretlemesi bunu kullanir
      << ",\"targetSide\":\"" << sideStr(ch->getTargetSide()) << "\""
      << ",\"ready\":"      << (ch->isAbilityReady() ? "true" : "false")
      // Character'a ekledigin yeni ozellikler (armor / stun) GUI'de de gorunur:
      << ",\"armor\":"      << ch->getArmor()
      << ",\"stunned\":"    << (ch->isStunned() ? "true" : "false")
      // [YENI] Karakterde de invulnerable(immune) / reflection isareti gorunsun (kartlarla ayni alanlar)
      << ",\"invulnerable\":"<< (ch->isInvulnerable() ? "true" : "false")
      << ",\"marked\":"      << (ch->isReflectionMarked() ? "true" : "false")
      // [YENI] Karaktere de birakilan sureli etkiler (senin Character::getEffects() vektorun)
      << ",\"effects\":"     << effectsJson(ch->getEffects())
      << "}";
    return o.str();
}

static std::string playerJson(Player& p){
    std::ostringstream o;
    o << "{"
      << "\"name\":\"" << esc(p.getName()) << "\""
      << ",\"character\":" << characterJson(p.getCharacter())
      << ",\"hand\":"      << cardListJson(p.getHand())
      << ",\"team\":"      << cardListJson(p.getTeam().getCards())
      << ",\"deckCount\":"   << p.getDeck().remaining()   // yandaki deste sayaci icin
      << ",\"actionPoint\":" << p.getActionPoint()        // tur basina 3 kart aksiyonu
      << ",\"abilityPoint\":"<< p.getAbilityPoint()       // tur basina 1 karakter yetenegi
      << "}";
    return o.str();
}

std::string stateJson(){
    gameManager& gm = gameManager::getInstance();
    std::ostringstream o;
    o << "{"
      << "\"tour\":"          << gm.getTourCounter()
      << ",\"currentPlayer\":"<< g_currentPlayer
      << ",\"gameOver\":"     << (g_winner.empty() ? "false" : "true")
      << ",\"winner\":\""     << esc(g_winner) << "\""
      << ",\"battlefield\":"  << cardListJson(gm.getBattlefield().getCards())
      << ",\"player1\":"    << playerJson(gm.getPlayer1())
      << ",\"player2\":"    << playerJson(gm.getPlayer2())
      << ",\"characterPool\":[";
    bool first = true;
    for(auto& kv : gm.getCharacterPool()){
        if(!first) o << ",";
        first = false;
        o << "{\"key\":\"" << esc(kv.first) << "\","
          << "\"character\":" << characterJson(kv.second) << "}";
    }
    o << "]}";
    return o.str();
}

// [YENI] Master deck'teki TUM kartlar (oyun basi anlik kopyasi) JSON dizisi olarak.
// GUI kart galerisi bunu okuyup her karti gosterir; yeni eklenen kart burada gorunur.
std::string masterDeckJson(){
    gameManager& gm = gameManager::getInstance();
    return cardListJson(gm.getAllCards());
}

// ---------------------------------------------------------------------------
// 3) OLAYLAR
// ---------------------------------------------------------------------------
static Player* playerByIndex(int idx){
    gameManager& gm = gameManager::getInstance();
    if(idx == 1) return &gm.getPlayer1();
    if(idx == 2) return &gm.getPlayer2();
    return nullptr;
}

// HEDEF KODLAMASI (Character de IDamageable oldugu icin karakterler de hedef olabilir):
//   >= 0 : arenadaki kart indeksi
//   -1   : hedefsiz (nullptr)  -> AoE yetenekler
//   -2   : 1. oyuncunun KARAKTERI
//   -3   : 2. oyuncunun KARAKTERI
static IDamageable* resolveTarget(int idx){
    gameManager& gm = gameManager::getInstance();
    if(idx == -2) return gm.getPlayer1().getCharacter();
    if(idx == -3) return gm.getPlayer2().getCharacter();
    const std::vector<card*>& field = gm.getBattlefield().getCards();
    if(idx >= 0 && idx < (int)field.size()) return field[idx];
    return nullptr;
}

void startGame(const std::string& p1Name, const std::string& p2Name){
    gameManager& gm = gameManager::getInstance();
    gm.setPlayers(p1Name, p2Name);
    gm.startGame();
    g_currentPlayer = 1;   // sira P1'de baslar
    g_winner.clear();      // yeni oyun -> kazanan sifirlanir
}

bool chooseCharacter(int playerIdx, const std::string& poolKey){
    Player* p = playerByIndex(playerIdx);
    if(!p) return false;
    auto& pool = gameManager::getInstance().getCharacterPool();
    auto it = pool.find(poolKey);
    if(it == pool.end()) return false;
    p->setCharacter(it->second);
    // Karakterin sahibini ata: senin hit()/useUniquePower() icindeki
    // "getOwner() == target->getOwner()" kontrolu karakterlerde de calissin diye.
    // (Aksi halde owner nullptr kalir ve oyuncu KENDI karakterine vurabilir.)
    it->second->setOwner(p);
    std::cout << p->getName() << " chose character: " << it->second->getName() << std::endl;

    // Once both players have chosen, deal the OPENING HAND (4 cards each).
    gameManager& gm = gameManager::getInstance();
    if(gm.getPlayer1().hasCharacter() && gm.getPlayer2().hasCharacter()
       && gm.getPlayer1().getHand().empty() && gm.getPlayer2().getHand().empty()){
        for(int i = 0; i < 4; ++i){ gm.getPlayer1().drawCard(); gm.getPlayer2().drawCard(); }
        std::cout << "--- Characters chosen. Each player drew a 4-card opening hand. ---" << std::endl;
    }
    return true;
}

void drawCard(int playerIdx){
    if(Player* p = playerByIndex(playerIdx)) p->drawCard();
}

bool playCard(int playerIdx, int handIndex){
    if(playerIdx != g_currentPlayer){                       // [SIRA] sadece sirasi gelen oynar
        std::cout << "It is not player " << playerIdx << "'s turn." << std::endl;
        return false;
    }
    Player* p = playerByIndex(playerIdx);
    if(!p) return false;

    const std::vector<card*>& hand = p->getHand();
    if(handIndex < 0 || handIndex >= (int)hand.size()) return false;

    // Without a character, mana would be reduced on a null pointer -> guard.
    if(!p->hasCharacter()){
        std::cout << p->getName() << ": choose a character first!" << std::endl;
        return false;
    }

    card* chosen = hand[handIndex];
    // playCard now returns bool: false if not enough mana (GUI treats card as not played).
    if(!p->playCard(chosen, gameManager::getInstance().getBattlefield())) return false;

    std::cout << p->getName() << " played: " << chosen->getName() << std::endl;
    return true;
}

bool attack(int playerIdx, int attackerIdx, int targetIdx){
    if(playerIdx != g_currentPlayer){                       // [SIRA] sadece sirasi gelen saldirir
        std::cout << "It is not player " << playerIdx << "'s turn." << std::endl;
        return false;
    }
    Player* p = playerByIndex(playerIdx);
    if(!p) return false;

    const std::vector<card*>& field = gameManager::getInstance().getBattlefield().getCards();
    if(attackerIdx < 0 || attackerIdx >= (int)field.size()) return false;
    if(targetIdx >= 0 && attackerIdx == targetIdx) return false;   // kendine vurma

    IDamageable* target = resolveTarget(targetIdx);   // kart VEYA karakter olabilir
    if(!target){
        std::cout << "No valid target." << std::endl;
        return false;
    }

    card* attacker = field[attackerIdx];

    if(ownerIndex(attacker) != playerIdx){                  // sadece kendi kartinla saldir
        std::cout << "That card is not yours." << std::endl;
        return false;
    }
    if(!attacker->isActive()){                              // summoning sickness / bu tur zaten oynadi
        std::cout << attacker->getName() << " cannot act this turn (summoning sickness)." << std::endl;
        return false;
    }
    // NOT: actionPoint dusurmesi ve kontrolu SENIN Player::hitTarget'inda yapiliyor.
    // Burada sadece bilgilendirme icin bakiyoruz (cift dusmesin diye harcamiyoruz).
    if(p->getActionPoint() <= 0){
        std::cout << p->getName() << " has no action points left this turn." << std::endl;
        return false;
    }

    // NOT: setActive(false) artik SENIN hit() fonksiyonunda (sadece saldiri gercekten
    // gerceklestiginde). Burada tekrar yapmiyoruz: engellenen bir saldiri (ornegin kendi
    // takimina) kartin hakkini bosa harcamasin.
    p->hitTarget(target, attacker);   // senin hitTarget'in IDamageable aldigi icin karakter de vurulabilir
    checkGameOver();                  // karakter oldiyse oyunu bitir
    return true;
}

// Arena'daki bir kartin OZEL YETENEGINI kullanir.
// targetIdx < 0 ise hedefsiz (nullptr) cagrilir -> AoE yetenekler icin.
bool useCardAbility(int playerIdx, int cardIdx, int targetIdx){
    if(playerIdx != g_currentPlayer){
        std::cout << "It is not player " << playerIdx << "'s turn." << std::endl;
        return false;
    }
    gameManager& gm = gameManager::getInstance();
    Player* user  = playerByIndex(playerIdx);
    Player* enemy = playerByIndex(playerIdx == 1 ? 2 : 1);
    if(!user || !enemy) return false;

    const std::vector<card*>& field = gm.getBattlefield().getCards();
    if(cardIdx < 0 || cardIdx >= (int)field.size()) return false;

    card* usedCard = field[cardIdx];
    if(ownerIndex(usedCard) != playerIdx){
        std::cout << "That card is not yours." << std::endl;
        return false;
    }
    if(!usedCard->isActive()){
        std::cout << usedCard->getName() << " cannot act this turn (summoning sickness)." << std::endl;
        return false;
    }
    if(!user->hasCharacter() || !user->getCharacter()->canAfford(usedCard->getAbilityCost())){
        std::cout << "Not enough mana for " << usedCard->getName() << "'s ability." << std::endl;
        return false;
    }

    // NOT: actionPoint kontrolu/dusumu ve mana harcamasi SENIN Player::useCardUniqueAbility'inde.
    if(user->getActionPoint() <= 0){
        std::cout << user->getName() << " has no action points left this turn." << std::endl;
        return false;
    }

    IDamageable* target = resolveTarget(targetIdx);
    // GUVENLIK: hedefli yeteneklerin lambda'lari target'i null kontrolu yapmadan kullaniyor
    // (ornegin target->takeDamage(...)). nullptr gecersek segfault olur -> hedefi zorunlu tutuyoruz.
    // Istisnalar (nullptr GUVENLI, target'a dokunulmaz):
    //   - targetSide == NONE : lambda pitch/allyTeam/enemyTeam/self uzerinden calisir.
    //   - targetCount > 1     : COK hedefli; lambda selectTargets ile kuyruktan secer,
    //                           'target' parametresini kullanmaz (arayuz -1 gecer).
    if(!target && usedCard->getTargetSide() != TargetSide::NONE && usedCard->getTargetCount() <= 1){
        std::cout << "This ability needs a target." << std::endl;
        return false;
    }

    gm.useCardAbility(*user, *enemy, target, usedCard);   // senin gameManager metodun (mana + action orada)
    // setActive(false) senin useUniquePower'inda; burada tekrar yapmiyoruz.
    std::cout << usedCard->getName() << " used its ability." << std::endl;
    checkGameOver();
    return true;
}

// Oyuncunun KARAKTER yetenegini kullanir (senin gameManager::useCharacterAbility'in).
bool useCharacterAbility(int playerIdx, int targetIdx){
    if(playerIdx != g_currentPlayer){
        std::cout << "It is not player " << playerIdx << "'s turn." << std::endl;
        return false;
    }
    gameManager& gm = gameManager::getInstance();
    Player* user  = playerByIndex(playerIdx);
    Player* enemy = playerByIndex(playerIdx == 1 ? 2 : 1);
    if(!user || !enemy || !user->hasCharacter()) return false;

    // Hedef kart olabilir veya KARAKTER (-3 / -2).
    IDamageable* target = resolveTarget(targetIdx);
    // GUVENLIK: hedefli karakter yeteneklerinin lambda'lari target'i null kontrolsuz kullaniyor
    // (orn. Kont: target->takeDamage(nullptr, 2)) -> hedefsiz cagri segfault ediyordu.
    // [YENI] targetSide == NONE olan karakterler haric (Hope / First Prince / Loras / Queen):
    // onlarin lambda'si sadece allyTeam / enemyTeam uzerinden calisir -> nullptr guvenli.
    if(!target && user->getCharacter()->getTargetSide() != TargetSide::NONE){
        std::cout << "This hero ability needs a target." << std::endl;
        return false;
    }

    gm.useCharacterAbility(*user, *enemy, target);   // cooldown/mana kontrolu senin kodunda
    checkGameOver();
    return true;
}

// TESLIM OL: playerIdx pes eder, rakip kazanir.
void surrender(int playerIdx){
    gameManager& gm = gameManager::getInstance();
    Player* p    = playerByIndex(playerIdx);
    Player* foe  = playerByIndex(playerIdx == 1 ? 2 : 1);
    if(!p || !foe) return;
    g_winner = foe->getName();
    gm.stopGame();
    std::cout << "*** " << p->getName() << " SURRENDERED! " << g_winner << " WINS! ***" << std::endl;
}

void endTurn(){
    gameManager& gm = gameManager::getInstance();
    if(!g_winner.empty()) return;      // oyun bitti

    if(g_currentPlayer == 1){
        // P1 turunu bitirdi -> sira P2'ye gecer. Tam tur-sonu islemleri henuz yapilmaz.
        g_currentPlayer = 2;
        std::cout << "--- " << gm.getPlayer2().getName() << "'s turn ---" << std::endl;
    } else {
        // Her iki oyuncu da oynadi -> TAM TUR bitti.
        gm.advanceTurn();               // senin tur-sonu mantigin: mana, executeEffects, tick'ler,
                                        // action puanlari VE tum kartlari tekrar aktif etme (setActive(true))
        gm.getPlayer1().drawCard();     // otomatik cekim (tur basina bir)
        gm.getPlayer2().drawCard();
        g_currentPlayer = 1;
        std::cout << "--- New round. Both players drew a card. "
                  << gm.getPlayer1().getName() << "'s turn ---" << std::endl;
    }
}

} // namespace gui
