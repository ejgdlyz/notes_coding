#include <iostream>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

struct A {
    int a;
};

// 共享内存分配器定义
namespace bip = boost::interprocess;
typedef bip::allocator<A, bip::managed_shared_memory::segment_manager> ShmemAllocator;
typedef bip::vector<A, ShmemAllocator> ShmemVector;

class B {
public:
    B(const ShmemAllocator& alloc) : m_data(alloc) {}

    void add_data(int value) {
        A a{value};
        m_data.push_back(a);
    }

    const ShmemVector& get_data() const {
        return m_data;
    }

private:
    ShmemVector m_data; // 使用共享内存分配器的 vector
};

int main() {
    // 创建或打开共享内存
    bip::managed_shared_memory segment(bip::open_or_create, "MySharedMemory", 65536);

    // 定义共享内存分配器
    ShmemAllocator alloc_inst(segment.get_segment_manager());

    // 在共享内存中构造对象 B
    B* b = segment.construct<B>("BObject")(alloc_inst);

    // 添加数据到 vector 中
    b->add_data(10);
    b->add_data(20);

    // 验证内存地址
    const ShmemVector& data = b->get_data();
    std::cout << "Address of B object: " << static_cast<void*>(b) << std::endl;
    std::cout << "Address of m_data: " << static_cast<const void*>(&data) << std::endl;
    for (const auto& elem : data) {
        std::cout << "Address of element in m_data: " << static_cast<const void*>(&elem) << std::endl;
    }

    // 销毁对象并删除共享内存
    segment.destroy<B>("BObject");
    bip::shared_memory_object::remove("MySharedMemory");
    
    return 0;
}

