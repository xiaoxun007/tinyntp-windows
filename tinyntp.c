/*
 * tinyntp - Windows 绯荤粺鏃堕棿鏍″噯宸ュ叿
 * 浠庡叕鍏辨椂闂存湇鍔″櫒鑾峰彇鏍囧噯鏃堕棿锛屾牎鍑嗘湰鍦扮郴缁熸椂閽? *
 * 鍔熻兘璇存槑锛? *   - 鏌ヨ鍏叡鏃堕棿鏈嶅姟鍣ㄨ幏鍙栨爣鍑嗘椂闂? *   - 鏄剧ず褰撳墠绯荤粺鏃堕棿涓庢爣鍑嗘椂闂寸殑宸€? *   - 鍙€夎嚜鍔ㄦ牎鍑嗙郴缁熸椂闂达紙闇€瑕佺鐞嗗憳鏉冮檺锛? *
 * 浣滆€咃細xiaoxun007
 * 鐗堟湰锛?.0.0
 */

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

/* 鏃堕棿鍗忚鐩稿叧甯搁噺 */
#define TIME_PROTO_PORT 123
#define TIME_EPOCH_DELTA 2208988800ULL
#define DEFAULT_TIME_SERVER "time.windows.com"
#define MAX_SERVER_NAME_LEN 256

/* 鏃堕棿鍗忚鏁版嵁鍖呯粨鏋?(48 瀛楄妭) */
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

/* 鏀寔鐨勬椂闂存湇鍔″櫒鍒楄〃 */
typedef struct {
    const char *name;
    const char *description;
} time_server;

