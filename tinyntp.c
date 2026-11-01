/*
 * tinyntp - Windows System Time Calibration Tool
 *
 * Fetches standard time from public time servers and calibrates
 * the local system clock.
 *
 * Features:
 *   - Query public time servers for standard time
 *   - Show time difference between system and standard time
 *   - Optional auto-calibration (requires admin privileges)
 *
 * Version: 1.0.0
 */

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

/* Protocol constants */
#define TIME_PROTO_PORT 123
#define TIME_EPOCH_DELTA 2208988800ULL
#define DEFAULT_TIME_SERVER "time.windows.com"
#define MAX_SERVER_NAME_LEN 256

/* Time protocol packet structure (48 bytes) */
typedef struct {
    BYTE flags;
    BYTE stratum;
    BYTE poll_interval;
    BYTE precision;
    DWORD root_delay;
    DWORD root_dispersion;
    DWORD reference_id;
    DWORD ref_time_sec;
    DWORD ref_time_frac;
    DWORD orig_time_sec;
    DWORD orig_time_frac;
    DWORD recv_time_sec;
    DWORD recv_time_frac;
    DWORD trans_time_sec;
    DWORD trans_time_frac;
} time_packet;

/* Built-in time server list */
typedef struct {
    const char *name;
    const char *description;
} time_server;

static time_server server_list[] = {
    { "time.windows.com", "Microsoft official time server" },
    { "ntp.aliyun.com",   "Alibaba Cloud public time server" },
    { "cn.ntp.org.cn",    "China national time server" },
    { "pool.ntp.org",     "Global public time pool" },
    { NULL, NULL }
};

static void print_banner(void) {
    printf("========================================\n");
    printf("   Windows System Time Calibration v1.0\n");
    printf("   Fetch standard time from public servers\n");
    printf("========================================\n");
    printf("\n");
}

static void print_help(const char *prog_name) {
    print_banner();
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("\nOptions:\n");
    printf("  (no args)    Query and show time difference using default server\n");
    printf("  -s SERVER    Specify time server address\n");
    printf("  -l           List all available time servers\n");
    printf("  -c           Auto-calibrate system time (requires admin)\n");
    printf("  -h           Show this help message\n");
    printf("\nExamples:\n");
    printf("  %s                    # Query and show difference\n", prog_name);
    printf("  %s -s ntp.aliyun.com  # Use Alibaba Cloud server\n", prog_name);
    printf("  %s -c                 # Auto-calibrate system time\n", prog_name);
    printf("\nNote: Auto-calibrate requires Administrator privileges.\n");
}

static void list_servers(void) {
    printf("Available time servers:\n");
    printf("\n");
    int i = 0;
    while (server_list[i].name != NULL) {
        printf("  %-25s  %s\n", server_list[i].name, server_list[i].description);
        i++;
    }
    printf("\n");
}

static int init_winsock(void) {
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("Error: Network initialization failed\n");
        return -1;
    }
    return 0;
}

static void cleanup_winsock(void) {
    WSACleanup();
}

/* Fetch standard time from server. Returns time_t, -1 on failure. */
static time_t fetch_standard_time(const char *server_name) {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        printf("Error: Cannot create network socket\n");
        return -1;
    }

    /* Set receive timeout to 5 seconds */
    DWORD timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    /* Resolve server address */
    struct hostent *he = gethostbyname(server_name);
    if (he == NULL) {
        printf("Error: Cannot resolve server: %s\n", server_name);
        closesocket(sock);
        return -1;
    }

    struct sockaddr_in server_addr;
    ZeroMemory(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TIME_PROTO_PORT);
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    /* Build request packet */
    time_packet packet;
    ZeroMemory(&packet, sizeof(packet));
    packet.flags = 0x1B;  /* Client mode */

    /* Send request */
    if (sendto(sock, (const char*)&packet, sizeof(packet), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Error: Failed to send request\n");
        closesocket(sock);
        return -1;
    }

    /* Receive response */
    int recv_len = recvfrom(sock, (char*)&packet, sizeof(packet), 0, NULL, NULL);
    closesocket(sock);

    if (recv_len == SOCKET_ERROR) {
        printf("Error: Receive timeout or failed\n");
        return -1;
    }

    /* Parse timestamp (big-endian to host) */
    DWORD trans_sec = ntohl(packet.trans_time_sec);
    time_t standard_time = (time_t)(trans_sec - TIME_EPOCH_DELTA);

    return standard_time;
}

static void format_time(time_t t, char *buf, size_t buf_len) {
    struct tm *tm_info = localtime(&t);
    strftime(buf, buf_len, "%Y-%m-%d %H:%M:%S", tm_info);
}

int main(int argc, char *argv[]) {
    const char *server_name = DEFAULT_TIME_SERVER;
    int do_calibrate = 0;

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-l") == 0) {
            print_banner();
            list_servers();
            return 0;
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            server_name = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0) {
            do_calibrate = 1;
        }
    }

    print_banner();

    printf("Connecting to time server: %s\n", server_name);

    if (init_winsock() != 0) {
        return 1;
    }

    /* Get standard time */
    time_t standard_time = fetch_standard_time(server_name);
    if (standard_time == (time_t)-1) {
        cleanup_winsock();
        return 1;
    }

    cleanup_winsock();

    /* Get local system time */
    time_t local_time = time(NULL);

    /* Format and display */
    char time_buf[64];
    format_time(standard_time, time_buf, sizeof(time_buf));
    printf("Standard time: %s\n", time_buf);

    format_time(local_time, time_buf, sizeof(time_buf));
    printf("System time:   %s\n", time_buf);

    /* Calculate time difference */
    double diff = difftime(standard_time, local_time);
    printf("\nTime difference: %.2f seconds", diff);
    if (diff > 0) {
        printf(" (system clock is slow by %.2f s)\n", diff);
    } else if (diff < 0) {
        printf(" (system clock is fast by %.2f s)\n", -diff);
    } else {
        printf("\n");
    }

    /* Auto calibrate if requested */
    if (do_calibrate) {
        printf("\nCalibrating system time...\n");

        struct tm *tm_info = localtime(&standard_time);
        SYSTEMTIME st;
        ZeroMemory(&st, sizeof(st));
        st.wYear = tm_info->tm_year + 1900;
        st.wMonth = tm_info->tm_mon + 1;
        st.wDay = tm_info->tm_mday;
        st.wHour = tm_info->tm_hour;
        st.wMinute = tm_info->tm_min;
        st.wSecond = tm_info->tm_sec;
        st.wMilliseconds = 0;

        if (!SetLocalTime(&st)) {
            DWORD err = GetLastError();
            if (err == ERROR_ACCESS_DENIED) {
                printf("Error: Access denied!\n");
                printf("Calibrating system time requires Administrator privileges.\n");
                printf("Please run Command Prompt as Administrator, then run this tool.\n");
            } else {
                printf("Error: Calibration failed, error code: %lu\n", err);
            }
            return 1;
        }

        printf("Calibration complete! System time updated.\n");
    } else {
        printf("\nTip: Use -c flag to auto-calibrate system time (requires admin)\n");
    }

    return 0;
}
