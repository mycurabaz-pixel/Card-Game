#include "Card.h"
#include <iostream>

// std::cout iceren tek uye oldugu icin destructor'i kaynak dosyaya aldik.
card::~card(){
    std::cout << "card has been killed" << std::endl;
}
