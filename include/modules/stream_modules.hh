// include/modules/stream_modules.hh
#ifndef STREAM_MODULES_HH
#define STREAM_MODULES_HH

#include "../sim_object.hh"
#include "../simple_port.hh"
#include "../packet.hh"
#include <queue>

class StreamProducer : public SimObject, public SimplePort {
private:
    uint64_t counter = 0;
    uint64_t stream_id = 1;

public:
    StreamProducer(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}

    void tick() override {
        if (event_queue->getCurrentCycle() % 5 == 0) {
            auto* trans = new tlm::tlm_generic_payload();
            trans->set_command(tlm::TLM_WRITE_COMMAND);
            uint32_t data = counter++;
            trans->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
            trans->set_data_length(4);
            trans->set_address(0x2000 + counter);

            Packet* pkt = new Packet(trans, event_queue->getCurrentCycle(), PKT_STREAM_DATA);
            pkt->stream_id = stream_id;

            if (!send(pkt)) {
                DPRINTF(STREAM, "StreamProducer: Backpressure! Packet dropped.\n");
                delete pkt;
            } else {
                DPRINTF(STREAM, "StreamProducer: Sent stream data %u\n", data);
            }
        }
    }

    bool send(Packet* pkt) override {
        if (pair) {
            SimplePort* other = (my_side == 0) ? pair->side_b : pair->side_a;
            return other->recv(pkt);
        }
        return false;
    }

    bool recv(Packet*) override { return false; } // 不接收响应
};

class StreamConsumer : public SimObject, public SimplePort {
private:
    std::queue<Packet*> buffer;
    static const int MAX_BUFFER = 2;

public:
    StreamConsumer(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}

    bool recv(Packet* pkt) override {
        if (buffer.size() >= MAX_BUFFER) {
            DPRINTF(STREAM, "StreamConsumer: Buffer full! Applying backpressure.\n");
            return false;
        }
        buffer.push(pkt);
        DPRINTF(STREAM, "StreamConsumer: Accepted stream packet\n");
        return true;
    }

    void tick() override {
        if (!buffer.empty()) {
            Packet* pkt = buffer.front(); buffer.pop();
            uint32_t* data = (uint32_t*)pkt->payload->get_data_ptr();
            DPRINTF(STREAM, "StreamConsumer: Processed stream data = %u\n", *data);
            delete pkt;
        }
    }

    bool send(Packet*) override { return false; }
};
/*
通信模式：单向流（Stream） 

StreamProducer 定期发送数据。
StreamConsumer 使用缓冲区实现 Buffer-based 反压。
若缓冲区满，recv() 返回 false，上游可丢弃或重试。
*/
#endif // STREAM_MODULES_HH
