//-------------------------------------------------------------
//这一版是基于 ZLG 官方提供的 DBC 异步解析示例代码进行修改的，主要改动如下：

//#include "stdio.h"
//#include <iostream>
//#include <string.h>
//#include <thread>
//#include <windows.h>
//#include "zdbc.h"
//#include "zlgcan.h"

//#define FUNCCALL __stdcall
//using namespace std;
//
//int g_thd_run = 1;

// =========================================================================
// 1. 【实车高频防卡死版】异步解析成功后的回调函数
// =========================================================================
//void CALLBACK MyOnAsyncAnalyseDone(void* ctx, DBCMessage* pMsg, uint64 extraData) {
//    // 【 1：ID 白名单过滤】
//    // 实车总线上有几十种报文，这里只放行真正关心的 ID (比如: 0x302电池, 0x304车速)
//    if (pMsg->nID != 0x302 && pMsg->nID != 0x304) {
//        return;
//    }
//
//    // 【 2：时间节流防刷屏 (Throttling)】
//    // 实车 0x304 报文可能每 10ms 就发一次(1秒100次)。为了不让 printf 把 CPU 卡死，
//    // 我们利用静态变量控制：每隔 500ms (0.5秒) 才往控制台渲染一次最新数据！
//    static uint64 last_print_time_302 = 0;
//    static uint64 last_print_time_304 = 0;
//    uint64 current_time = GetTickCount64();
//
//    if (pMsg->nID == 0x302) {
//        if (current_time - last_print_time_302 < 500) return; // 距离上次打印不足 500ms，直接放行不打印
//        last_print_time_302 = current_time;
//    }
//    if (pMsg->nID == 0x304) {
//        if (current_time - last_print_time_304 < 500) return;
//        last_print_time_304 = current_time;
//    }
//
//    // --- 能够走到这里，说明是目标 ID 且满足了 500ms 的时间节流 ---
//    printf("\n>>> [实车捕获] 时间戳:%llu | ID: 0x%X (%s) <<<\n", extraData, pMsg->nID, pMsg->strName);
//    printf("------------------------------------------------------------------\n");
//    printf(" 信号名称                      起始位  长度  原始值      实际物理值\n");
//    printf("------------------------------------------------------------------\n");
//
//    for (int i = 0; i < (int)pMsg->nSignalCount; ++i) {
//        uint64 rawVal = pMsg->vSignals[i].nRawValue;
//        // 调用 ZDBC_CalcActualValue 自动把底层的 Raw 换算为真实物理值 (km/h, V, A等)
//        double actualVal = ZDBC_CalcActualValue(&pMsg->vSignals[i], &rawVal);
//
//        printf(" %-30s %-7d %-5d %-10llu %-10.2f %s\n",
//            pMsg->vSignals[i].strName,
//            pMsg->vSignals[i].nStartBit,
//            pMsg->vSignals[i].nLen,
//            rawVal,
//            actualVal,
//            pMsg->vSignals[i].unit);
//    }
//    printf("------------------------------------------------------------------\n");
//}
//
//// =========================================================================
//// 2. 后台接收线程：只管无脑极速收包，并甩锅给 DBC 异步引擎
//// =========================================================================
//void thread_receive_task(CHANNEL_HANDLE chn, DBCHandle dbc_handle) {
//    ZCAN_Receive_Data canData[100] = {};
//
//    while (g_thd_run) {
//        // 读取驱动 FIFO 接收缓存
//        if (ZCAN_GetReceiveNum(chn, 0)) {
//            uint32_t len = ZCAN_Receive(chn, canData, 100, 20);
//            for (uint32_t i = 0; i < len; i++) {
//                // 【绝招】在接收线程里，绝不要写任何 if-else 业务判断，也不要写 printf！
//                // 把硬件收来的 can_frame 数据瞬间扔进 DBC 异步解析黑盒，线程立刻回头去收下一帧！
//                ZDBC_AsyncAnalyse(dbc_handle, &canData[i].frame, FT_CAN, canData[i].timestamp);
//            }
//        }
//        Sleep(1); // 仅仅休眠 1ms，极速轮询防溢出
//    }
//}
//
//// =========================================================================
//// 3. 设备初始化 (纯静默监听模式)
//// =========================================================================
//int StartDevice(DEVICE_HANDLE& dev, CHANNEL_HANDLE& chn, std::thread& thd_handle, DBCHandle dbc_handle) {
//    dev = ZCAN_OpenDevice(ZCAN_USBCAN_4E_U, 0, 0);
//    if (INVALID_DEVICE_HANDLE == dev) {
//        printf("打开 USBCAN-4E-U 设备失败！请确认 USB 没掉线。\n");
//        return -1;
//    }
//
//    IProperty* property = GetIProperty(dev);
//    if (property->SetValue("0/baud_rate", "500000") != STATUS_OK) {
//        printf("设置波特率失败！请确认通道未被 ZCANPRO 独占。\n");
//        ReleaseIProperty(property);
//        ZCAN_CloseDevice(dev);
//        return -1;
//    }
//    // 注意：接实车监听时，不需要再设置 "0/set_device_tx_echo"，我们只听实车发出来的！
//    ReleaseIProperty(property);
//
//    ZCAN_CHANNEL_INIT_CONFIG config;
//    memset(&config, 0, sizeof(config));
//    config.can_type = TYPE_CAN; // 标准CAN
//    config.can.mode = 0;        // 0: 正常监听模式
//
//    chn = ZCAN_InitCAN(dev, 0, &config);
//    if (0 == ZCAN_StartCAN(chn)) {
//        printf("启动 CAN0 通道失败！\n");
//        ZCAN_CloseDevice(dev);
//        return -1;
//    }
//
//    ZCAN_ClearBuffer(chn);
//
//    // 启动独立接收线程
//    thd_handle = std::thread(thread_receive_task, chn, dbc_handle);
//    printf(">> USBCAN-4E-U CAN0 监听已就绪！正在持续捕获实车高频报文...\n");
//    return 0;
//}

