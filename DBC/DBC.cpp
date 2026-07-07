/*
 更新时间：251104

 - zdbc函数库对普通dbc解析功能是正常的。
 - 如果只需要解析DBC消息，可用 Decode 和 Encode 的方法。

 - J1939 组播和广播包的处理，函数库处理有问题，如涉及到此用法，不建议使用该zdbc函数库。
 - DBC函数库没有获取 DBCMessage 是 CAN 还是 CANFD。
 - CANFD的消息，不能用CAN的方式来发送。
 
 - 新增异步解析 通过设置信号回调，解析多帧表示的信号
*/


#include "stdio.h"
#include "stdafx.h"
#include <iostream>
#include <string.h>
#include <thread>
#include <ctime>
#include <windows.h>
#include "zdbc.h"
#include "zlgcan.h"
#define FUNCCALL __stdcall

using namespace std;

struct Ctx{
	CHANNEL_HANDLE chn;
};

Ctx m_ctx;
int g_thd_run = 1;

// 加载DBC文件
int LoadDBCFile(DBCHandle & dbc_handle, string DBC_file_path) {
	FileInfo file_info;
	memset(&file_info, 0, sizeof(file_info));
	file_info.merge = 0;
	strcpy_s(file_info.strFilePath, _MAX_FILE_PATH_, DBC_file_path.c_str());

	bool ret = ZDBC_LoadFile(dbc_handle, &file_info);
	if (ret == false) {
		printf("加载DBC文件失败\n");
		return -1;
	}
	else
		printf("加载DBC文件成功\n");
	return 0;
}


// 打印DBC消息
void PrintfDBCMessage(DBCMessage msg)
{
	printf("ID: 0x%x\t 消息名: %s\t %s\n", msg.nID, msg.strName, msg.nExtend == 1 ? "扩展帧" : "标准帧");

	printf("\t    信号名                              起始位  位长度  原始值\n");
	for (int i = 0; i < (int)msg.nSignalCount; ++i) {
		printf("\t%2d  %-30s\t%-5d\t%-5d\t%-5ld\n", i + 1,
			msg.vSignals[i].strName,
			msg.vSignals[i].nStartBit,
			msg.vSignals[i].nLen,
			msg.vSignals[i].nRawValue
			);
	}
	printf("\n");
}


// 读取DBC文件中的消息
int ReadDBCFileMessage(DBCHandle dbc_handle) {
	// 获取 DBC 文件中含有的消息数目
	int msgCount = ZDBC_GetMessageCount(dbc_handle);
	printf("该DBC文件中总共有 %d 条消息\n\n", msgCount);

	// 打印每一条消息的定义
#if 1
	DBCMessage msg;
	if (ZDBC_GetFirstMessage(dbc_handle, &msg)) {
		PrintfDBCMessage(msg);
		while (ZDBC_GetNextMessage(dbc_handle, &msg)) {
			PrintfDBCMessage(msg);
		}
	}
#endif
	return 0;
}


// 获取单个ID的DBC定义
int GetDBCMessageByID(DBCHandle dbc_handle, int ID)
{
	// 获取单个ID的信息
	DBCMessage msg;
	if (ZDBC_GetMessageById(dbc_handle, ID, &msg) == true)		// 获取对应ID的DBCMessage
	{
		PrintfDBCMessage(msg);
		return 1;
	}
	return 0;
}


// 获取具体信号的值与含义对个数
void GetSignalValPair(DBCHandle dbc_handle, int ID, const char* signalName)
{
	DBCMessage msg;
	if (ZDBC_GetMessageById(dbc_handle, ID, &msg) == true) {		// 先判断有没有这条报文
		int signalCount = ZDBC_GetValDescPairCount(dbc_handle, 0x2A1, signalName);
		printf("ID = %x 有 %d 个信号的值与含义对\n", signalCount);

		for (int i = 0; i < signalCount; i++){

		}
	}
}


