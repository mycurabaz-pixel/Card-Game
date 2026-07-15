#ifndef IDAMAGEABLE_H
#define IDAMAGEABLE_H

#include <string>

class IDamageable {
public:
    virtual void takeDamage(int dmg) = 0;
    virtual std::string getName() = 0;
    virtual int getHealth() = 0;
    virtual void setHealth(int health) = 0;
    virtual ~IDamageable() = default;
};

#endif // IDAMAGEABLE_H
