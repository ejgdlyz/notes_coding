#include "../base/common.h"

/**
 * 工厂方法
 */

class Pizza
{
public:
    virtual void prepare()
    {
        println("Preparing " + name);
        println("Tossing dough... ");
        println("Adding sauce... " );
        println("Adding topings:");
        for(const auto& top: toppings)
        {
            println("\t" + top);
        }

    }
    virtual void bake()
    {
        println("Bake for 25 minutes at 350");
    }
    virtual void cut()
    {   
        println("Cutting the pizza into diagonal slices");
    }
    virtual void box()
    {
        println("Place pizza in offical PizzaStore box");
    }

    string getName() const { return name;}

protected:
    string name;                    // 名称
    string dough;                   // 面团类型
    string sauce;                   // 酱料
    vector<string> toppings;        // 佐料
};

/// @brief 纽约风味的芝士披萨
class NYStyleCheezePizza : public Pizza
{
public:
    NYStyleCheezePizza()
    {
        name = "NY Style Sauce and Cheese Pizza";
        dough = "Thin Crust Dough";
        sauce = "Marinara Sauce";

        toppings.push_back("Grated Reggiano Cheese");
    }
};

/// @brief 芝加哥风味的芝士披萨
class ChicagoStyleCheezePizza : public Pizza
{
public:
    ChicagoStyleCheezePizza()
    {
        name = "Chicago Style Deep Dish Cheese Pizza";
        dough = "Extra Thick Crust Dough";
        sauce = "Plum Tomato Sauce";

        toppings.push_back("Shredded Mozzarella Cheese");
    }

    /// @brief 将披萨切为正方形
    void cut() override
    {
        println("Cutting the pizza into square slices");
    }
};

class PizzaStore
{
public:
    /// 该方法不知道那些具体类参与进来，即解耦
    Pizza *orderPizza(const std::string &type)
    {
        Pizza *pizza = nullptr;

        pizza = createPizza(type);

        pizza->prepare();
        pizza->bake();
        pizza->cut();
        pizza->box();

        return pizza;
    }

    /// 工厂方法，子类负责自己的 createPizza() 定义
    virtual Pizza *createPizza(const std::string& type) = 0;
};

class NYStylePizzaStore : public PizzaStore
{
public:
    NYStylePizzaStore() : PizzaStore() {}
    
    Pizza *createPizza(const std::string& type) override 
    {
        Pizza *pizza = nullptr;
        if (type == "cheese")
        {
            pizza = new NYStyleCheezePizza();
        }
        // ... 

        return pizza;
    }
};

class ChicagoStylePizzaStore : public PizzaStore
{
public:
    ChicagoStylePizzaStore() : PizzaStore() {}

    Pizza *createPizza(const std::string& type) override 
    {
        Pizza *pizza = nullptr;
        if (type == "cheese")
        {
            pizza = new ChicagoStyleCheezePizza();
        }
        // ... 

        return pizza;
    }
};

// 原本由一个对象负责所有具体类的实例化，现在由一群子类负责实例化。
// 实例化披萨的责任被移到一个 “方法”中，此方法如同一个工厂

// 工厂方法用于处理对象的创建，并将这样的行为封装在子类中。客户程序中关于超类的代码和子类对象创建代码解耦了。

/// 订购披萨
void test01()
{
    PizzaStore *nyPizzaStore = new NYStylePizzaStore();
    PizzaStore *chiPizzaStore = new ChicagoStylePizzaStore();

    auto pizza = nyPizzaStore->orderPizza("cheese");  // 纽约风味的 cheese 披萨
    println("Ethan ordered a " + pizza->getName());    
    delete pizza;
    pizza = nullptr;

    pizza = chiPizzaStore->orderPizza("cheese");  // 芝加哥风味的 cheese 披萨
    println("Joel ordered a " + pizza->getName());    
    delete pizza;
    pizza = nullptr;
}

int main(int argc, char const *argv[])
{
    test01();
    return 0;
}
