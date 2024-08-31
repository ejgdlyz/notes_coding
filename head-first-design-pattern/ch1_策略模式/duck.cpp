#include "../base/common.h"

class FlyBehavior
{
public:
    virtual void fly() = 0;
};

class QuackBehavior
{
public:
    virtual void quark() = 0;
};



class FlyWithWings : public FlyBehavior
{
public:
    virtual void fly() override 
    {
        println("I'm flying!!!");
    }
};

class FlyNoWay : public FlyBehavior
{
public:
    virtual void fly() override 
    {
        println("I cannot fly.");
    }
};

class Quack : public QuackBehavior
{
public:
    virtual void quark() override 
    {
        println("Quack");
    }
};

class MuteQuack : public QuackBehavior
{
public:
    virtual void quark() override 
    {
        println("<Silence>");
    }
};

class Squeak : public QuackBehavior
{
public:
    virtual void quark() override 
    {
        println("橡皮鸭子吱吱叫");
    }
};


class Duck
{
    
public:  /// A1: 加入两个新方法，设定鸭子的行为，而不是在鸭子的构造器内实例化对象
    void setFlyBehavior(FlyBehavior *fb)
    {
        if (flyBehavior)
        {
            delete flyBehavior;
            flyBehavior = nullptr;
        }
        flyBehavior = fb;
    }
    
    void setFlyBehavior(QuackBehavior *qb)
    {
        if (flyBehavior)
        {
            delete quackBehavior;
            quackBehavior = nullptr;
        }
        quackBehavior = qb;
    }

public:
    Duck()
    {
    }

    virtual ~Duck() 
    {
        delete quackBehavior;
        delete flyBehavior;
    }

    virtual void display() 
    {
        println("show xxx");
    }
    
    void performFly()
    {
        flyBehavior->fly();
    }

    void performQuark()
    {
        quackBehavior->quark();
    }

    void swim()
    {
        println("All ducks float, even decoys.");
    }

protected:
  FlyBehavior *flyBehavior;
  QuackBehavior *quackBehavior;
};

class MallardDuck : public Duck
{
public:
    MallardDuck()
    {
        flyBehavior = new FlyWithWings();  /// Que1: 直接指定实例变量，不够弹性
        quackBehavior = new Quack();
    }

    void display() override 
    {
       println("I'm a real Mallard duck.");
    }
};

class ModelDuck : public Duck
{
public:
    ModelDuck()
    {
        flyBehavior = new FlyNoWay();
        quackBehavior = new Quack();
    }

    void display() override 
    {
       println("I'm a mode duck.");
    }

};

class FlyRocketPowered : public FlyBehavior
{
public:
    virtual void fly() override 
    {
        println("I'm flying with a rocket!!!!!");
    }
};

void test01()
{

    Duck *mallard = new MallardDuck();
    mallard->performFly();
    mallard->performQuark();
    delete mallard;
}

void test02()
{

    Duck *model = new ModelDuck();
    model->performFly();
    
    model->setFlyBehavior(new FlyRocketPowered());

    model->performFly();

    delete model;
}

int main(int argc, char const *argv[])
{
    test01();

    test02();

    return 0;
}


/**
 * 策略模式
 * 定义算法族（不同组行为），分别封装起来，让它们之间可以互相替换，此模式让算法独立于使用算法的客户。
*/