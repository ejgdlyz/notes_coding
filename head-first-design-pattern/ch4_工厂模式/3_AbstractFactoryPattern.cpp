#include "../base/common.h"

/**
 * 抽象工厂模式
 */


/**
 * 披萨原料家族
    * 芝加哥使用一组原料，纽约使用另一组原料。
    * 每个家族都包含一种面团、一种酱料、一种芝士以及一种海鲜佐料等。
    * 每个区域使用自己的原料组，不同的区域就组成一个原料家族。
 * 
 */

/**
 * 原料工厂
    * 工厂将生产面团、酱料、芝士等。
    * 处理各个区域的差异。
 * 
 */


struct Dough
{};
struct Sauce
{};
struct Cheese
{};
struct Veggies
{};
struct Pepperoni
{};
struct Clams
{};

struct ThinCrustDough : public Dough
{};
struct ThickCrustDough : public Dough
{};

struct MarinaraSauce : public Sauce
{};
struct PlumTomatoSauce : public Sauce
{};

struct ReggianoCheese : public Cheese
{};
struct Mozzarella : public Cheese
{};

struct Garlic : public Veggies
{};
struct Onion : public Veggies
{};
struct Mashroom : public Veggies
{};
struct RedPepper : public Veggies
{};
struct BlackOlives : public Veggies
{};
struct EggPlant : public Veggies
{};
struct Spinach : public Veggies
{};

struct SlicedPrpperoni : public Pepperoni
{};

struct FreshClams : public Clams
{};
struct FrozenClams : public Clams
{};


/// @brief 原料工厂抽象接口
class PizzaIngredientFactory
{
public:
    /// 每个原料(类)对应一个方法来创建该原料

    virtual Dough *createDough() = 0;
    virtual Sauce *createSauce() = 0;
    virtual Cheese *createCheese() = 0;
    virtual vector<Veggies *> createVeggies() = 0;
    virtual Pepperoni *createPepperoni() = 0;
    virtual Clams *createClams() = 0;
};


/**
 * 每个区域创建一个工厂
 * 实现一组原料类供工厂使用，这些类可以在合适的区域内共享
 * 将原料工程整合进旧的 PizzaStore 中
*/

/// 具体的原料工厂

/// @brief  纽约原料工厂
class NYPizzaIngredientFactory : public PizzaIngredientFactory
{
public:
    /// 原料家族中的每一种原料，都提供了纽约的版本

    virtual Dough *createDough() override
    {
        return new ThinCrustDough();
    }

    virtual Sauce *createSauce() override
    {
        return new MarinaraSauce();
    }

    virtual Cheese *createCheese() override
    {
        return new ReggianoCheese();
    }
    
    virtual vector<Veggies *> createVeggies() override
    {
        vector<Veggies *> veggies;
        veggies.push_back(new Garlic());
        veggies.push_back(new Onion());
        veggies.push_back(new Mashroom());
        veggies.push_back(new RedPepper());
        return veggies;
    }
    
    virtual Pepperoni *createPepperoni() override
    {
        return new SlicedPrpperoni();
    }
    
    virtual Clams *createClams() override
    {
        return new FrozenClams();
    }
};

/// @brief  芝加哥原料工厂
class ChicagoPizzaIngredientFactory : public PizzaIngredientFactory
{
public:
    /// 原料家族中的每一种原料，都提供了纽约的版本

    virtual Dough *createDough() override
    {
        return new ThickCrustDough();
    }

    virtual Sauce *createSauce() override
    {
        return new PlumTomatoSauce();
    }

    virtual Cheese *createCheese() override
    {
        return new Mozzarella();
    }
    
    virtual vector<Veggies *> createVeggies() override
    {
        vector<Veggies *> veggies;
        veggies.push_back(new BlackOlives());
        veggies.push_back(new EggPlant());
        veggies.push_back(new Spinach());
        veggies.push_back(new RedPepper());
        return veggies;
    }
    
    virtual Pepperoni *createPepperoni() override
    {
        return new SlicedPrpperoni();
    }
    
    virtual Clams *createClams() override
    {
        return new FreshClams();
    }
};


/// 使用工厂生产的原料来重做披萨

class Pizza
{
public:
    Pizza(const string &name) : name(name)
    {}

    /// @brief 收集来自披萨工厂的原料
    virtual void prepare() = 0;

    void bake()
    {
        println("Bake for 25 minutes at 350");
    }

    void cut()
    {
        println("Cutting the pizza into diagonal slices");
    }

    void box()
    {
        println("Place pizza in offical PizzaStore box");
    }

    void setName(const string &name)
    {
        this->name = name;
    }

    string getName() const
    {
        return name;
    }

    string toString() const
    {
        // 打印披萨
    }

protected:
    /// 每个披萨都持有一组使用的原料
    string name;
    Dough *dough;
    Sauce *sauce;
    vector<Veggies *> veggies;
    Cheese *cheese;
    Pepperoni *pepperoni;
    Clams *clam;
};