//// =========================================================================
//// 4. 主程序：指挥官只需保持在线
//// =========================================================================
//int main() {
//    std::thread thd_handle;
//    std::string dbc_path = "C:\\Users\\HP\\Desktop\\C_USBCAN_E_U_251203\\WLHG_Power_v4_R_1_0.dbc";
//
//    // 1. 初始化 DBC 引擎：第2个参数设为 1，激活内部隐藏的异步解析线程！
//    DBCHandle dbc_handle = ZDBC_Init(0, 1);
//    if (INVALID_DBC_HANDLE == dbc_handle) return -1;
//
//    FileInfo file_info;
//    memset(&file_info, 0, sizeof(file_info));
//    strcpy_s(file_info.strFilePath, _MAX_FILE_PATH_, dbc_path.c_str());
//    if (!ZDBC_LoadFile(dbc_handle, &file_info)) {
//        printf("加载 DBC 文件失败！\n");
//        ZDBC_Release(dbc_handle);
//        return -1;
//    }
//
//    // 2. 注册“异步回调专员”，将我们的防卡死解析函数绑定到引擎上
//    ZDBC_SetAsyncAnalyseCallback(dbc_handle, MyOnAsyncAnalyseDone, NULL);
//
//    // 3. 打开硬件并启动后台接收线程
//    DEVICE_HANDLE dev = nullptr;
//    CHANNEL_HANDLE chn = nullptr;
//    if (StartDevice(dev, chn, thd_handle, dbc_handle) == -1) {
//        ZDBC_Release(dbc_handle);
//        return -1;
//    }

//    printf("\n=== 正在实车监听中，按任意键安全退出程序 ===\n");
//    getchar(); // 主线程完全阻塞在这里，不消耗 CPU，后台所有解包工作都在异步接力进行！
//
//    // 4. 安全停车与销毁
//    g_thd_run = 0;
//    if (thd_handle.joinable()) thd_handle.join();
//    ZCAN_CloseDevice(dev);
//    ZDBC_Release(dbc_handle);
//    return 0;
//}
//--------------------------------------------------------------


//这一版是同步模式的
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "zdbc.h"       // ZLG DBC 库头文件
#include "zlgcan.h"     // CAN 卡硬件底层驱动

// 【白名单筛选器】判断当前收到的 ID 是不是我们想解析的目标
bool IsTargetId(uint32 can_id) {
    // 这里可以任意增减你感兴趣的 CAN ID
    if (can_id == 0x302 || can_id == 0x303 || can_id == 0x304) {
        return true;
    }
    return false;
}