// 发送DBC消息 发送回调函数
static int SendDBCMessage1(DBCHandle dbc_handle) {
	uint32 ID = 0x15D;

	DBCMessage msg;
	if (ZDBC_GetMessageById(dbc_handle, ID, &msg) == false)		// 获取对应ID的DBCMessage
	{
		printf("没有对应id的消息\n");
		return 0;
	}
	//else{
	//	PrintfDBCMessage(msg);
	//}

	// 通过 ZDBC_CalcRawValue 设置信号值（这里是根据实际的DBC信号设置的值，作为示例）
	double actualVal = 2000;				// 物理值
	uint64 rawValue = ZDBC_CalcRawValue(&msg.vSignals[6], &actualVal);	// 计算实际值, 超出可信号值范围时会被修正
	msg.vSignals[6].nRawValue = rawValue;

	if (ZDBC_Send(dbc_handle, &msg, FT_CAN) == ERR_SUCCESS){
		printf("发送0x%X DBC消息\n", ID);
	}
	else{
		printf("发送失败\n");
	}

	if (ZDBC_Send(dbc_handle, &msg, FT_CANFD) == ERR_SUCCESS){
		printf("发送0x%X DBC消息\n", ID);
	}
	else{
		printf("发送失败\n");
	}
}


// 发送DBC消息 Encode方式
static int SendDBCMessage2(DBCHandle dbc_handle, CHANNEL_HANDLE ch) {
	uint32 ID = 0x15D;

	DBCMessage msg;
	if (ZDBC_GetMessageById(dbc_handle, ID, &msg) == false)		// 获取对应ID的DBCMessage
	{
		printf("没有对应id的消息\n");
		return 0;
	}

	// 通过 ZDBC_CalcRawValue 设置信号值（这里是根据实际的DBC信号设置的值，作为示例）
	double actualVal = 2025;				// 物理值
	uint64 rawValue = ZDBC_CalcRawValue(&msg.vSignals[6], &actualVal);	// 计算实际值, 超出可信号值范围时会被修正
	msg.vSignals[6].nRawValue = rawValue;

	uint32 val = 1;

	// CAN
	can_frame can_f;
	memset(&can_f, 0, sizeof(can_f));
	bool ret = ZDBC_Encode(dbc_handle, &can_f, &val, &msg, FT_CAN);
	if (ret){
		ZCAN_Transmit_Data can_data;
		memset(&can_data, 0, sizeof(can_data));
		can_data.frame = can_f;
		ZCAN_Transmit(ch, &can_data, 1);
	}
	else{
		printf("转换失败\n");
	}

	// CANFD
	canfd_frame canfd_f;
	memset(&canfd_f, 0, sizeof(canfd_f));
	ret = ZDBC_Encode(dbc_handle, &canfd_f, &val, &msg, FT_CANFD);
	if (ret){
		ZCAN_TransmitFD_Data canfd_data;
		memset(&canfd_data, 0, sizeof(canfd_data));
		canfd_data.frame = canfd_f;
		ZCAN_TransmitFD(ch, &canfd_data, 1);
	}
	else{
		printf("转换失败\n");
	}
	return 0;
}


