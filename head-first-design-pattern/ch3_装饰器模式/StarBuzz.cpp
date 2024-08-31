#include "../base/common.h"
#include <memory>

class Beverage
{
public:
    typedef std::shared_ptr<Beverage> ptr;

    Beverage(std::string desc) : description(desc)
    {
    }

    virtual std::string getDescription() const
    {
        return description;
    }

    virtual double cost() = 0;
protected:
    std::string description = "Unknown Beverage";
};

/******  饮料 *******/
/// 浓缩咖啡（具体组件）
class Espresso : public Beverage
{
public:
    Espresso() : Beverage("Espresso")
    {
    }

    double cost()
    {
        return 1.99;
    }
};

/// 饮料 2 （具体组件）
class DarkRoast : public Beverage
{
public:
    DarkRoast() : Beverage("DarkRoast")
    {}

    double cost()
    {
        return .99;
    }
};

/// 饮料 3 （具体组件）
class HouseBlend : public Beverage
{
public:
    HouseBlend() : Beverage("HouseBlend")
    {
    }

    double cost()
    {
        return .89;
    }
};

/******  调料 *******/
/// 调料类（装饰者抽象类）
class CondimentDecorator : public Beverage
{
public:
    CondimentDecorator(const std::string& desc) : Beverage(desc) 
    {}

    virtual std::string getDescription() const = 0;
};

/// 调料 1（具体装饰者）
class Mocha : public CondimentDecorator
{
public:
    Mocha(Beverage::ptr p) 
        : CondimentDecorator(""), beveragePtr(p)
    {
    }

    double cost() override 
    {
        return 0.2 + beveragePtr->cost();
    }

    std::string getDescription() const override
    {
        return beveragePtr->getDescription() + ", Mocha";
    }
private:
    Beverage::ptr beveragePtr;
};

class Soy : public CondimentDecorator
{
public:
    Soy(Beverage::ptr p) 
        : CondimentDecorator(""), beveragePtr(p)
    {}

    double cost() override
    {
        return 0.15 + beveragePtr->cost();
    }

    std::string getDescription() const override
    {
        return beveragePtr->getDescription() + ", Soy";
    }

private:
    Beverage::ptr beveragePtr;
};

class Whip : public CondimentDecorator
{
public:
    typedef std::shared_ptr<Whip> ptr;
    Whip(Beverage::ptr p)
        : CondimentDecorator(""), beveragePtr(p)
    {}

    double cost() override
    {
        return 0.1 + beveragePtr->cost();
    }

    std::string getDescription() const override
    {
        return beveragePtr->getDescription() + ", Whip";
    }
private:
    Beverage::ptr beveragePtr;
};

void test01()
{
    std::shared_ptr<Beverage> beverage = std::make_shared<Espresso>();
    println(beverage->getDescription() + " $" + std::to_string(beverage->cost()));

    std::shared_ptr<Beverage> beverage2 = std::make_shared<DarkRoast>();
    beverage2 = std::make_shared<Mocha>(beverage2);
    beverage2 = std::make_shared<Mocha>(beverage2);
    beverage2 = std::make_shared<Whip>(beverage2);
    println(beverage2->getDescription() + " $" + std::to_string(beverage2->cost()));

    std::shared_ptr<Beverage> beverage3 = std::make_shared<HouseBlend>();
    beverage3 = std::make_shared<Soy>(beverage3);
    beverage3 = std::make_shared<Mocha>(beverage3);
    beverage3 = std::make_shared<Whip>(beverage3);
    println(beverage3->getDescription() + " $" + std::to_string(beverage3->cost()));
}

int main(int argc, char const *argv[])
{
    test01();
    return 0;
}
