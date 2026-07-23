#include <opencv2/opencv.hpp>
#include "inferrer/Inferrer.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>


#define PORT 8080
#define MAX_PACKET_SIZE 2000 
#define MAX_ACTIVE_FRAMES 5 


#pragma pack(push, 1)
struct PacketHeader {
    uint32_t frame_id;
    uint16_t packet_index;
    uint16_t total_packets;
    uint32_t payload_size;
};
#pragma pack(pop)


struct FrameAssembly {
    uint16_t total_packets = 0;
    uint16_t received_count = 0;
    std::vector<std::vector<char>> chunks;
    std::vector<bool> received_mask;

    void init(uint16_t total) {
        total_packets = total;
        received_count = 0;
        chunks.assign(total, std::vector<char>());
        received_mask.assign(total, false);
    }
};

int main() {
    
    std::string modelPath = "models/YOLOv10n_gestures.onnx"; 
    Inferrer yolo(modelPath.c_str());

    
    std::queue<std::vector<uchar>> inference_queue;
    std::mutex queue_mtx;
    std::condition_variable queue_cv;
    std::atomic<bool> keep_running{true};

    
    std::thread worker_thread([&]() {
        while (keep_running) {
            std::vector<uchar> jpeg_buffer;

            
            {
                std::unique_lock<std::mutex> lock(queue_mtx);
                
                queue_cv.wait(lock, [&]{ return !inference_queue.empty() || !keep_running; });

                if (!keep_running && inference_queue.empty()) break;

                
                jpeg_buffer = std::move(inference_queue.front());
                inference_queue.pop();
            } 

            
            cv::Mat frame = cv::imdecode(jpeg_buffer, cv::IMREAD_COLOR);
            if (!frame.empty()) {
                yolo.runInfer(frame);
            }
        }
    });

    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    int rcvbuf = 2 * 1024 * 1024; 
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    struct sockaddr_in servaddr, cliaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT);

    if (bind(sock, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "Bind failed!" << std::endl;
        return -1;
    }

    std::vector<char> recv_buf(MAX_PACKET_SIZE);
    socklen_t len = sizeof(cliaddr);
    
    std::map<uint32_t, FrameAssembly> frame_map;
    uint32_t last_displayed_frame_id = 0;

    std::cout << "Listening for chunked video on port " << PORT << "..." << std::endl;

    while (true) {
        
        int n = recvfrom(sock, recv_buf.data(), MAX_PACKET_SIZE, 0, (struct sockaddr *)&cliaddr, &len);
        if (n < (int)sizeof(PacketHeader)) continue; 

        
        PacketHeader header;
        std::memcpy(&header, recv_buf.data(), sizeof(PacketHeader));

        if (header.frame_id < last_displayed_frame_id) continue;

        
        if (frame_map.find(header.frame_id) == frame_map.end()) {
            if (frame_map.size() >= MAX_ACTIVE_FRAMES) {
                frame_map.erase(frame_map.begin()); 
            }
            frame_map[header.frame_id].init(header.total_packets);
        }

        FrameAssembly& assembly = frame_map[header.frame_id];

        
        if (header.packet_index < assembly.total_packets && !assembly.received_mask[header.packet_index]) {
            assembly.received_mask[header.packet_index] = true;
            assembly.received_count++;
            
            const char* payload_ptr = recv_buf.data() + sizeof(PacketHeader);
            assembly.chunks[header.packet_index].assign(payload_ptr, payload_ptr + header.payload_size);
        }

        
        if (assembly.received_count == assembly.total_packets) {
            
           
            std::vector<uchar> jpeg_buffer;
            for (const auto& chunk : assembly.chunks) {
                jpeg_buffer.insert(jpeg_buffer.end(), chunk.begin(), chunk.end());
            }

            
            {
                std::lock_guard<std::mutex> lock(queue_mtx);
                
            
                if (inference_queue.size() >= 2) {
                    inference_queue.pop(); 
                }
                
                
                inference_queue.push(std::move(jpeg_buffer)); 
            }
            queue_cv.notify_one(); // Wake up the worker thread!

            last_displayed_frame_id = header.frame_id;

            
            auto it = frame_map.begin();
            while (it != frame_map.end()) {
                if (it->first <= last_displayed_frame_id) {
                    it = frame_map.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    
    keep_running = false;
    queue_cv.notify_one();
    worker_thread.join();
    close(sock);
    return 0;
}