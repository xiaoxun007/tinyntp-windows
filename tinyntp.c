/*
 * tinyntp - Windows System Time Calibration Tool
 * v1.0.1 - China optimized, slim build
 */

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define NTP_PORT 123
#define NTP_DELTA 2208988800ULL
#define TIMEOUT_MS 8000

/* Built-in server list, try in order */
static const char *servers[] = {
    "ntp.aliyun.com",
    "cn.ntp.org.cn",
    "time.windows.com",
    NULL
};

typedef struct {
    BYTE flags;
    BYTE stratum;
    BYTE poll;
    BYTE precision;
    DWORD root_delay;
    DWORD root_disp;
    DWORD ref_id;
    DWORD ref_s, ref_f;
    DWORD orig_s, orig_f;
    DWORD rx_s, rx_f;
    DWORD tx_s, tx_f;
} ntp_pkt;

static int init_net(void) {
    WSADATA w;
    return WSAStartup(MAKEWORD(2,2), &w);
}

static time_t query_server(const char *host) {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return -1;

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&(DWORD){TIMEOUT_MS}, sizeof(DWORD));

    struct hostent *he = gethostbyname(host);
    if (!he) { closesocket(s); return -1; }

    struct sockaddr_in addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(NTP_PORT);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    ntp_pkt pkt;
    ZeroMemory(&pkt, sizeof(pkt));
    pkt.flags = 0x1B;

    sendto(s, (const char*)&pkt, sizeof(pkt), 0, (struct sockaddr*)&addr, sizeof(addr));
    int n = recvfrom(s, (char*)&pkt, sizeof(pkt), 0, NULL, NULL);
    closesocket(s);

    if (n <= 0) return -1;
    return (time_t)(ntohl(pkt.tx_s) - NTP_DELTA);
}

int main(int argc, char *argv[]) {
    const char *custom = NULL;
    int calibrate = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h")) {
            printf("tinyntp - Windows time calibration\n");
            printf("Usage: tinyntp [-s server] [-c]\n");
            printf("  -s SERVER   Use custom NTP server\n");
            printf("  -c          Calibrate system time (needs admin)\n");
            printf("  -h          Show this help\n");
            return 0;
        } else if (!strcmp(argv[i], "-s") && i+1 < argc) {
            custom = argv[++i];
        } else if (!strcmp(argv[i], "-c")) {
            calibrate = 1;
        }
    }

    printf("tinyntp - Time Calibration Tool\n");
    printf("================================\n\n");

    if (init_net() != 0) { printf("Network init failed\n"); return 1; }

    time_t std_time = -1;
    if (custom) {
        printf("Querying: %s\n", custom);
        std_time = query_server(custom);
        if (std_time == -1) printf("Failed to reach %s\n", custom);
    } else {
        for (int i = 0; servers[i]; i++) {
            printf("Trying: %s ... ", servers[i]);
            std_time = query_server(servers[i]);
            if (std_time != -1) { printf("OK\n"); break; }
            printf("timeout\n");
        }
    }

    WSACleanup();

    if (std_time == -1) {
        printf("\nAll servers failed. Try -s with a custom server.\n");
        return 1;
    }

    time_t local = time(NULL);
    double diff = difftime(std_time, local);

    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&std_time));
    printf("Standard: %s\n", buf);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&local));
    printf("System:   %s\n", buf);
    printf("Diff: %.2fs\n", diff);

    if (calibrate) {
        printf("\nCalibrating...\n");
        struct tm *t = localtime(&std_time);
        SYSTEMTIME st;
        ZeroMemory(&st, sizeof(st));
        st.wYear = t->tm_year + 1900;
        st.wMonth = t->tm_mon + 1;
        st.wDay = t->tm_mday;
        st.wHour = t->tm_hour;
        st.wMinute = t->tm_min;
        st.wSecond = t->tm_sec;
        if (!SetLocalTime(&st)) {
            printf("Failed. Need admin privileges.\n");
            return 1;
        }
        printf("Done!\n");
    }

    return 0;
}
