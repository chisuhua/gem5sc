// include/modules/traffic_gen.hh
#ifndef TRAFFIC_GEN_HH
#define TRAFFIC_GEN_HH

#include "../sim_object.hh"
#include "../simple_port.hh"
#include "../packet.hh"
#include <vector>
#include <random>

enum class GenMode {
    SEQUENTIAL,
    RANDOM,
    STREAM_COPY,
    TRACE
};

struct TraceEntry {
    uint64_t addr;
    bool is_write;
};

class TrafficGenerator : public SimObject, public SimplePort {
private:
    SimplePort* downstream;
    GenMode mode;
    uint64_t start_addr, end_addr;
    uint64_t cur_addr;
    int num_requests;
    int completed;

    // Stream copy
    uint64_t src_base, dst_base, copy_size, copy_progress;

    // Trace mode
    std::vector<TraceEntry> trace;
    size_t trace_pos;

    // Random
    std::mt19937 rng;
    std::uniform_int_distribution<uint64_t> addr_dist;

    void scheduleNext();

public:
    TrafficGenerator(const std::string& n, EventQueue* eq, SimplePort* port,
                     GenMode m, uint64_t start = 0x1000, uint64_t end = 0x2000,
                     int req_count = 10)
        : SimObject(n, eq), downstream(port), mode(m), start_addr(start),
          end_addr(end), num_requests(req_count), completed(0), cur_addr(start),
          src_base(0x1000), dst_base(0x2000), copy_size(0), copy_progress(0),
          trace_pos(0), rng(std::random_device{}()),
          addr_dist(start, end)
    {
        if (mode == GenMode::TRACE) {
            // 示例 trace
            trace = {
                {0x1000, false},
                {0x1004, true},
                {0x1008, false},
                {0x100c, true}
            };
        }
    }

    void tick() override {
        // 在 tick 中尝试发起请求
        // 可以加间隔控制，如每 10 周期发一个
        if (event_queue->getCurrentCycle() % 10 == 0) {
            generateAndSend();
        }
    }

    bool recv(Packet* pkt) override {
        if (pkt->isResponse()) {
            completed++;
            DPRINTF(TG, "Received response, %d/%d completed\n", completed, num_requests);
            delete pkt;
            return true;
        }
        return false;
    }

    bool send(Packet* pkt) override {
        if (downstream) {
            return downstream->recv(pkt);
        }
        return false;
    }

private:
    void generateAndSend() {
        if (completed >= num_requests) return;

        bool issued = false;
        switch (mode) {
            case GenMode::SEQUENTIAL:
                issued = issueRequest(cur_addr % 3 == 0); // 每3个写一次
                cur_addr += 4;
                if (cur_addr >= end_addr) cur_addr = start_addr;
                break;

            case GenMode::RANDOM:
                issued = issueRequest(rng() % 2);
                break;

            case GenMode::STREAM_COPY:
                if (copy_progress < copy_size) {
                    uint64_t offset = copy_progress;
                    issued = issueRequest(true, dst_base + offset);     // 写
                    issueRequest(false, src_base + offset);            // 读（可并行）
                    copy_progress += 4;
                }
                break;

            case GenMode::TRACE:
                if (trace_pos < trace.size()) {
                    auto& entry = trace[trace_pos++];
                    issued = issueRequest(entry.is_write, entry.addr);
                }
                break;
        }

        if (issued) {
            DPRINTF(TG, "Issued request\n");
        }
    }

    bool issueRequest(bool is_write, uint64_t addr = 0) {
        if (addr == 0) addr = cur_addr;

        auto* trans = new tlm::tlm_generic_payload();
        trans->set_command(is_write ? tlm::TLM_WRITE_COMMAND : tlm::TLM_READ_COMMAND);
        trans->set_address(addr);
        trans->set_data_length(4);
        trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        Packet* pkt = new Packet(trans, event_queue->getCurrentCycle(),
                                 is_write ? PKT_REQ_WRITE : PKT_REQ_READ);
        if (!downstream) {
            DPRINTF(TG, "No downstream port!\n");
            return false;
        }
        if (send(pkt)) {
            // 请求发出，等待响应
            return true;
        } else {
            delete pkt;
            DPRINTF(TG, "Backpressure: request not sent\n");
            return false;
        }
    }
public:
    void setDownstream(SimplePort* port) {
        downstream = port;
    }
};
#endif
