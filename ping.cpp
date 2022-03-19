#include <string>
#include <chrono>
#include <iostream>

#include <memory.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

uint16_t inet_cksum(uint16_t *buf, int nwords) {
    unsigned long sum = 0;

    while (nwords-- > 0)
        sum += *buf++;

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return ~sum;
}

int ping(int sock, std::string ip) {
    const int poll_timeout = 1000;

    for (int i = 0; i < 5; i++) {
        auto start = std::chrono::steady_clock::now();

        sockaddr_in addr = { .sin_family = AF_INET };
        if (inet_aton(ip.c_str(), &addr.sin_addr) < 0) {
            std::cerr << "Failed to parse IP-address" << std::endl;
            return -1;
        }

        icmphdr hdr = {
            .type = ICMP_ECHO,
            .code = 0,
            .checksum = 0,
            .un = {
                .echo = {
                    .id = 1,
                    .sequence = (uint16_t)i
                }
            }
        };
        hdr.checksum = inet_cksum((uint16_t *)&hdr, sizeof(icmphdr) / sizeof(uint16_t));

        if (sendto(sock, (void*)&hdr, sizeof(icmphdr), 0, (const sockaddr *)&addr, sizeof(addr)) < 0) {
            std::cerr << "Failed to send ICMP request" << std::endl;
            return -1;
        }

        pollfd fds[] = {
            pollfd{
                .fd = sock,
                .events = POLLIN
            }
        };
        if (poll((pollfd *)&fds, 1, poll_timeout) <= 0) {
            std::cout << "Some shit happened during transimitting the packet" << std::endl;
            return -1;
        }

        icmphdr rcv_hdr;
        socklen_t socklen = sizeof(sockaddr_in);
        if (recvfrom(sock, (void*)&rcv_hdr, sizeof(icmphdr), 0, (sockaddr*)&addr, &socklen) <= 0) {
            std::cerr << "Failed to receive ICMP response" << std::endl;
            return -1;
        }

        auto end = std::chrono::steady_clock::now();
        std::cout << "time=" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Only one argument expected" << std::endl;
        return -1;
    }

    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        std::cerr << "Failed to init ICMP socket" << std::endl;
        return -1;
    }

    int code = ping(sock, std::string(argv[1]));

    close(sock);

    return code;
}
