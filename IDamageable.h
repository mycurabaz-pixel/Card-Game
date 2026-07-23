#ifndef IDAMAGEABLE_H
#define IDAMAGEABLE_H

#include <string>

// [DUZELTME] getOwner/setOwner Player* kullaniyor ama Player burada tanimli degildi.
// Sadece pointer tuttugumuz icin ileri bildirim yeterli (Player.h include etmiyoruz -> dongu yok).
class Player;

class IDamageable {
public:
    virtual void takeDamage(IDamageable* attacker, int dmg) = 0;
    virtual std::string getName() = 0;
    virtual int getHealth() = 0;
    virtual void setHealth(int health) = 0;
    virtual ~IDamageable() = default;
    virtual void reflect(IDamageable* target, int damage) = 0;
    virtual bool isReflectionMarked() = 0;
    virtual void setReflectionMarked(bool reflection_marked) = 0;
    virtual void addArmor(int armor) = 0;
    virtual int getArmor() = 0;
    virtual void setStunned(bool stunned) = 0;
    virtual bool isStunned() = 0;
    virtual void setInvulnerable(bool invulnerable) = 0;
    virtual bool isInvulnerable() = 0;
    virtual Player* getOwner() = 0;
    virtual void setOwner(Player* owner) = 0;
};

#endif // IDAMAGEABLE_H