// 普通接收线程
void thread_task(CHANNEL_HANDLE chn, DBCHandle dbc_handle)
{
	ZCAN_Receive_Data canData[100] = {};
	ZCAN_ReceiveFD_Data canfdData[100] = {};
	int chn_idx = (unsigned int)chn & 0x000000FF;	// 通道号

	while (g_thd_run)
	{
		if (ZCAN_GetReceiveNum(chn, 0))		// 0-CAN
		{
			uint32_t ReceiveNum = ZCAN_Receive(chn, canData, 100, 10);
			for (int i = 0; i < ReceiveNum; i++) {
				ZDBC_AsyncAnalyse(dbc_handle, &canData[i].frame, FT_CAN, canData[i].timestamp);	// 异步解析

				printf("CHN:%d [%lld] CAN  ", chn_idx, canData[i].timestamp);		// 通道和时间戳

				printf(IS_EFF(canData[i].frame.can_id) ? "扩展帧 " : "标准帧 ");		// 帧类型
				//printf(IS_RTR(canData[i].frame.can_id) ? " 远程帧 " : " 数据帧 ");	// 帧格式
				printf(IS_TX_ECHO(canData[i].frame.__pad) ? "Tx " : "Rx ");	// 方向

				printf("ID:0x%X Data: ", GET_ID(canData[i].frame.can_id));			// ID 
				for (int j = 0; j < canData[i].frame.can_dlc; j++){					// 数据
					printf("%02X ", canData[i].frame.data[j]);
				}
				printf("\n");
			}
		}

		if (ZCAN_GetReceiveNum(chn, 1))		// 1-CANFD
		{
			uint32_t ReceiveNum = ZCAN_ReceiveFD(chn, canfdData, 100, 10);
			for (int i = 0; i < ReceiveNum; i++)
			{
				ZDBC_AsyncAnalyse(dbc_handle, &canfdData[i].frame, FT_CANFD, canfdData[i].timestamp);	// 异步解析
				printf("CHN:%d [%lld] CANFD", chn_idx, canfdData[i].timestamp);			// 通道和时间戳
				printf((canfdData[i].frame.flags & 0x01) == 0x01 ? "加速 " : "");		// 加速

				printf(IS_EFF(canfdData[i].frame.can_id) ? " 扩展帧 " : " 标准帧 ");		// 帧类型
				//printf(IS_RTR(canfdData[i].frame.can_id) ? " 远程帧 " : " 数据帧 ");	// 帧格式
				printf(IS_TX_ECHO(canfdData[i].frame.flags) ? " Tx " : " Rx ");	// 方向

				printf("ID:0x%X Data: ", GET_ID(canfdData[i].frame.can_id));			// ID
				for (int j = 0; j < canfdData[i].frame.len; j++) {						// 数据
					printf("%02x ", canfdData[i].frame.data[j]);
				}
				printf("\n");
			}
		}
		Sleep(10);
	}
}


// CAN 发送回调函数（老方法）
static bool __stdcall DBCSendFunc(void* ctx, void* pObj) {
	Ctx* info = static_cast<Ctx*>(ctx);
	can_frame* obj = static_cast<can_frame*>(pObj);

	ZCAN_Transmit_Data data;
	data.transmit_type = 0;
	data.frame = *obj;
	if (ZCAN_Transmit(info->chn, &data, 1))
		return true;
	return false;
}


// CANFD 发送回调函数（老方法）
static bool __stdcall DBCSendFDFunc(void* ctx, void* pObj) {
	Ctx* info = static_cast<Ctx*>(ctx);
	canfd_frame* obj = static_cast<canfd_frame*>(pObj);

	ZCAN_TransmitFD_Data data;
	data.transmit_type = 0;
	data.frame = *obj;
	if (ZCAN_TransmitFD(info->chn, &data, 1))
		return true;
	return false;
}


