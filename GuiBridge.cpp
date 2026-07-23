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

    // Hedef secimini konsoldan degil arayuzden al.
    targeting::setTargetChooser(
        [](const std::vector<card*>& candidates, int count) -> std::vector<int> {
            std::vector<int> picked;
            while((int)picked.size() < count && !g_pendingTargets.empty()){
                picked.push_back(g_pendingTargets.front());
                g_pendingTargets.pop_front();
            }
            // Arayuz secim gondermediyse: bos donmek yerine ilk uygun adaylari al,
            // boylece oyun kilitlenmez (konsoldaki gibi bloklamak arayuzde olmaz).
            for(int i = 0; (int)picked.size() < count && i < (int)candidates.size(); ++i){
                bool used = false;
                for(int p : picked) if(p == i){ used = true; break; }
                if(!used) picked.push_back(i);
            }
            return picked;
        });
}

std::string takeLog(){
    std::string s = g_logBuffer.str();
    g_logBuffer.str("");                           // tamponu bosalt
    g_logBuffer.clear();
    return s;
}

void queueTargetSelection(const std::string& csvIndices){
    std::stringstream ss(csvIndices);
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
      // HASAR ARALIGI: artik SABIT formul yok. Kartin kendi getMaxRandomBonus()'u okunuyor,
      // yani random_unique_dmg'deki araligi degistirirsen GUI otomatik dogru gosterir.
      << ",\"dmgMin\":"      << c->getDmgHit()
      << ",\"dmgMax\":"      << (c->getDmgHit() + c->getMaxRandomBonus())
      << ",\"randMax\":"     << c->getMaxRandomBonus()
      // ACIKLAMA: kullanicinin kendi yazdigi getCardInfo() kullaniliyor.
      // Boylece kart metinlerini kodda degistirdiginde GUI otomatik guncellenir.
      << ",\"info\":\""      << esc(c->getCardInfo()) << "\""
      // YETENEK ACIKLAMASI: lambda'daki "-> Ability: ..." mesajindan
      << ",\"abilityDesc\":\"" << esc(abilityDescOf(c->getName())) << "\""
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
      << ",\"ready\":"      << (ch->isAbilityReady() ? "true" : "false")
      // Character'a ekledigin yeni ozellikler (armor / stun) GUI'de de gorunur:
      << ",\"armor\":"      << ch->getArmor()
      << ",\"stunned\":"    << (ch->isStunned() ? "true" : "false")
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
    // GUVENLIK: yetenek lambda'larinin cogu target'i null kontrolu yapmadan kullaniyor
    // (ornegin target->takeDamage(...)). nullptr gecersek segfault olur -> hedefi zorunlu tutuyoruz.
    if(!target){
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

    // Hedef kart olabilir veya DUSMAN KARAKTER (-3 / -2).
    IDamageable* target = resolveTarget(targetIdx);
    // GUVENLIK: karakter yetenegi lambda'lari da target'i null kontrolsuz kullanabiliyor
    // (orn. Kont: target->takeDamage(nullptr, 2)) -> hedefsiz cagri segfault ediyordu.
    if(!target){
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
