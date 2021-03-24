/** @file mpacli.h
 *  @brief Message Process Architecture (MPA) API prototypes and types.
 *
 *  This file contains API definitions of Message Process
 *  Architecture (MPA). These APIs provide message handling operations,
 *  such as: receive, send, publish, etc. All API operations are based
 *  on mpaknl core operations to work.
 *
 *  @author Shengxiao Lu
 *  @author Siwen Yu (yusiwen@gmail.com)
 *
 *  @date 2003-5-28
 *  - First version by Shengxiao Lu
 *
 *  @date 2006-4-30
 *  - Maintained by Siwen Yu
 *
 *  @date 2015-12-21
 *  - Add more Doxygen style comments
 *
 *  @date 2017-2-20
 *  - Format variable names
 *  - Make some pointer point to const vars in Getters
 */
#ifndef __MPA_CLIENT__
#define __MPA_CLIENT__

#ifdef __cplusplus
extern "C" {
#endif

#include "rscommon/commonbase.h"
#include "rscommon/msq.h"

/*************************宏定义***************************************/

typedef enum MPA_SM { MPA_SM_P2P = 0, MPA_SM_PUB } MPA_SM;

/************************结构定义**************************************/
#ifndef HT_MPA_MPAMESSAGE_
#define HT_MPA_MPAMESSAGE_

#define MPA_MESSAGESIZE C_MsgbufM

#define INIT_MPAMSG_DEFAULT()                                                                      \
  { "" }

typedef struct MPAMessage {
  char buf[MPA_MESSAGESIZE];
} MPAMessage;

#endif

/************************错误定义**************************************/
#define MPA_ERR_BASE -1000

#define MPA_ERR_INIT -200
#define MPA_ERR_PARAM -201
#define MPA_ERR_OUT_OF_RANGE -202

#define MPA_ERR_SVRINFO MPA_ERR_BASE * 1
#define MPA_ERR_TYPEINFO MPA_ERR_BASE * 2
#define MPA_ERR_RECV MPA_ERR_BASE * 3
#define MPA_ERR_RECV_2BIG (MPA_ERR_BASE * 3 + 1)  // 需接收的消息大于给定缓冲区大小
#define MPA_ERR_RECV_NOQ (MPA_ERR_BASE * 3 + 2)   // 消息队列不存在
#define MPA_ERR_RECV_NOMSG (MPA_ERR_BASE * 3 + 3) // 消息队列无消息（IPC_NOWAIT时）
#define MPA_ERR_SEND MPA_ERR_BASE * 4
#define MPA_ERR_SEND_NOMEM (MPA_ERR_BASE * 4 + 1) // 发送的消息大于系统缓冲区大小
#define MPA_ERR_SEND_NOQ (MPA_ERR_BASE * 4 + 2)   // 消息队列不存在
#define MPA_ERR_INTR MPA_ERR_BASE * 5

#define MPA_ERR_NOINIT -205
#define MPA_ERR_END -206

/****************************函数原型*********************************/
/*********************************************************************
 *                            环境初始化                              *
 **********************************************************************/
/*=====================================================================
* func name: MPA_Init
* func desc: 初始化MPA运行环境
* param :    pszSHMFileName [in] MPA使用的共享内存路径
*            sid            [in] 系统唯一标识(>0)
* return:    0     成功
*            !=0   失败
=====================================================================*/
DLL_PUBLIC int MPA_Init(const char *pszSHMFileName, DWORD sid);

DLL_PUBLIC int MPA_End(Boolean bRelease);

/*=====================================================================
* func name: MPA_GetSID
* func desc: 获得当前系统唯一标识符
* return:    >0    当前系统标识符
*            <0    失败
=====================================================================*/
DLL_PUBLIC DWORD MPA_GetSID(void);

/*********************************************************************
 *                            消息拆包、封包                          *
 **********************************************************************/
/*=====================================================================
* func name: MPA_MsgInit
* func desc: 初始化消息包
* param :    pMessage  [in] 当前消息
* return:    NULL
=====================================================================*/
DLL_PUBLIC void MPA_MsgInit(MPAMessage *pMessage);

DLL_PUBLIC ssize_t MPA_GetMsgLength(const MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_GetMsgID
* func desc: 得到当前消息的唯一标识号
* param :    pMessage  [in] 当前消息
* return:    >=0       消息标识号
* note: 保留
=====================================================================*/
DLL_PUBLIC WORD MPA_GetMsgID(const MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_SetMsgID
* func desc: 设置当前消息唯一标识号
* param :    wMsgID      [in]    消息唯一标识号
*            pMessage    [in]    当前消息
* return:
* note: 保留
=====================================================================*/
DLL_PUBLIC void MPA_SetMsgID(WORD wMsgID, MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_GetMsgType
* func desc: 得到消息类型(即发布时的类型)
* param :    pMessage [in]  消息
* param :    pMsgType [out]  消息类型
* return:    =0            成功
*            MPA_ERR_PARAM  参数错误
=====================================================================*/
DLL_PUBLIC int MPA_GetMsgType(const MPAMessage *pMessage, DWORD *pMsgType);

/*=====================================================================
* func name: MPA_GetMsgMode
* func desc: 获取消息的发送模式
* param :    pMessage [in]  当前消息
* param :    pMsgMode [out]  消息发送模式
*            MPA_SM_P2P     为p2p模式发送
*            MPA_SM_PUB     为pub/sub模式发送
* return:    =0            成功
*            MPA_ERR_PARAM  参数错误
=====================================================================*/
DLL_PUBLIC int MPA_GetMsgMode(const MPAMessage *pMessage, BYTE *pMsgMode);

/*=====================================================================
* func name: MPA_GetMsgSource
* func desc: 获取消息发送者的唯一标识
* param :    pMessage    [in]    当前消息
* param :    pMsgSource  [out]   消息发送者
* return:    =0            成功
*            MPA_ERR_PARAM  参数错误
=====================================================================*/
DLL_PUBLIC int MPA_GetMsgSource(const MPAMessage *pMessage, DWORD *pMsgSource);

/*=====================================================================
* func name: MPA_GetMsgDest
* func desc: 获取消息的目的地标识
* param :    pMessage    [in]    当前消息
* param :    pMsgDest    [out]   消息目的
* return:    =0            成功
*            MPA_ERR_PARAM  参数错误
=====================================================================*/
DLL_PUBLIC int MPA_GetMsgDest(const MPAMessage *pMessage, DWORD *pMsgDest);

/*=====================================================================
* func name: MPA_GetMsgReplyTo
* func desc: 获取消息的应答目的地标识
* param :    pMessage    [in]    当前消息
* param :    pMsgReplyTo [out]   消息应答目的
* return:    =0            成功
*            MPA_ERR_PARAM  参数错误
=====================================================================*/
DLL_PUBLIC int MPA_GetMsgReplyTo(const MPAMessage *pMessage, DWORD *pMsgReplyTo);

/*=====================================================================
* func name: MPA_SetMsgReplyTo
* func desc: 设置当前消息对应的应答消息的目的系统唯一标识号
* param :    sid        [in]    应答消息的目的地唯一标识号
*            pMessage   [in]    当前消息
* return:
=====================================================================*/
DLL_PUBLIC void MPA_SetMsgReplyTo(DWORD sid, MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_GetMsgTimeStamp
* func desc: 获取当前消息产生的时间
* param :    pMessage    [in]    当前消息
* return:    系统绝对时间(毫秒)
* note: 保留
=====================================================================*/
DLL_PUBLIC DWORD MPA_GetMsgTimeStamp(const MPAMessage *pMessage);
DLL_PUBLIC void MPA_SetMsgTimeStamp(WORD wTimeStamp, const MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_GetMsgExpiration
* func desc: 获取当前消息的过期时间
* param :    pMessage    [in]    当前消息
* return:    系统绝对时间(毫秒)
* note: 保留
=====================================================================*/
DLL_PUBLIC WORD MPA_GetMsgExpiration(const MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_SetMsgExpiration
* func desc: 设置当前消息的过期时间
* param :   dwExpTime  [in]    系统过期时间(毫秒)
*           pMessage   [in]    当前消息
* return:    = 0    成功
*            !=0    失败
* note: 保留
=====================================================================*/
DLL_PUBLIC void MPA_SetMsgExpiration(WORD wExpTime, MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_GetMsgProp
* func desc: 获取消息的属性值
* param :   pszName    [in]   属性名
*           pszValue   [out]  属性值
*           size       [in]   pszValue变量长度
*           pMessage   [in]   当前消息
* return:    实际大小
* note: 保留
=====================================================================*/
DLL_PUBLIC ssize_t MPA_GetMsgProp(const char *pszName, char *pszValue, size_t size,
                                  const MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_SetMsgProp
* func desc: 设置消息的属性值
* param :   pszName    [in]    属性名
*           pszValue   [in]    属性值
*           pMessage   [in]    当前消息
* note: 保留
=====================================================================*/
DLL_PUBLIC int MPA_SetMsgProp(const char *pszName, const char *pszValue, MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_GetMsgBody
* func desc: 获得消息的正文
* param :   body       [out]    正文
*           size       [out]    正文长度
*           pMessage   [in]    当前消息
* return : 指向包体的指针。当body为NULL时，可以使用返回的指针读取包内容。
*           当size,pMessage为NULL时，返回NULL。
=====================================================================*/
DLL_PUBLIC char *MPA_GetMsgBody(char *body_, size_t *size, const MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_Sub
* func desc: 设置消息的正文
* param :   body       [in]    正文
*           size       [in]    正文长度
*           pMessage   [in]    当前消息
* note: 保留
=====================================================================*/
DLL_PUBLIC int MPA_SetMsgBody(const char *body_, size_t size, MPAMessage *pMessage);

/*********************************************************************
 *                            消息操作                                *
 **********************************************************************/
/*=====================================================================
* func name: MPA_Sub
* func desc: 消息订阅 订阅某个类别的消息
* param :    type  [in] 欲订阅的消息类别
* return:    = 0    成功
*            !=0    失败
* note: 保留
=====================================================================*/
DLL_PUBLIC int MPA_Sub(DWORD type);

/*=====================================================================
* func name: MPA_Send
* func desc: 消息发送，发送至某个系统
* param :    sid      [in] 目的系统标识符
*            pMessage [in] 欲发送的消息
* return:    = 0    成功
*            !=0    失败
=====================================================================*/
DLL_PUBLIC int MPA_Send(DWORD sid, const MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_SendSelf
* func desc: 消息发送，发送至本进程
* param :    mtype    [in] 欲发送的消息类型
*            pMessage [in] 欲发送的消息
* return:    = 0    成功
*            !=0    失败
=====================================================================*/
DLL_PUBLIC int MPA_SendSelf(DWORD mtype, const MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_SendSeldEx
* func desc: 消息发送，发送至本进程
* param :    pMessage  [in] 欲发送的消息
* return:    = 0    成功
*            !=0    失败
=====================================================================*/
DLL_PUBLIC int MPA_SendSelfEx(const MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_Pub
* func desc: 消息发布，发布至所有订阅此消息类型的进程
* param :    type     [in] 欲发布的消息类型
*            pMessage [in] 欲发送的消息
* return:    = 0    成功
*            !=0    失败
=====================================================================*/
DLL_PUBLIC int MPA_Pub(DWORD type, const MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_Recv
* func desc: 消息接收
* param :    pMessage  [out] 接收到的消息
* return:    >=0    接收到消息的长度
*            <0    失败
=====================================================================*/
DLL_PUBLIC ssize_t MPA_Recv(MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_RecvTypeNonBlock
* func desc: 消息接收(非阻塞)
* param :    mtype     [in] 需要接收的消息类型
*            pMessage  [out] 接收到的消息
* return:    >=0    接收到消息的长度
*            <0    失败
=====================================================================*/
DLL_PUBLIC ssize_t MPA_RecvTypeNonBlock(DWORD mtype, MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_RecvNonBlock
* func desc: 消息接收(非阻塞)
* param :    pMessage  [out] 接收到的消息
* return:    >=0    接收到消息的长度
*            <0    失败
=====================================================================*/
DLL_PUBLIC ssize_t MPA_RecvNonBlock(MPAMessage *pMessage);

/*=====================================================================
* func name: MPA_Validate
* func desc: 检查本进程(g_sid)绑定的消息队列是否存在
* return:    0     存在
*            -1    不存在
*            -2    判断失败
*            -1000 未能找到本进程的ServerInfo信息
=====================================================================*/
DLL_PUBLIC int MPA_Validate(void);

DLL_PUBLIC void DumpMPAMessage(const MPAMessage *pMessage);

DLL_PUBLIC void mpa_getVersion(int *major, int *minor, int *patch, char *meta);
DLL_PUBLIC char *mpa_getBuildInfo(char *buffer, size_t len);

#ifdef __cplusplus
}
#endif

#endif
/* vim: set ts=2 sw=2 sts=2 tw=0 expandtab : */
