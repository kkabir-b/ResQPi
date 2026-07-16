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
#define MAX_PACKET_SIZE 2000 // Big enough to hold our header + CHUNK_SIZE
#define MAX_ACTIVE_FRAMES 5  // How many incomplete frames we track at once (Sliding Window)

// Force the compiler to pack this struct with zero padding (exactly 12 bytes)
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t frame_id;
    uint16_t packet_index;
    uint16_t total_packets;
    uint32_t payload_size;
};
#pragma pack(pop)

// Structure to track the reassembly of a single frame
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
    // 0. Set up object detection model
    std::string modelPath = "models/YOLOv10n_gestures.onnx"; 
    Inferrer yolo(modelPath.c_str());

    // --- NEW: Threading Primitives ---
    std::queue<std::vector<uchar>> inference_queue;
    std::mutex queue_mtx;
    std::condition_variable queue_cv;
    std::atomic<bool> keep_running{true};

    // --- NEW: The Background Inference Thread ---
    std::thread worker_thread([&]() {
        while (keep_running) {
            std::vector<uchar> jpeg_buffer;

            // 1. Wait until a frame is in the queue
            {
                std::unique_lock<std::mutex> lock(queue_mtx);
                // Sleep thread until the queue has data (uses zero CPU while waiting)
                queue_cv.wait(lock, [&]{ return !inference_queue.empty() || !keep_running; });

                if (!keep_running && inference_queue.empty()) break;

                // Grab the oldest frame and remove it from the queue
                jpeg_buffer = std::move(inference_queue.front());
                inference_queue.pop();
            } // Lock is automatically released here!

            // 2. Decode and run inference (Heavy lifting is now off the network thread!)
            cv::Mat frame = cv::imdecode(jpeg_buffer, cv::IMREAD_COLOR);
            if (!frame.empty()) {
                yolo.runInfer(frame);
            }
        }
    });

    // 1. Set up UDP Socket
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
        // 2. Receive packet
        int n = recvfrom(sock, recv_buf.data(), MAX_PACKET_SIZE, 0, (struct sockaddr *)&cliaddr, &len);
        if (n < (int)sizeof(PacketHeader)) continue; 

        // 3. Parse Header
        PacketHeader header;
        std::memcpy(&header, recv_buf.data(), sizeof(PacketHeader));

        if (header.frame_id < last_displayed_frame_id) continue;

        // 4. Initialize frame
        if (frame_map.find(header.frame_id) == frame_map.end()) {
            if (frame_map.size() >= MAX_ACTIVE_FRAMES) {
                frame_map.erase(frame_map.begin()); 
            }
            frame_map[header.frame_id].init(header.total_packets);
        }

        FrameAssembly& assembly = frame_map[header.frame_id];

        // 5. Store chunk
        if (header.packet_index < assembly.total_packets && !assembly.received_mask[header.packet_index]) {
            assembly.received_mask[header.packet_index] = true;
            assembly.received_count++;
            
            const char* payload_ptr = recv_buf.data() + sizeof(PacketHeader);
            assembly.chunks[header.packet_index].assign(payload_ptr, payload_ptr + header.payload_size);
        }

        // --- MODIFIED: Step 6 & 7 ---
        if (assembly.received_count == assembly.total_packets) {
            
            // Stitch chunks (This is very fast memory copying, safe for main thread)
            std::vector<uchar> jpeg_buffer;
            for (const auto& chunk : assembly.chunks) {
                jpeg_buffer.insert(jpeg_buffer.end(), chunk.begin(), chunk.end());
            }

            // Hand the finished JPEG off to the inference thread
            {
                std::lock_guard<std::mutex> lock(queue_mtx);
                
                // CRITICAL FOR REAL-TIME: 
                // If YOLO is slower than the network, the queue will grow infinitely and lag.
                // By forcing the queue size to stay at 2, we drop stale frames and ensure 
                // YOLO is always inferencing the most recent reality.
                if (inference_queue.size() >= 2) {
                    inference_queue.pop(); 
                }
                
                // std::move transfers memory ownership instantly without copying data
                inference_queue.push(std::move(jpeg_buffer)); 
            }
            queue_cv.notify_one(); // Wake up the worker thread!

            last_displayed_frame_id = header.frame_id;

            // 7. Clean up the map (Kept on main thread to avoid blocking network loop)
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

    // Cleanup (though this while(true) loop never actually reaches here)
    keep_running = false;
    queue_cv.notify_one();
    worker_thread.join();
    close(sock);
    return 0;
}