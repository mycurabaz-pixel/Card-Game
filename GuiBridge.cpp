#include "GuiBridge.h"
#include "GameManager.h"
#include "TargetSelection.h"

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
      << "\"name\":\""   << esc(ch->getName()) << "\""
      << ",\"hp\":"      << ch->getHealth()
      << ",\"mana\":"    << ch->getMana()
      << ",\"cooldown\":"<< ch->getcurrentCooldown()
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
      << ",\"deckCount\":" << p.getDeck().remaining()   // yandaki deste sayaci icin
      << "}";
    return o.str();
}

std::string stateJson(){
    gameManager& gm = gameManager::getInstance();
    std::ostringstream o;
    o << "{"
      << "\"tour\":"          << gm.getTourCounter()
      << ",\"currentPlayer\":"<< g_currentPlayer
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

void startGame(const std::string& p1Name, const std::string& p2Name){
    gameManager& gm = gameManager::getInstance();
    gm.setPlayers(p1Name, p2Name);
    gm.startGame();
    g_currentPlayer = 1;   // sira P1'de baslar
}

bool chooseCharacter(int playerIdx, const std::string& poolKey){
    Player* p = playerByIndex(playerIdx);
    if(!p) return false;
    auto& pool = gameManager::getInstance().getCharacterPool();
    auto it = pool.find(poolKey);
    if(it == pool.end()) return false;
    p->setCharacter(it->second);
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
    if(targetIdx   < 0 || targetIdx   >= (int)field.size()) return false;
    if(attackerIdx == targetIdx) return false;

    card* attacker = field[attackerIdx];

    if(ownerIndex(attacker) != playerIdx){                  // sadece kendi kartinla saldir
        std::cout << "That card is not yours." << std::endl;
        return false;
    }
    if(!attacker->isActive()){                              // summoning sickness / bu tur zaten oynadi
        std::cout << attacker->getName() << " cannot act this turn (summoning sickness)." << std::endl;
        return false;
    }

    p->hitTarget(field[targetIdx], attacker);
    attacker->setActive(false);   // bir kart turda bir kez saldirir; advanceTurn tekrar aktif eder
    return true;
}

void endTurn(){
    gameManager& gm = gameManager::getInstance();

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
