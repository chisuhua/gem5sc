// include/modules/credit_pool.hh
#ifndef CREDIT_FLOW_HH
#define CREDIT_FLOW_HH

#include <unordered_map>
#include <vector>

/**
 * CreditPool - 支持多流共享与预分配的信用池
 */
class CreditPool {
private:
    int total_credits;
    int available_credits;

    std::unordered_map<uint64_t, int> reservations;
    std::unordered_map<uint64_t, int> used_credits;

    struct WaitEntry {
        uint64_t stream_id;
        int count;
        explicit WaitEntry(uint64_t id, int c) : stream_id(id), count(c) {}
    };
    std::vector<WaitEntry> wait_queue;

public:
    explicit CreditPool(int credits) : total_credits(credits), available_credits(credits) {}

    bool reserveCredits(uint64_t stream_id, int count) {
        if (count <= available_credits) {
            reservations[stream_id] = count;
            used_credits[stream_id] = 0;
            available_credits -= count;
            return true;
        }
        return false;
    }

    bool tryGetCredit(uint64_t stream_id) {
        auto res_it = reservations.find(stream_id);
        auto used_it = used_credits.find(stream_id);

        if (res_it != reservations.end() && used_it != used_credits.end() && used_it->second < res_it->second) {
            used_it->second++;
            return true;
        }

        if (available_credits > 0) {
            available_credits--;
            used_credits[stream_id]++;
            return true;
        }

        for (auto& entry : wait_queue) {
            if (entry.stream_id == stream_id) {
                entry.count++;
                return false;
            }
        }
        wait_queue.emplace_back(stream_id, 1);
        return false;
    }

    void returnCredit(uint64_t stream_id) {
        auto it = used_credits.find(stream_id);
        if (it == used_credits.end() || it->second <= 0) return;

        it->second--;

        auto res_it = reservations.find(stream_id);
        if (res_it == reservations.end() || it->second < res_it->second) {
            available_credits++;
        }

        wakeWaiters();
    }

    void wakeWaiters() {
        for (auto it = wait_queue.begin(); it != wait_queue.end();) {
            if (available_credits >= it->count) {
                available_credits -= it->count;
                used_credits[it->stream_id] += it->count;
                DPRINTF(CREDIT, "Woke up stream %lu, granted %d credits\n", it->stream_id, it->count);
                it = wait_queue.erase(it);
            } else {
                ++it;
            }
        }
    }

    void releaseReservation(uint64_t stream_id) {
        auto res_it = reservations.find(stream_id);
        if (res_it != reservations.end()) {
            available_credits += res_it->second;
            reservations.erase(res_it);
            used_credits.erase(stream_id);
        }
    }

    int getAvailableCredits() const { return available_credits; }
    int getTotalCredits() const { return total_credits; }
};

#endif // CREDIT_FLOW_HH