// 启动设备和通道，并绑定发送回调
int StartDevice(DEVICE_HANDLE & dev, CHANNEL_HANDLE & chn, std::thread & thd_handle, DBCHandle dbc_handle) {
	UINT dev_type = ZCAN_USBCANFD_200U;
	dev = ZCAN_OpenDevice(dev_type, 0, 0);

	if (INVALID_DEVICE_HANDLE == dev) {
		std::cout << "打开设备失败" << std::endl;
		return -1;
	}
	// 通道初始化
	char path[64] = {};

	// 设置仲裁域波特率为 500k
	sprintf_s(path, "%d/canfd_abit_baud_rate", 0);
	if (0 == ZCAN_SetValue(dev, path, "500000")) {
		std::cout << "设置仲裁域波特率失败" << std::endl;
		return -1;
	}
	// 设置通道 0 数据域波特率为 2M
	sprintf_s(path, "%d/canfd_dbit_baud_rate", 0);
	if (0 == ZCAN_SetValue(dev, path, "2000000")) {
		std::cout << "设置数据域波特率失败" << std::endl;
		return -1;
	}

	// 初始化通道
	ZCAN_CHANNEL_INIT_CONFIG config;
	memset(&config, 0, sizeof(config));
	config.can_type = 1; // 0 - CAN，1 - CANFD
	config.can.mode = 0; // 0 - 正常模式，1 - 只听模式
	chn = ZCAN_InitCAN(dev, 0, &config);
	if (INVALID_CHANNEL_HANDLE == chn) {
		std::cout << "初始化通道失败" << std::endl;
		return -1;
	}

	// 使能通道终端电阻
	sprintf_s(path, "%d/initenal_resistance", 0);
	if (0 == ZCAN_SetValue(dev, path, "1")) {
		std::cout << "使能终端电阻失败" << std::endl;
		return -1;
	}

	// 全局发送回显
	sprintf_s(path, "%d/set_device_tx_echo", 0);
	if (ZCAN_SetValue(dev, path, "1") == STATUS_ERR) {                // 1-使能 0-禁能
		printf("设置全局发送回显\n");
		return -1;
	}

	// 启动 CAN 通道
	if (0 == ZCAN_StartCAN(chn)) {
		std::cout << "启动通道失败" << std::endl;
		return -1;
	}

	// 独立线程接收报文
	thd_handle = std::thread(thread_task, chn, dbc_handle);

	std::cout << "启动USBCANFD-200U CAN0" << std::endl;
	return 0;
}


// 回调函数
void CALLBACK  MyOnAsyncAnalyseDone(void* ctx, DBCMessage* pMsg, uint64 extraData)
{
	printf("time:%ud\n", extraData);
	PrintfDBCMessage(*pMsg);
}


int _tmain(int argc, _TCHAR* argv[])
{
	std::thread thd_handle;
	std::string DBC_file_path = "C:\\Users\\HP\\Desktop\\C_USBCAN_E_U_251203\\WLHG_Power_v4_R_1_0.dbc";	// 替换自己的文件地址

	// 获取句柄
	DBCHandle dbc_handle = ZDBC_Init(0, 1);
	if (INVALID_DBC_HANDLE == dbc_handle){
		printf("生成DBC句柄失败\n");
		goto end;
	}

	// 加载DBC文件
	if (LoadDBCFile(dbc_handle, DBC_file_path) == -1) {
		printf("加载DBC文件失败\n");
		goto end;
	}

	// 设置回调函数，需要配合 ZDBC_AsyncAnalyse
	ZDBC_SetAsyncAnalyseCallback(dbc_handle, MyOnAsyncAnalyseDone, NULL);

	// 获取DBC文件中的信息
	ReadDBCFileMessage(dbc_handle);
	
	// 获取某个ID的DBC定义
	GetDBCMessageByID(dbc_handle, 0x15D);


// 打开设备
// 用DBC函数库解析结果发送报文
#if 0
	DEVICE_HANDLE dev = nullptr;
	CHANNEL_HANDLE chn = nullptr;

	// 启动 USBCANFD-200U 作为示例
	if (StartDevice(dev, chn, thd_handle, dbc_handle) == -1) {	// 启动设备和通道
		printf("打开设备失败");
		goto end;
	}

	/*
	下面两种发送DBC的方式
	发送回调：
		通过设置回调函数，绑定通道，调用DBCSend即可发送
	Encode解码发送（推荐）：
		这个方式是新接口，不需要绑定设备直接将DBC消息解码之后，用设备发送接口发送，性能比较好，可以调用定时发送队列发送的功能。
	*/

	//// 发送回调方式 
	//m_ctx.chn = chn;
	//ZDBC_SetSender(dbc_handle, DBCSendFunc, &m_ctx);	// 设置CAN发送回调
	//ZDBC_SetCANFDSender(dbc_handle, DBCSendFDFunc, &m_ctx);	// 设置CANFD发送回调
	//SendDBCMessage1(dbc_handle);					91


	// Encode 编码方式
	SendDBCMessage2(dbc_handle, chn);

#endif

	getchar();
end:
	g_thd_run = 0;
	if (thd_handle.joinable())
		thd_handle.join();
	ZDBC_Release(dbc_handle);
	system("pause");
	return 0;
}

