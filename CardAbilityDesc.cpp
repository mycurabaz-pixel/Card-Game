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
        {"Flame Dragon", "Set the battlefield on fire! \n (deals 7 damage to every card at the battlefield.)"},
        {"Red Dragon", "Shot a critical ember! \n (deals 17 damage to an enemy)"},
        {"Ice Healer", "Healed ally cards by freezing them! \n (heals all team by 3 hp)"},
        {"Frost Guardian", "Granted ice shields to an ally! \n (added 10 armor to an ally)"},
        {"Cruel Soldier", "Sword speed doubled! \n (doubled his damage)"},
        {"Dawn Knight", "Struck with a strike of justice! \n (deals 15 damage to a target)"},
        {"Ancient Mage", "Rained meteors from the sky! \n (deals 5 daamge to all cards in enemy team)"},
        {"Sky Mage", "Broke magic resistance! \n (halves damage hit a target)"},
        {"Forest Guardian", "Rooted enemies with vines! \n (stuns an enemy for 1 round)"},
        {"Dryad", "Called upon Mother Nature's blessing! \n (heals an ally by 5 hp)"},
        {"Storm Dragon", "Created a hurricane! \n (deals damge to 3 enemies it starts with 12 and halves for next enemy)"},
        {"Dark Mage", "Cast life drain! \n (deals 15 damage to a target and heals itself by 6 hp)"},
        {"Light Priest", "Blinded a card with light! \n (stuns a target)"},
        {"Village Vanguard", "Performed a simple but effective headbutt! \n (deals 7 damage to a target)"},
        {"Spellbreaker", "Cleared all spells in the area! \n (clears all effects from every card at the battlefield)"},
        {"Ember Guardian", "Infused weapon with fire! \n (doubles its own damage)"},
        {"Cryomancer", "Started a blizzard! \n (deals 2 damage to every card at the battlefield and adds a damage over time effect)"},
        {"Wind Healer", "Cleared poison with wind! \n (heals a target by 5 hp and clears its effects)"},
        {"Shadow Dragon", "Cast dark fear! \n (deals 10 damage to a target and adds a damage over time effect)"},
        {"Barbarian", "Entered rage mode! \n (doubles its own damage)"},
        {"Necromancer", "Revived the fallen! \n (revives your last dead card with half of its max hp)"},
        {"Cyber Observer", "Locked onto the target! \n (deals 3 damage to 3 enemies)"},
        {"High Tech Robot", "Fired a plasma ball! \n (deals 10 damage to a target)"},
        {"Chief Engineer", "Deployed an automatic turret! \n (summons a turret to the battlefield)"},
        {"Blacksmith Robot", "Repaired armor! \n (gives 5 armor to an ally)"},
        {"Pulse Guardian", "Emitted an electromagnetic wave! \n (gains 2 armor and turns lightning effects on itself into extra armor)"},
        {"Cyber Protector", "Activated firewall shield! \n (gains 8 armor)"},
        {"Doomsday Tank", "Fired a nuclear missile! \n (deals 7 damage to every card at the battlefield)"},
        {"Goliath V2", "Pierced through with a railgun! \n (deals 15 damage to a target)"},
        {"Light Drone", "Activated the signal jammer! \n (halves a target's damage)"},
        {"Swift Jet", "Caused a supersonic blast! \n (deals 7 damage to a target)"},
        {"Pyro-Technician", "Launched a napalm bomb! \n (deals 5 damage to all cards in enemy team)"},
        {"Freezing Android", "Sprayed liquid nitrogen! \n (freezes a target so it cannot act)"},
        {"Bio-Mechanical Golem", "Activated organic batteries! \n (triples its own damage and gains 3 armor)"},
        {"Rune Circuit", "Triggered a magical overload! \n (gives 1 extra damage to 3 allies)"},
        {"Saw Robot", "Overclocked the blades! \n (deals 2 damage to a target and adds a bleed effect)"},
        {"Quantum Reflector", "Bent light to become invisible! \n (makes itself invulnerable)"},
        {"Repair Unit", "Started repairs with nanobots! \n (heals all team by 2 hp)"},
        {"Dark Matter Armor", "Created a black hole! \n (splits its own hp into armor for 3 allies and makes them invulnerable, then dies)"},
        {"Sonic Copter", "Blew away with propeller wind! \n (deals 1 damage to all cards in enemy team)"},
        {"Cybernetic Ninja", "Emitted a katana shockwave! \n (doubles its own damage)"},
        {"Nightmare Kraken", "Cast a deep sea curse! \n (deals 5 damage to all enemies and lowers their max hp by 5)"},
        {"Leviathan", "Started a tsunami! \n (deals 7 damage to every card at the battlefield)"},
        {"Coral Healer", "Active coral healing! \n (heals a target by 7 hp)"},
        {"Mermaid", "Sang the current song! \n (raises all allies max hp by 2 and heals them by 2)"},
        {"Blackbeard", "Executed a boarding attack! \n (makes all allies hit a target for a quarter of their damage)"},
        {"Captain Barbarossa", "Subjected the fleet to cannon fire! \n (deals 3 damage to all cards in enemy team)"},
        {"Sea Oracle", "Saw the storm of the future! \n (reveals the enemy hero's hand)"},
        {"Siren Song", "Charmed the enemy ! \n (takes an enemy card to your side for 2 turns)"},
        {"Arctic Raider", "Fired a frozen harpoon! \n (deals 4 damage to a target and freezes it)"},
        {"Frozen Giant Octopus", "Threw an ice sheet with tentacles! \n (deals 20 damage to a target)"},
        {"Pirate Cannoneer", "Used Greek fire! \n (adds a fire damage over time effect to all enemies)"},
        {"Stormbringer", "Blew a capsizing wind! \n (deals 2 damage to all cards in enemy team)"},
        {"Sea Glow Sprite", "Blinded with a night glow! \n (stuns a target)"},
        {"Ship Crew", "Threw a rum bottle! \n (deals 5 damage to a target)"},
        {"Deepwater Guardian", "Will drown the magic shield in a vortex! \n (gives 4 armor to all allies)"},
        {"Razor Tentacle", "Sharp tentacle slash! \n (deals 6 damage to 3 enemies and adds a bleed effect)"},
        {"Glacial Savior", "Performed cold water shock therapy! \n (heals all team by 3 hp)"},
        {"Abyss Witch", "Mariana trench darkness! \n (deals 4 damage to a target and stuns it)"},
        {"Lava Captain", "Volcanic pirate attack! \n (deals 6 damage to a target and adds a fire damage over time effect)"},
        {"Monsoon Healer", "Humid storm healing! \n (heals a target by 3 hp and gives it 3 armor)"},
    };
    return m;
}