int main() {
    // ==========================================
    // STEP 1: 初始化 DBC 解析库 (同步手动模式)
    // ==========================================
    DBCHandle hDbc = ZDBC_Init(0, 0); // (0,0) 代表不启动后台异步线程，自己掌控全局
    if (INVALID_DBC_HANDLE == hDbc) {
        printf("DBC 解析库初始化失败！\n");
        return -1;
    }

    FileInfo fi;
    memset(&fi, 0, sizeof(fi));
    strcpy_s(fi.strFilePath, sizeof(fi.strFilePath), "C:\\Users\\HP\\Desktop\\C_USBCAN_E_U_251203\\WLHG_Power_v4_R_1_0.dbc");
    if (!ZDBC_LoadFile(hDbc, &fi)) {
        printf("加载 DBC 文件失败，请检查路径！\n");
        ZDBC_Release(hDbc);
        return -1;
    }
    printf(">> DBC 字典加载成功！\n");

    // ==========================================
    // STEP 2: 初始化并启动硬件 CAN 卡 (以 USBCAN-4E-U 为例)
    // ==========================================
    DEVICE_HANDLE dev = ZCAN_OpenDevice(ZCAN_USBCAN_4E_U, 0, 0);
    if (INVALID_DEVICE_HANDLE == dev) {
        printf("打开 CAN 设备失败！请确认 USB 没掉线且没被 ZCANPRO 占用。\n");
        ZDBC_Release(hDbc);
        return -1;
    }

    // 设置标准 CAN 波特率为 500Kbps
    IProperty* property = GetIProperty(dev);
    property->SetValue("0/baud_rate", "500000");
    ReleaseIProperty(property);

    ZCAN_CHANNEL_INIT_CONFIG cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.can_type = TYPE_CAN; // 标准 CAN
    cfg.can.mode = 0;        // 正常工作模式

    CHANNEL_HANDLE chn = ZCAN_InitCAN(dev, 0, &cfg);
    ZCAN_StartCAN(chn);
    printf(">> CAN0 通道启动成功！正在实时抓取总线数据...\n\n");

    // ==========================================
    // STEP 3: 实时接收与解析的主循环
    // ==========================================
    ZCAN_Receive_Data canData[100]; // 每次最多从驱动缓存拉取 100 帧
    DBCMessage decodedMsg;          // 用于存放被自动解码后的消息结构体

    printf("=== 进入实时解析循环 ===\n");

    while (true) {
        // 1. 从硬件接收队列读取实时数据 (超时时间设为 20ms)
        uint32 len = ZCAN_Receive(chn, canData, 100, 20);

//// =========================================================================
//// 2. 【软件测试】：在内存里用2 帧假的 CAN 报文，直接喂给后面的解析引擎
//        uint32 len = 2; // 声明本次收到了 2 帧报文
//        ZCAN_Receive_Data canData[2];
//        memset(canData, 0, sizeof(canData));
//
//        // --- 第一帧：捏造 ID: 0x304 (车速报文) -> 目标让程序算出来是 60.0 km/h ---
//        canData[0].frame.can_id = 0x304;
//        canData[0].frame.can_dlc = 8;
//        // 假设车速在第0、第1字节
//        // 物理值 60.0 对应的原始数字 (Raw) 就是 1400，十六进制为 0x0578----目标的raw值=（期望的物理值-Offset）/Factor
//        // 按 Intel (小端) 格式摆放：低位在前，高位在后
//        canData[0].frame.data[0] = 0x78; // Byte 0: 低字节
//        canData[0].frame.data[1] = 0x05; // Byte 1: 高字节
//        canData[0].timestamp = GetTickCount64();
//
//        // --- 第二帧：捏造 ID: 0x302 (电池报文) -> 目标让程序算出来是 380.0 V ---
//        canData[1].frame.can_id = 0x302;
//        canData[1].frame.can_dlc = 8;
//        // 假设电压在第0、第1字节，解析公式为: $Actual=Raw\times0.1+0$
//        // 物理值 380.0 对应的原始数字 (Raw) 就是 3800，十六进制为 0x0ED8
//        canData[1].frame.data[0] = 0xD8; // Byte 0: 低字节
//        canData[1].frame.data[1] = 0x0E; // Byte 1: 高字节
//        canData[1].timestamp = GetTickCount64();
//// =========================================================================

        for (uint32 i = 0; i < len; i++) {
            uint32 real_id = GET_ID(canData[i].frame.can_id);

            // 2. 【核心过滤】如果不是我们指定的 ID，直接忽略！
            if (!IsTargetId(real_id)) {
                continue;
            }

            // 3. 【一键解码】把底层的二进制报文 can_frame，直接解码进 DBCMessage 结构体！
            // 这一步之后，decodedMsg 里的各个 vSignals 的 nRawValue 就全是当前真实的实时数据了
            if (ZDBC_Decode(hDbc, &decodedMsg, &canData[i].frame, 1, FT_CAN)) {

                printf("\n>>> 捕获报文 [ID: 0x%03X] %s | 时间戳: %llu <<<\n",
                    decodedMsg.nID, decodedMsg.strName, canData[i].timestamp);

                // 4. 遍历报文里的所有信号，计算出当前真实的物理值
                for (int s = 0; s < (int)decodedMsg.nSignalCount; s++) {
                    DBCSignal* pSig = &decodedMsg.vSignals[s];

                    // 【核心公式换算】将实时提取出的 nRawValue 转换成物理浮点数
                    double actual_val = ZDBC_CalcActualValue(pSig, &pSig->nRawValue);

                    // 打印实时结果
                    printf("  %-25s : %-10.2f %s (底层Raw: %llu)\n",
                        pSig->strName, actual_val, pSig->unit, pSig->nRawValue);
                }
                printf("----------------------------------------------------\n");
            }
        }

        // 稍微休眠 1 毫秒，避免 Windows 主程序把 CPU 占满
        Sleep(1);
    }

    // ==========================================
    // STEP 4: 资源释放 (程序正常终止时执行)
    // ==========================================
    ZCAN_CloseDevice(dev);
    ZDBC_Release(hDbc);
    return 0;
}