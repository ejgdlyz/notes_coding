#include <iostream>
#include <string>
#include <vector>
#include <list>
using namespace std;

// 来表达一种抽象的通知, 相当于 Observer
class IProgress  
{
public:
    virtual void DoProgress(float value) = 0;  // 相当于 update
    virtual ~IProgress() {}
};

class FileSplitter
{
    string m_filePath;  // 待分割的文件路径
    int m_fileNumber;   // 分割的文件个数

    list<IProgress*> m_iProgressList;  // 支持多个观察者
public:
    FileSplitter() = default;
    FileSplitter(const string& filePath, int fileNumber):
    m_filePath(filePath), m_fileNumber(fileNumber)
    {}

    void split()
    {
        // 1. 分批读取一个大文件

        // 2. 将大文件分批向小文件中写入
        for (int i = 0; i < m_fileNumber; ++i)
        {
            // ...

            float progressValue = m_fileNumber;
            progressValue = (i + 1) / progressValue;
            onProgress(progressValue);  // 发送通知

        }
    }

    void addIProgress(IProgress* iProgress)
    {
        m_iProgressList.push_back(iProgress);
    }

    void removeIProgress(IProgress* iProgress)
    {
        m_iProgressList.remove(iProgress);
    }

protected:
    virtual void onProgress(float value)
    {
        for (auto it = m_iProgressList.begin(); it != m_iProgressList.end(); ++it)
        {
            (*it)->DoProgress(value);  // 更新进度条
        }
        
    }
};

// 界面 
class TextBox
{
public:
    string getText();
};
class Form
{
public:
    virtual ~Form();
};


struct TextBox
{
    string getText();
};

// C++ 中不推荐使用多继承，但是 普通继承 + 抽象基类 是合适的
// 这里的 MainForm 可以认为是一个观察者
class MainForm : public Form, public IProgress
{
    TextBox* txtFilePath;
    TextBox* txtFileNumber;
    ProgressBar* progressBar;
public:
    void Button1_click()
    {
        string filePath = txtFilePath->getText();
        int number = atoi(txtFileNumber->getText().c_str());

        FileSplitter splitter;
        ConsoleNotifier cn;
        splitter.addIProgress(this);  // 订阅通知
        splitter.addIProgress(&cn);  // 订阅通知

        splitter.split();  

        splitter.removeIProgress(this);
        splitter.removeIProgress(&cn);

    }

    virtual void DoProgress(float value)
    {
        progressBar->setValue(value);
    }
};

// 进度条百分比/界面版本
struct ProgressBar
{
    void setValue(float value);
};

// 进度条 命令行/界面版本
class ConsoleNotifier : public IProgress
{
public:
    virtual void DoProgress(float value)
    {
        cout << ".";
    }
};
int main()
{
    std::cout << "hello world!" << std::endl;
    return 0;

}