/**
 * 创建纽约和芝加哥风味的披萨： NYCheesePizza 和 ChicagoCheesePizza'
 * 两者唯一的差别是使用的原料不同，做法都一样（面团+酱料+芝士）等。
 * 所以，不需要设计两个不同的类来处理不同风味的披萨，让原料工厂处理这种区域差异就可以
*/ 

/// @brief  芝士披萨
class CheesePizza : public Pizza
{
public:
    CheesePizza(PizzaIngredientFactory *ingredientFactory, const string &name = "CheesePizza")
        : Pizza(name), ingredientFactory(ingredientFactory)
    {}

    virtual void prepare() override
    {
        println("Prepare " + name);

        dough = ingredientFactory->createDough();
        sauce = ingredientFactory->createSauce();
        cheese = ingredientFactory->createCheese();
    }

private:
    PizzaIngredientFactory *ingredientFactory;
};

/// @brief 蛤蜊披萨
class ClamPizza : public Pizza
{
public:
    ClamPizza(PizzaIngredientFactory *ingredientFactory, const string &name = "ClamsPizza")
        : Pizza(name), ingredientFactory(ingredientFactory)
    {}

    virtual void prepare() override
    {
        println("Prepare " + name);

        dough = ingredientFactory->createDough();
        sauce = ingredientFactory->createSauce();
        cheese = ingredientFactory->createCheese();

        // 如果是纽约工厂，就会使用新鲜的蛤蜊。如果是芝加哥工厂，就会使用冷冻的蛤蜊。
        clam = ingredientFactory->createClams();
    }

private:
    PizzaIngredientFactory *ingredientFactory;
};

/**
 * Pizza 类不关心披萨原料，它只知道如何制作披萨。
 * Pizza 和区域原料之间解耦，无论原料在哪，Pizza 类都可以被复用。 
 * 
 * sauce = ingredientFactory->createSauce();
 * sauce:               将 Pizza 的实例变量设置为此批萨所使用的某种酱料
 * ingredientFactory:   原料工厂，Pizza 不在乎使用什么工厂，只要是原料工厂就行
 * createSauce():       该方法会返回这个区域所使用的酱料。
 */



/// 披萨店和本地的原料工厂搭上线

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

/// @brief 纽约披萨店
class NYPizzaStore : public PizzaStore
{
public: 
    Pizza *createPizza(const string &item) override
    {
        Pizza *pizza = nullptr;

        PizzaIngredientFactory *ingredientFactory = new NYPizzaIngredientFactory();

        if (item == "cheese")
        {
            pizza = new CheesePizza(ingredientFactory, "New York Style Cheese Pizza.");
        }
        else if (item == "vaggie")
        {
            pizza = new CheesePizza(ingredientFactory, "New York Style Veggie Pizza.");
        }
        else if (item == "clam")
        {
            pizza = new CheesePizza(ingredientFactory, "New York Style Clam Pizza.");
        }
        else if (item == "pepperoni")
        {
            pizza = new CheesePizza(ingredientFactory, "New York Style Pepponi Pizza.");
        }

        return pizza;
    }
};

/// @brief 芝加哥披萨店
class ChicagoPizzaStore : public PizzaStore
{
public: 
    Pizza *createPizza(const string &item) override
    {
        Pizza *pizza = nullptr;

        PizzaIngredientFactory *ingredientFactory = new NYPizzaIngredientFactory();

        if (item == "cheese")
        {
            pizza = new CheesePizza(ingredientFactory, "Chicago Style Cheese Pizza.");
        }
        else if (item == "vaggie")
        {
            pizza = new CheesePizza(ingredientFactory, "Chicago Style Veggie Pizza.");
        }
        else if (item == "clam")
        {
            pizza = new CheesePizza(ingredientFactory, "Chicago Style Clam Pizza.");
        }
        else if (item == "pepperoni")
        {
            pizza = new CheesePizza(ingredientFactory, "Chicago Style Pepponi Pizza.");
        }

        return pizza;
    }
};



/**
 * 引入抽象工厂来创建披萨原料家族
 * 
 * 抽象原料工厂为产品家族提供接口，如 面团、酱料、芝士、肉和蔬菜。
 * 抽象原料工厂派生一些具体的工厂，这些工厂生产相同的产品，但是产品的实现不同：
 *      纽约原料工厂为纽约披萨商店提供原料
 *      芝加哥原料工厂为芝加哥披萨商店提供原料
 * 
 * PizzaStore 利用具体的工厂所生产的原料来制作不同的产品，但是客户的代码始终保持不变
 * 
 */


void test()
{
    // Ethan 的订单
    PizzaStore *nyPizzaStore = new NYPizzaStore();
    nyPizzaStore->orderPizza("cheese");
    
    // Joel 的订单
    PizzaStore *chiPizzaStore = new ChicagoPizzaStore();
    chiPizzaStore->orderPizza("cheese");
}

int main(int argc, char const *argv[])
{
    
    test();

    return 0;
}
