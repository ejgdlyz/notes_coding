#include <mutex>
#include <map>
#include <string>
#include <shared_mutex>

using namespace std;


// 使用 std::shared_mutex 保护数据结构

class dns_entry
{};
class dns_cache
{
    std::map<std::string, dns_entry> entries;   
    mutable std::shared_mutex entry_mutex;

public:
    dns_entry find_enrty(const std::string& domain) const
    {
        std::shared_lock<std::shared_mutex> lk(entry_mutex);  // 使用共享锁（读锁）保护共享的只读访问，使得多个线程得以调用 find_enrty()
        std::map<std::string, dns_entry> ::const_iterator const it = entries.find(domain);
        return (it == entries.end()) ? dns_entry(): it->second; 
    }
    void update_or_add_entry(std::string const& domain, dns_entry const& dns_details)
    {
        // 当缓存表需要更新时，使用排他锁（写锁）进行排他访问，
        // 其他线程调用 find_enrty 或 update_or_add_entry 都将阻塞
        std::lock_guard<std::shared_mutex> lk(entry_mutex);  
        entries[domain] = dns_details;
    }
}; 