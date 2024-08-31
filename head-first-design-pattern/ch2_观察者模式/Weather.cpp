#include "../base/common.h"
#include <vector>

class Observer;

class Subject
{
public:
    virtual void registerObserver(Observer *o) = 0;
    virtual void removeObserver(Observer *o) = 0;
    virtual void notifyObservers() = 0;
};

class Observer
{
public:
    virtual void update(float temp, float humidity, float presure) = 0;
};

class DisplayElement
{
public:
   virtual void display() = 0;    
};

/// @brief 实现主题接口 
class WeatherData : public Subject
{
public:
    WeatherData()
    {
        observers.reserve(64);
    }

    virtual void registerObserver(Observer *o) override
    {
        observers.push_back(o);
    }

    virtual void removeObserver(Observer *o) override
    {
        for(auto it = observers.begin(); it != observers.end(); ++it)
        {
            if (*it == o)
            {
                observers.erase(it++);
                return;
            }
        }
    }

    virtual void notifyObservers() override
    {
        for(auto it = observers.begin(); it != observers.end(); ++it)
        {
            Observer *observer = *it;
            observer->update(temprature, humidity, pressure);
        }
    }

    /// 当从气象站得到更新观测值时，通知观察者
    void measurementsChanged()
    {
        notifyObservers();
    }

    void setMeasurements(float temp, float humi, float pres)
    {
        temprature = temp;
        humidity = humi;
        pressure = pres;
        measurementsChanged(); // 模拟使用
    }

    /// WeatherData 其他方法...
    
private:
    vector<Observer*> observers;
    float temprature;
    float humidity;
    float pressure;
};

/// @brief 实现布告板
class CurrentConditionDisplay : public Observer, public DisplayElement
{
public:
    CurrentConditionDisplay(Subject *weatherData)
        : weatherData(weatherData)
    {
        weatherData->registerObserver(this);
    }

    virtual void update(float temp, float humi, float pres) override
    {
        temprature = temp;
        humidity = humi;
        
        display();
    }
    
    virtual void display() override
    {
        println("Current Conditions: ", temprature, "F degrees and ", humidity, "\% humidity");
    }

private:
    float temprature;
    float humidity;
    Subject *weatherData;
};

void test01()
{
    WeatherData *weatherData = new WeatherData;
    
    CurrentConditionDisplay *currentDisplay = new CurrentConditionDisplay(weatherData);
    
    weatherData->setMeasurements(80, 65, 30.4f);
    weatherData->setMeasurements(82, 70, 29.4f);
    weatherData->setMeasurements(78, 90, 28.4f);

    delete weatherData;
    delete currentDisplay;
}

int main(int argc, char const *argv[])
{
    test01();
    return 0;
}
