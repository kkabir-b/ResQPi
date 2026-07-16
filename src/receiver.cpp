#include <opencv2/opencv.hpp>
#include "inferrer/Inferrer.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <cstring>


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
    //0. Set up object detection model
    std::string modelPath = "models/YOLOv10n_gestures.onnx"; 
    Inferrer yolo(modelPath.c_str());
    // 1. Set up UDP Socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    // CRITICAL FIX: Crank up the OS receive buffer to 2MB to prevent dropped packets under load
    int rcvbuf = 2 * 1024 * 1024; 
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    struct sockaddr_in servaddr, cliaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces
    servaddr.sin_port = htons(PORT);

    if (bind(sock, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "Bind failed!" << std::endl;
        return -1;
    }

    std::vector<char> recv_buf(MAX_PACKET_SIZE);
    socklen_t len = sizeof(cliaddr);
    
    // Sliding window map to track multiple frames arriving simultaneously
    std::map<uint32_t, FrameAssembly> frame_map;
    uint32_t last_displayed_frame_id = 0;

    std::cout << "Listening for chunked video on port " << PORT << "..." << std::endl;

    while (true) {
        // 2. Receive packet
        int n = recvfrom(sock, recv_buf.data(), MAX_PACKET_SIZE, 0, (struct sockaddr *)&cliaddr, &len);
        if (n < (int)sizeof(PacketHeader)) continue; // Ignore corrupted/tiny packets

        // 3. Parse Header
        PacketHeader header;
        std::memcpy(&header, recv_buf.data(), sizeof(PacketHeader));

        // Ignore packets for frames older than what we've already displayed
        if (header.frame_id < last_displayed_frame_id) {
            continue;
        }

        // 4. Initialize this frame in our map if we haven't seen it yet
        if (frame_map.find(header.frame_id) == frame_map.end()) {
            // Prevent map from growing infinitely if many packets are lost
            if (frame_map.size() >= MAX_ACTIVE_FRAMES) {
                frame_map.erase(frame_map.begin()); // Erase the oldest tracked frame
            }
            frame_map[header.frame_id].init(header.total_packets);
        }

        FrameAssembly& assembly = frame_map[header.frame_id];

        // 5. Store the chunk payload data
        if (header.packet_index < assembly.total_packets && !assembly.received_mask[header.packet_index]) {
            assembly.received_mask[header.packet_index] = true;
            assembly.received_count++;
            
            const char* payload_ptr = recv_buf.data() + sizeof(PacketHeader);
            // Notice we only read exactly `header.payload_size` bytes (No padding issues!)
            assembly.chunks[header.packet_index].assign(payload_ptr, payload_ptr + header.payload_size);
        }

        // 6. If this frame is now 100% complete, decode and show it!
        if (assembly.received_count == assembly.total_packets) {
            
            // Stitch chunks together into one JPEG buffer
            std::vector<uchar> jpeg_buffer;
            for (const auto& chunk : assembly.chunks) {
                jpeg_buffer.insert(jpeg_buffer.end(), chunk.begin(), chunk.end());
            }

            // Decode the JPEG
            cv::Mat frame = cv::imdecode(jpeg_buffer, cv::IMREAD_COLOR);
            if (!frame.empty()) {
                yolo.runInfer(frame);
            }

            last_displayed_frame_id = header.frame_id;

            // 7. Clean up the map (Erase this frame and any stale incomplete frames older than it)
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

    close(sock);
    return 0;
}