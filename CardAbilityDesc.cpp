// ============================================================================
// OTOMATIK URETILDI - elle duzenleme.
// Kaynak: GameManager.cpp icindeki yetenek lambdalarinda gecen
//         std::cout << "-> Ability: ..." mesajlari.
// Yani kart yetenek aciklamalari SENIN KODUNDAN geliyor.
// Lambda mesajini degistirirsen bu dosyayi yeniden uretmek yeterli.
// ============================================================================

#include "CardAbilityDesc.h"

const std::map<std::string, std::string>& cardAbilityDesc(){
    static const std::map<std::string, std::string> m = {
        {"Flame Dragon", "Set the battlefield on fire!"},
        {"Red Dragon", "Shot a critical ember!"},
        {"Ice Healer", "Healed ally cards by freezing them!"},
        {"Frost Guardian", "Granted ice shields to an ally!"},
        {"Cruel Soldier", "Sword speed doubled!"},
        {"Dawn Knight", "Struck with a strike of justice!"},
        {"Ancient Mage", "Rained meteors from the sky!"},
        {"Sky Mage", "Broke magic resistance!"},
        {"Forest Guardian", "Rooted enemies with vines!"},
        {"Dryad", "Called upon Mother Nature's blessing!"},
        {"Storm Dragon", "Created a hurricane!"},
        {"Dark Mage", "Cast life drain!"},
        {"Light Priest", "Blinded a card with light!"},
        {"Village Vanguard", "Performed a simple but effective headbutt!"},
        {"Spellbreaker", "Cleared all spells in the area!"},
        {"Ember Guardian", "Infused weapon with fire!"},
        {"Cryomancer", "Started a blizzard!"},
        {"Wind Healer", "Cleared poison with wind!"},
        {"Shadow Dragon", "Cast dark fear!"},
        {"Barbarian", "Entered rage mode!"},
        {"Cyber Observer", "Locked onto the target!"},
        {"High Tech Robot", "Fired a plasma ball!"},
        {"Chief Engineer", "Deployed an automatic turret!"},
        {"Blacksmith Robot", "Repaired armor!"},
        {"Pulse Guardian", "Emitted an electromagnetic wave!"},
        {"Cyber Protector", "Activated firewall shield!"},
        {"Doomsday Tank", "Fired a nuclear missile!"},
        {"Goliath V2", "Pierced through with a railgun!"},
        {"Light Drone", "Activated the signal jammer!"},
        {"Swift Jet", "Caused a supersonic blast!"},
        {"Pyro-Technician", "Launched a napalm bomb!"},
        {"Freezing Android", "Sprayed liquid nitrogen!"},
        {"Bio-Mechanical Golem", "Activated organic batteries!"},
        {"Rune Circuit", "Triggered a magical overload!"},
        {"Saw Robot", "Overclocked the blades!"},
        {"Quantum Reflector", "Bent light to become invisible!"},
        {"Repair Unit", "Started repairs with nanobots!"},
        {"Dark Matter Armor", "Created a black hole!"},
        {"Sonic Copter", "Blew away with propeller wind!"},
        {"Cybernetic Ninja", "Emitted a katana shockwave!"},
        {"Nightmare Kraken", "Cast a deep sea curse!"},
        {"Leviathan", "Started a tsunami!"},
        {"Coral Healer", "Active coral healing!"},
        {"Mermaid", "Sang the current song!"},
        {"Blackbeard", "Executed a boarding attack!"},
        {"Captain Barbarossa", "Subjected the fleet to cannon fire!"},
        {"Sea Oracle", "Saw the storm of the future!"},
        {"Siren Song", "Charmed the enemy !"},
        {"Arctic Raider", "Fired a frozen harpoon!"},
        {"Frozen Giant Octopus", "Threw an ice sheet with tentacles!"},
        {"Pirate Cannoneer", "Used Greek fire!"},
        {"Stormbringer", "Blew a capsizing wind!"},
        {"Sea Glow Sprite", "Blinded with a night glow!"},
        {"Ship Crew", "Threw a rum bottle!"},
        {"Deepwater Guardian", "Will drown the magic shield in a vortex!"},
        {"Razor Tentacle", "Sharp tentacle slash!"},
        {"Glacial Savior", "Performed cold water shock therapy!"},
        {"Abyss Witch", "Mariana trench darkness!"},
        {"Lava Captain", "Volcanic pirate attack!"},
        {"Monsoon Healer", "Humid storm healing!"},
    };
    return m;
}
