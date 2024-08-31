#include <iostream>
#include <iostream>
#include <vector>

/**
 * 将共同的逻辑(算法步骤)封装到一个基类，而不同的细节交给子类实现
 * 基类中定义一个共享的公共模板方法 或 预设好一个的执行步骤顺序
*/

class CaffeineBeverage {
public:
    // 模板方法定义了算法的步骤，通常为 final，防止子类修改
    void prepareRecipe() {
        boilWater();
        brew(); 
        pourInCup();
        addCondiments();
    }

protected:
    virtual void brew() const = 0;           // 抽象方法，需要子类具体实现
    virtual void addCondiments() const = 0;  // 抽象方法，需要子类具体实现

    void boilWater() const {
        std::cout << "Boiling water" << std::endl;
    }

    void pourInCup() const {
        std::cout << "Pouring into cup" << std::endl;
    }
};

class Tea : public CaffeineBeverage {
protected:
    void brew() const override {
        std::cout << "Steeping the tea" << std::endl;
    }

    void addCondiments() const override {
        std::cout << "Adding lemon" << std::endl;
    }
};

class Coffee : public CaffeineBeverage {
protected:
    void brew() const override {
        std::cout << "Dripping coffee through filter" << std::endl;
    }

    void addCondiments() const override {
        std::cout << "Adding sugar and milk" << std::endl;
    }
};

int main() {
    Tea myTea;
    Coffee myCoffee;

    std::cout << "Making tea:" << std::endl;
    myTea.prepareRecipe();

    std::cout << "\nMaking coffee:" << std::endl;
    myCoffee.prepareRecipe();

    return 0;
}