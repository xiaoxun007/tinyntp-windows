# tinyntp - 鏈€杞婚噺 Windows NTP 鏃堕棿鍚屾宸ュ叿

鍗曟枃浠?C 瀹炵幇锛岀紪璇戝悗浠呭嚑鍗?KB锛岀敤鏉ュ揩閫熷悓姝?Windows 绯荤粺鏃堕棿銆?
## 鍔熻兘

- 浠?NTP 鏈嶅姟鍣ㄨ幏鍙栧噯纭椂闂?- 涓€閿悓姝ョ郴缁熸椂闂?- 鏀寔鑷畾涔?NTP 鏈嶅姟鍣?- 鍗?exe锛屾棤闇€瀹夎锛岀豢鑹蹭究鎼?
## 鐢ㄦ硶

```bash
# 鐢ㄩ粯璁ゆ湇鍔″櫒 (pool.ntp.org) 鍚屾
tinyntp.exe

# 鎸囧畾 NTP 鏈嶅姟鍣?tinyntp.exe ntp.aliyun.com

# 鏄剧ず甯姪
tinyntp.exe -h
```

> **娉ㄦ剰锛?* 鍚屾绯荤粺鏃堕棿闇€瑕?*绠＄悊鍛樻潈闄?*銆?> 璇峰彸閿懡浠ゆ彁绀虹 鈫?浠ョ鐞嗗憳韬唤杩愯锛屽啀鎵ц鏈▼搴忋€?
## 缂栬瘧

鏈」鐩娇鐢?GitHub Actions 鑷姩缂栬瘧锛屼篃鍙湰鍦扮敤 MinGW 缂栬瘧锛?
```bash
gcc -O2 -o tinyntp.exe tinyntp.c -lws2_32 -s
```

## 榛樿 NTP 鏈嶅姟鍣?
榛樿浣跨敤 `pool.ntp.org`锛屽浗鍐呮帹鑽愪娇鐢細
- `ntp.aliyun.com` (闃块噷浜?
- `cn.ntp.org.cn` (涓浗 NTP 姹?
- `time.windows.com` (寰蒋)
