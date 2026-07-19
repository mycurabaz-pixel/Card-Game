#ifndef IDAMAGEABLE_H
#define IDAMAGEABLE_H

#include <string>

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
};

#endif // IDAMAGEABLE_H
