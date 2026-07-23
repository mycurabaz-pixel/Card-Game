// ============================================================================
// MASAUSTU ARAYUZ KABUGU  (Native C++ + WebView2)
//
// Bu dosya oyunun mantigina HIC dokunmaz. Yaptigi tek is:
//   1) Bir pencere acar ve icine gui.html'i yukler,
//   2) C++ tarafindaki gui:: fonksiyonlarini JavaScript'e "bind" eder.
//
// Boylece HTML arayuzu window.playCard(...) gibi cagrilar yapar, gercek oyun
// mantigi C++'ta native calisir. WASM'a gerek yok.
// ============================================================================

#include "webview/webview.h"
#include "GuiBridge.h"

#include <string>
#include <windows.h>

// --- JS'ten gelen argumanlari cozmek icin kucuk yardimcilar -----------------
// webview, bind edilen fonksiyona argumanlari JSON dizisi olarak verir:
//   ["Alice","Bob"]  ya da  [1,0]
// Harici JSON kutuphanesi kullanmamak icin basit ayristirma yapiyoruz.

static std::string argRaw(const std::string& req, int index){
    int depth = 0, cur = 0;
    bool inStr = false, esc = false;
    std::string acc;
    for(size_t i = 0; i < req.size(); ++i){
        char c = req[i];
        if(esc){ acc += c; esc = false; continue; }
        if(c == '\\' && inStr){ acc += c; esc = true; continue; }
        if(c == '"'){ inStr = !inStr; acc += c; continue; }
        if(!inStr){
            if(c == '[' || c == '{'){ if(depth++ == 0 && c=='[') { continue; } }
            else if(c == ']' || c == '}'){ if(--depth == 0 && c==']') break; }
            else if(c == ',' && depth == 1){
                if(cur == index) return acc;
                acc.clear(); cur++; continue;
            }
        }
        if(depth >= 1) acc += c;
    }
    return (cur == index) ? acc : std::string();
}

static std::string trimQuotes(std::string s){
    size_t a = s.find_first_not_of(" \t");
    size_t b = s.find_last_not_of(" \t");
    if(a == std::string::npos) return "";
    s = s.substr(a, b - a + 1);
    if(s.size() >= 2 && s.front() == '"' && s.back() == '"') s = s.substr(1, s.size() - 2);
    return s;
}

static int argInt(const std::string& req, int index){
    std::string s = trimQuotes(argRaw(req, index));
    try { return std::stoi(s); } catch(...) { return -1; }
}

static std::string argStr(const std::string& req, int index){
    return trimQuotes(argRaw(req, index));
}

// JSON string olarak dondurmek icin kacis
static std::string jsonStr(const std::string& in){
    std::string out = "\"";
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
    return out + "\"";
}

// gui.html'i exe'nin yaninda arar (sabit yol yazmamak icin)
static std::string htmlPath(){
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string p(buf);
    size_t slash = p.find_last_of("\\/");
    if(slash != std::string::npos) p = p.substr(0, slash);
    for(char& c : p) if(c == '\\') c = '/';
    return "file:///" + p + "/gui.html";
}

int main(){
    gui::init();   // cout -> log tamponu, hedef secimi arayuze baglanir

    webview::webview w(false, nullptr);
    w.set_title("Project:Kangreya");
    w.set_size(1280, 820, WEBVIEW_HINT_NONE);

    // ---- C++ fonksiyonlarini JavaScript'e ac ----
    w.bind("getState", [](std::string) -> std::string {
        return gui::stateJson();                       // zaten JSON
    });

    w.bind("getLog", [](std::string) -> std::string {
        return jsonStr(gui::takeLog());
    });

    w.bind("startGame", [](std::string req) -> std::string {
        gui::startGame(argStr(req, 0), argStr(req, 1));
        return "true";
    });

    w.bind("chooseCharacter", [](std::string req) -> std::string {
        return gui::chooseCharacter(argInt(req, 0), argStr(req, 1)) ? "true" : "false";
    });

    w.bind("drawCard", [](std::string req) -> std::string {
        gui::drawCard(argInt(req, 0));
        return "true";
    });

    w.bind("playCard", [](std::string req) -> std::string {
        return gui::playCard(argInt(req, 0), argInt(req, 1)) ? "true" : "false";
    });

    w.bind("attack", [](std::string req) -> std::string {
        return gui::attack(argInt(req, 0), argInt(req, 1), argInt(req, 2)) ? "true" : "false";
    });

    w.bind("useCardAbility", [](std::string req) -> std::string {
        return gui::useCardAbility(argInt(req, 0), argInt(req, 1), argInt(req, 2)) ? "true" : "false";
    });

    w.bind("useCharacterAbility", [](std::string req) -> std::string {
        return gui::useCharacterAbility(argInt(req, 0), argInt(req, 1)) ? "true" : "false";
    });

    w.bind("surrender", [](std::string req) -> std::string {
        gui::surrender(argInt(req, 0));
        return "true";
    });

    w.bind("endTurn", [](std::string) -> std::string {
        gui::endTurn();
        return "true";
    });

    w.bind("queueTargets", [](std::string req) -> std::string {
        gui::queueTargetSelection(argStr(req, 0));
        return "true";
    });

    w.navigate(htmlPath());
    w.run();
    return 0;
}
