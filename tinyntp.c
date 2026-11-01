/*
 * tinyntp - 鏈€杞婚噺鐨?Windows NTP 鏃堕棿鍚屾宸ュ叿
 * 鍗曟枃浠?C 瀹炵幇锛岀紪璇戝悗浠呭嚑鍗?KB
 *
 * 鐢ㄦ硶:
 *   tinyntp                # 鐢ㄩ粯璁?NTP 鏈嶅姟鍣ㄥ悓姝? *   tinyntp ntp.aliyun.com # 鎸囧畾 NTP 鏈嶅姟鍣? *   tinyntp -h             # 鏄剧ず甯姪
 */

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define NTP_TIMESTAMP_DELTA 2208988800ull  /* 1900-01-01 鍒?1970-01-01 鐨勭鏁?*/
#define NTP_PORT 123
#define DEFAULT_NTP_SERVER "pool.ntp.org"

/* NTP 鏁版嵁鍖呯粨鏋?(48 瀛楄妭) */
typedef struct {
    BYTE li_vn_mode;    /* 2 bit Leap, 3 bit Version, 3 bit Mode */
    BYTE stratum;
    BYTE poll;
    BYTE precision;
    DWORD rootDelay;
    DWORD rootDispersion;
    DWORD refId;
    DWORD refTm_s;      /* 鍙傝€冩椂闂?- 绉?*/
    DWORD refTm_f;      /* 鍙傝€冩椂闂?- 姣 */
    DWORD origTm_s;    /*  originate 鏃堕棿 - 绉?*/
    DWORD origTm_f;
    DWORD rxTm_s;       /* 鎺ユ敹鏃堕棿 - 绉?*/
    DWORD rxTm_f;
    DWORD txTm_s;       /* 鍙戦€佹椂闂?- 绉?(鏈€鍏抽敭) */
    DWORD txTm_f;
} ntp_packet;

static void print_help(const char *prog) {
    printf("tinyntp - Windows NTP 鏃堕棿鍚屾宸ュ叿\n");
    printf("鐢ㄦ硶: %s [NTP鏈嶅姟鍣╙\n", prog);
    printf("\n閫夐」:\n");
    printf("  鏃犲弬鏁?   浣跨敤榛樿鏈嶅姟鍣?pool.ntp.org 鍚屾\n");
    printf("  鏈嶅姟鍣?   鎸囧畾 NTP 鏈嶅姟鍣ㄥ湴鍧€ (濡?ntp.aliyun.com)\n");
    printf("  -h        鏄剧ず姝ゅ府鍔‐n");
    printf("\n绀轰緥:\n");
    printf("  %s\n", prog);
    printf("  %s ntp.aliyun.com\n", prog);
}

int main(int argc, char *argv[]) {
    const char *server = DEFAULT_NTP_SERVER;

    /* 瑙ｆ瀽鍛戒护琛屽弬鏁?*/
    if (argc >= 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        server = argv[1];
    }

    printf("tinyntp - 姝ｅ湪浠?%s 鑾峰彇鏃堕棿...\n", server);

    /* 鍒濆鍖?Winsock */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("閿欒: WSAStartup 澶辫触\n");
        return 1;
    }

    /* 鍒涘缓 UDP socket */
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        printf("閿欒: 鍒涘缓 socket 澶辫触\n");
        WSACleanup();
        return 1;
    }

    /* 璁剧疆瓒呮椂 5 绉?*/
    DWORD timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    /* 瑙ｆ瀽鏈嶅姟鍣ㄥ湴鍧€ */
    struct hostent *he = gethostbyname(server);
    if (he == NULL) {
        printf("閿欒: 鏃犳硶瑙ｆ瀽涓绘満鍚?%s\n", server);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_addr;
    ZeroMemory(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(NTP_PORT);
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    /* 鏋勫缓 NTP 璇锋眰鍖?*/
    ntp_packet packet;
    ZeroMemory(&packet, sizeof(packet));
    packet.li_vn_mode = 0x1B;  /* li=0, vn=3, mode=3 (client) */

    /* 鍙戦€佽姹?*/
    if (sendto(sock, (char*)&packet, sizeof(packet), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("閿欒: 鍙戦€?NTP 璇锋眰澶辫触\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    /* 鎺ユ敹鍝嶅簲 */
    int recv_len = recvfrom(sock, (char*)&packet, sizeof(packet), 0, NULL, NULL);
    closesocket(sock);
    WSACleanup();

    if (recv_len == SOCKET_ERROR) {
        printf("閿欒: 鎺ユ敹 NTP 鍝嶅簲瓒呮椂鎴栧け璐n");
        return 1;
    }

    /* 瑙ｆ瀽鏃堕棿 - NTP 鏃堕棿鎴虫槸澶х搴?*/
    DWORD txTm_s = ntohl(packet.txTm_s);
    time_t ntp_time = (time_t)(txTm_s - NTP_TIMESTAMP_DELTA);

    /* 杞崲涓烘湰鍦版椂闂村瓧绗︿覆 */
    struct tm *tm_info = localtime(&ntp_time);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("NTP 鏈嶅姟鍣ㄦ椂闂? %s\n", time_str);

    /* 鍚屾鍒扮郴缁熸椂闂?*/
    SYSTEMTIME st;
    ZeroMemory(&st, sizeof(st));
    st.wYear = tm_info->tm_year + 1900;
    st.wMonth = tm_info->tm_mon + 1;
    st.wDay = tm_info->tm_mday;
    st.wHour = tm_info->tm_hour;
    st.wMinute = tm_info->tm_min;
    st.wSecond = tm_info->tm_sec;
    st.wMilliseconds = 0;

    /* 闇€瑕佺鐞嗗憳鏉冮檺鎵嶈兘璁剧疆绯荤粺鏃堕棿 */
    if (!SetLocalTime(&st)) {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED) {
            printf("\n閿欒: 鏉冮檺涓嶈冻锛乗n");
            printf("鍚屾绯荤粺鏃堕棿闇€瑕佺鐞嗗憳鏉冮檺銆俓n");
            printf("璇峰彸閿偣鍑诲懡浠ゆ彁绀虹 -> 浠ョ鐞嗗憳韬唤杩愯锛屽啀鎵ц鏈▼搴忋€俓n");
        } else {
            printf("\n閿欒: SetLocalTime 澶辫触锛岄敊璇爜: %lu\n", err);
        }
        return 1;
    }

    printf("\n绯荤粺鏃堕棿鍚屾鎴愬姛锛乗n");
    return 0;
}
