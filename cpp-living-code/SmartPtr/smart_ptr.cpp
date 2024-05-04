#include <iostream>
#include <memory>


template<class T>
class SmartPtr
{
public:
    SmartPtr(): ptr_(nullptr), cnt_(nullptr) {}
    explicit SmartPtr(T* ptr): ptr_(ptr), cnt_(new size_t(1)) {}


    SmartPtr(const SmartPtr<T>& other): ptr_(other.ptr_), cnt_(other.cnt_)
    {
        ++(*cnt_);
    }

    SmartPtr<T>& operator=(const SmartPtr<T>& other)
    {
        if (ptr_ == other.ptr_)
        {
            return *this;
        }

        release();
        ptr_ = other.ptr_;
        cnt_ = other.cnt_;
        ++(*cnt_);

        return *this;
    }

    SmartPtr(SmartPtr<T>&& other) : ptr_(other.ptr_), cnt_(other.cnt_)  // 移动构造
    {
        ++(*cnt_);
        other.release();
    }

    SmartPtr<T>& operator=(SmartPtr<T>&& other)
    {
        if (ptr_ == other.ptr_)
        {
            return *this;
        }

        release();
        ptr_ = other.ptr_;
        cnt_ = other.cnt_;
        ++(*cnt_);

        other.release();

        return *this;
    }

    ~SmartPtr()
    {
        release();
    }

    T* get()
    {
        return ptr_;
    }

    T* operator->()
    {
        return ptr_;
    }

    T& operator*()
    {
        return *ptr_;
    }

    size_t getCnt()
    {
        if (cnt_ == nullptr) 
            return 0;
        return *cnt_;
    }

private:
    T* ptr_;
    size_t* cnt_;
    
    void release()
    {
        if (cnt_ != nullptr && --(*cnt_) == 0)
        {
            delete ptr_;
            delete cnt_;
            ptr_ = nullptr;
            cnt_ = nullptr;
        }
    }
};

struct A
{
    int a = 0;
};

void test()
{
    SmartPtr<A> sp0;
    std::cout << "sp0: " << sp0.getCnt() << std::endl;
    
    {
        SmartPtr<A> sp1(new A);
        std::cout << "sp1: " << sp1.getCnt() << std::endl;

        SmartPtr<A> sp2(sp1);
        std::cout << "sp2: " << sp2.getCnt() << std::endl;
        
        sp0 = sp2;
        std::cout << "sp0: " << sp0.getCnt() << std::endl;
        
    }
    std::cout << "sp0: " << sp0.getCnt() << std::endl;

    SmartPtr<A> sp1(std::move(sp0));
    std::cout << "sp1: " << sp1.getCnt() << std::endl;

    SmartPtr<A> sp2;
    sp2 = std::move(sp1);
    std::cout << "sp2: " << sp2.getCnt() << std::endl;

}

void test2()
{
    std::shared_ptr<A> sp0;
    std::cout << "sp0: " << sp0.use_count() << std::endl;
    
    {
        std::shared_ptr<A> sp1(new A);
        std::cout << "sp1: " << sp1.use_count() << std::endl;

        std::shared_ptr<A> sp2(sp1);
        std::cout << "sp2: " << sp2.use_count() << std::endl;
        
        sp0 = sp2;
        std::cout << "sp0: " << sp0.use_count() << std::endl;
    }

    std::cout << "sp0: " << sp0.use_count() << std::endl;

    std::shared_ptr<A> sp1(std::move(sp0));
    std::cout << "sp1: " << sp1.use_count() << std::endl;
}
int main(int argc, char const *argv[])
{
    // test();
    std::cout << "offical implementation ....." << std::endl;
    // test2();
    float f = 1.0;
    short c = *(short*)&f;
    std::cout << c << std::endl;
    std::cout << sizeof(float) << std::endl;
    return 0;
}
