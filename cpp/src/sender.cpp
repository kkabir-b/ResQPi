#include <opencv2/opencv.hpp>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <cstring>

#define PORT 8080
#define DEST_IP "127.0.0.1" 
#define CHUNK_SIZE 1400     


#pragma pack(push, 1)
struct PacketHeader {
    uint32_t frame_id;
    uint16_t packet_index;
    uint16_t total_packets;
    uint32_t payload_size;
};
#pragma pack(pop)

int main() {
    
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error opening webcam!" << std::endl;
        return -1;
    }

    
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(DEST_IP);

    cv::Mat frame;
    std::vector<uchar> jpeg_buffer;
    
    
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 55}; 
    uint32_t frame_id = 0;

    std::cout << "Streaming chunks to " << DEST_IP << ":" << PORT << "..." << std::endl;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        
        cv::imencode(".jpg", frame, jpeg_buffer, params);
        
        uint32_t total_size = jpeg_buffer.size();
        uint16_t total_packets = (total_size + CHUNK_SIZE - 1) / CHUNK_SIZE;

        
        for (uint16_t i = 0; i < total_packets; ++i) {
            uint32_t offset = i * CHUNK_SIZE;
            uint32_t current_chunk_size = std::min((uint32_t)CHUNK_SIZE, total_size - offset);

            
            PacketHeader header;
            header.frame_id = frame_id;
            header.packet_index = i;
            header.total_packets = total_packets;
            header.payload_size = current_chunk_size;

            
            std::vector<char> packet(sizeof(PacketHeader) + current_chunk_size);
            std::memcpy(packet.data(), &header, sizeof(PacketHeader));
            std::memcpy(packet.data() + sizeof(PacketHeader), jpeg_buffer.data() + offset, current_chunk_size);

            
            sendto(sock, packet.data(), packet.size(), 0,
                   (const struct sockaddr *)&servaddr, sizeof(servaddr));
        }

        frame_id++; 
        
        
        
    }

    close(sock);
    return 0;
}