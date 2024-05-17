#include <iostream>
#include <string>
#include <sstream>

using namespace std;

struct ListNode {
    int val;
    ListNode* next;
    ListNode* prev;
    ListNode(int val = 0, ListNode* next = nullptr, ListNode* prev = nullptr)
        : val(val)
        , next(next)
        , prev(prev){
    }
};

class List {
public:
    List(const List&) = delete;
    List& operator=(const List&) = delete;

    List() {
        dummy_head = new ListNode();
        dummy_tail = new ListNode();
        dummy_head->next = dummy_tail;
        dummy_tail->prev = dummy_head;
    }

    ~List() {
        auto tmp = dummy_head;
        while (tmp) {
            auto next = tmp->next;
            delete tmp;
            tmp = next;
        }
    }

    bool empty() const {
        return dummy_head->next == dummy_tail;
    }

    void pushFront(int val) {
        auto node = new ListNode(val);
        insert(dummy_head, node);
    }

    void pushBack(int val) {
        auto node = new ListNode(val);
        insert(dummy_tail->prev, node);
    }

    void removeListNode(ListNode* node){
        node->prev->next = node->next;
        node->next->prev = node->prev;
        delete node;
        --size;
    }

    ListNode* findListNode(int val) const {
        auto cur = dummy_head->next;
        while (cur != dummy_tail) {
            if (cur->val == val) {
                return cur;
            }
            cur = cur->next;

        }
        return nullptr;
    }

    // 将新节点 node 插入到 prev 后
    void insert(ListNode* prev, ListNode* node) {
        node->prev = prev;
        node->next = prev->next;
        prev->next->prev = node;
        prev->next = node;
        ++size;
    }

    void toString(std::ostream& os) const {
        auto cur = dummy_head->next;
        os << "size=" << size << ": ";
        while (cur != dummy_tail) {
            os << cur->val << " ";
            cur = cur->next;
        }
    }

private:
    ListNode* dummy_head;
    ListNode* dummy_tail;
    int size;
    int capacity;
};


int main(int argc, char* argv[]) {
    int n = 5;
    List list;
    for(int i = 0; i < n; ++i) {
        list.pushBack(i);
    }
    list.pushFront(100);
    
    auto node = list.findListNode(3);
    if (node) {
        list.removeListNode(node);
    }

    std::stringstream ss;
    list.toString(ss);
    std::cout << ss.str() << endl;

    return 0;
}