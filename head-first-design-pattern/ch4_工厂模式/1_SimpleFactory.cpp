#include "../base/common.h"

/**
 * 简单工厂严格来说是一种编程习惯，而不是一种设计模式
 */

class Pizza
{
public:
    virtual void prepare() = 0;
    virtual void bake() = 0;
    virtual void cut() = 0;
    virtual void box() = 0;
};

class CheezePizza : public Pizza
{
public:
    CheezePizza()
    {
        // ...
    }

    void prepare() override {}
    void bake() override {}
    void cut() override {}
    void box() override {}
};

/// 简单披萨工厂，即通过一个对象来创建披萨实例
class SimplePizzaFactory
{
public:
    Pizza *createPizza(const std::string& type)
    {
        Pizza *pizza = nullptr;
        if (type == "cheese")
        {
            pizza = new CheezePizza();
        }
        else if (type == "pepperoni")
        {
            // pizza = new PepperoniPizza();
        }
        // ... 

        return pizza;
    }
};


class PizzaStore 
{
public: 
    PizzaStore(SimplePizzaFactory *factory) : factory(factory)
    {}

    Pizza *orderPizza(const std::string& type)
    {
        Pizza *pizza = nullptr;

        pizza = factory->createPizza(type);

        pizza->prepare();
        pizza->bake();
        pizza->cut();
        pizza->box();

        return pizza;
    }

private:
    SimplePizzaFactory *factory;
};

/// 这样做不够弹性，比如我想加盟披萨店，但是要建立 纽约风味的披萨，芝加哥风味披萨等，就需要分别实现 NYPizzaFactory, ChicagoPizzaFactory，....
/// 另外，如果想多一些控制，比如加盟店自创的流程：烘烤的差异，不要切片，换包装盒等。
/// 将 createPizza() 放回 PizzaStore 中，将其设置为虚函数，为每个区域风味建立一个子类。
/// 见 工厂方法


