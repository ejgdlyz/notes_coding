#include <iostream>
#include <queue>
using namespace std;


class MedianFinder {
public:
    priority_queue<int, vector<int>, greater<int>> heap1; // 小顶堆(较小元素位于堆顶)，保存较大的一半
    priority_queue<int, vector<int>, less<int>> heap2;  // 大顶堆(较大元素位于堆顶)，保存较小的一半
    MedianFinder() {

    }
    
    void addNum(int num) {
        // 需向 heap2 添加元素，为了使 heap2 一直保存较小的那一半，
        // 先添加在 heap1 中，再弹出 heap1 最小值
        if (heap1.size() != heap2.size())  
        {
            heap1.push(num);
            heap2.push(heap1.top());
            heap1.pop();
        }
        // 向 heap1 添加元素，为了使 heap1 一直保存较大的那一半，
        // 先添加在 heap2 中，然后弹出 heap2 的最大值放到 heap1 中
        else  
        {
            heap2.push(num);
            heap1.push(heap2.top());
            heap2.pop();
        }
    }
    
    double findMedian() {
        return heap1.size() == heap2.size() ? (heap1.top() + heap2.top()) / 2 : heap1.top();
    }
};



int main(int argc, char const *argv[])
{
    std::priority_queue<int, vector<int>,greater<int>> pq1;  // greater, 小顶堆, 较小的元素位于堆顶
    pq1.push(4);
    pq1.push(6);
    pq1.push(5);
    cout << pq1.top() << " ";
    pq1.pop();
    cout << pq1.top() << " ";
    pq1.pop();
    cout << pq1.top() << endl;
    pq1.pop();

    std::priority_queue<int, vector<int>,less<int>> pq2;  // 大顶堆，较大元素位于堆顶
    pq2.push(2);
    pq2.push(1);
    pq2.push(3);
    cout << pq2.top() << " ";
    pq2.pop();
    cout << pq2.top() << " ";
    pq2.pop();
    cout << pq2.top() << endl;
    pq2.pop();

    
    return 0;
}