static time_server server_list[] = {
    { "time.windows.com", "寰蒋瀹樻柟鏃堕棿鏈嶅姟鍣? },
    { "ntp.aliyun.com", "闃块噷浜戝叕鍏辨椂闂存湇鍔″櫒" },
    { "cn.ntp.org.cn", "涓浗鍥藉鏃堕棿鏈嶅姟鍣? },
    { "pool.ntp.org", "鍏ㄧ悆鍏叡鏃堕棿姹? },
    { NULL, NULL }
};

static void print_banner(void) {
    printf("========================================\n");
    printf("       Windows 绯荤粺鏃堕棿鏍″噯宸ュ叿 v1.0\n");
    printf("       浠庡叕鍏辨椂闂存湇鍔″櫒鑾峰彇鏍囧噯鏃堕棿\n");
    printf("========================================\n");
    printf("\n");
}

static void print_help(const char *prog_name) {
    print_banner();
    printf("鐢ㄦ硶: %s [閫夐」]\n", prog_name);
    printf("\n閫夐」:\n");
    printf("  鏃犲弬鏁?     浣跨敤榛樿鏈嶅姟鍣?(time.windows.com) 鏌ヨ骞舵樉绀烘椂闂村樊\n");
    printf("  -s 鏈嶅姟鍣?  鎸囧畾鏃堕棿鏈嶅姟鍣ㄥ湴鍧€\n");
    printf("  -l          鍒楀嚭鎵€鏈夊彲鐢ㄧ殑鏃堕棿鏈嶅姟鍣╘n");
    printf("  -c          鑷姩鏍″噯绯荤粺鏃堕棿锛堥渶瑕佺鐞嗗憳鏉冮檺锛塡n");
    printf("  -h          鏄剧ず姝ゅ府鍔╀俊鎭痋n");
    printf("\n绀轰緥:\n");
    printf("  %s                    # 鏌ヨ骞舵樉绀烘椂闂村樊\n", prog_name);
    printf("  %s -s ntp.aliyun.com  # 浣跨敤闃块噷浜戞湇鍔″櫒\n", prog_name);
    printf("  %s -c                 # 鑷姩鏍″噯绯荤粺鏃堕棿\n", prog_name);
    printf("\n娉ㄦ剰: 鑷姩鏍″噯绯荤粺鏃堕棿闇€瑕佺鐞嗗憳鏉冮檺杩愯銆俓n");
}

static void list_servers(void) {
    printf("鍙敤鐨勬椂闂存湇鍔″櫒:\n");
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
        printf("閿欒: 缃戠粶鍒濆鍖栧け璐n");
        return -1;
    }
    return 0;
}

static void cleanup_winsock(void) {
    WSACleanup();
}

/* 浠庢椂闂存湇鍔″櫒鑾峰彇鏍囧噯鏃堕棿锛岃繑鍥?time_t 鏍煎紡锛屽け璐ヨ繑鍥?-1 */
static time_t fetch_standard_time(const char *server_name) {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        printf("閿欒: 鏃犳硶鍒涘缓缃戠粶杩炴帴\n");
        return -1;
    }

    /* 璁剧疆鎺ユ敹瓒呮椂 5 绉?*/
    DWORD timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    /* 瑙ｆ瀽鏈嶅姟鍣ㄥ湴鍧€ */
    struct hostent *he = gethostbyname(server_name);
    if (he == NULL) {
        printf("閿欒: 鏃犳硶瑙ｆ瀽鏈嶅姟鍣ㄥ湴鍧€: %s\n", server_name);
        closesocket(sock);
        return -1;
    }

    struct sockaddr_in server_addr;
    ZeroMemory(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TIME_PROTO_PORT);
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    /* 鏋勫缓璇锋眰鍖?*/
    time_packet packet;
    ZeroMemory(&packet, sizeof(packet));
    packet.flags = 0x1B;  /* 瀹㈡埛绔ā寮?*/

    /* 鍙戦€佽姹?*/
    if (sendto(sock, (const char*)&packet, sizeof(packet), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("閿欒: 鍙戦€佽姹傚け璐n");
        closesocket(sock);
        return -1;
    }

    /* 鎺ユ敹鍝嶅簲 */
    int recv_len = recvfrom(sock, (char*)&packet, sizeof(packet), 0, NULL, NULL);
    closesocket(sock);

    if (recv_len == SOCKET_ERROR) {
        printf("閿欒: 鎺ユ敹鍝嶅簲瓒呮椂鎴栧け璐n");
        return -1;
    }

    /* 瑙ｆ瀽鏃堕棿鎴筹紙澶х杞皬绔級 */
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

    /* 瑙ｆ瀽鍛戒护琛屽弬鏁?*/
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

    printf("姝ｅ湪杩炴帴鏃堕棿鏈嶅姟鍣? %s\n", server_name);

    if (init_winsock() != 0) {
        return 1;
    }

    /* 鑾峰彇鏍囧噯鏃堕棿 */
    time_t standard_time = fetch_standard_time(server_name);
    if (standard_time == (time_t)-1) {
        cleanup_winsock();
        return 1;
    }

    cleanup_winsock();

    /* 鑾峰彇褰撳墠绯荤粺鏃堕棿 */
    time_t local_time = time(NULL);

    /* 鏍煎紡鍖栨樉绀?*/
    char time_buf[64];
    format_time(standard_time, time_buf, sizeof(time_buf));
    printf("鏍囧噯鏃堕棿: %s\n", time_buf);

    format_time(local_time, time_buf, sizeof(time_buf));
    printf("绯荤粺鏃堕棿: %s\n", time_buf);

    /* 璁＄畻鏃堕棿宸?*/
    double diff = difftime(standard_time, local_time);
    printf("\n鏃堕棿宸? %.2f 绉?, diff);
    if (diff > 0) {
        printf(" (绯荤粺鏃堕棿鎱簡 %.2f 绉?\n", diff);
    } else if (diff < 0) {
        printf(" (绯荤粺鏃堕棿蹇簡 %.2f 绉?\n", -diff);
    } else {
        printf("\n");
    }

    /* 鏄惁鑷姩鏍″噯 */
    if (do_calibrate) {
        printf("\n姝ｅ湪鏍″噯绯荤粺鏃堕棿...\n");

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
                printf("閿欒: 鏉冮檺涓嶈冻锛佹牎鍑嗙郴缁熸椂闂撮渶瑕佺鐞嗗憳鏉冮檺銆俓n");
                printf("璇峰彸閿懡浠ゆ彁绀虹 -> 浠ョ鐞嗗憳韬唤杩愯锛屽啀鎵ц鏈▼搴忋€俓n");
            } else {
                printf("閿欒: 鏍″噯澶辫触锛岄敊璇爜: %lu\n", err);
            }
            return 1;
        }

        printf("鏍″噯瀹屾垚锛佺郴缁熸椂闂村凡鏇存柊銆俓n");
    } else {
        printf("\n鎻愮ず: 浣跨敤 -c 鍙傛暟鍙嚜鍔ㄦ牎鍑嗙郴缁熸椂闂达紙闇€瑕佺鐞嗗憳鏉冮檺锛塡n");
    }

    return 0;
}
