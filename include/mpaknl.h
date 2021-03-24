/** @file mpaknl.h
 *  @brief Message Process Architecture (MPA) core operation prototypes and
 * types.
 *
 *  This file contains type definitions and core operation prototypes of
 *  Message Process Architecture (MPA).These core operations maintain MPA
 *  memory segment which stores all MPA configurations. This memory segment
 *  is also shared among all processes of Payment platform.
 *
 *  These core operations provide:
 *  - Create and initialize memory segment, and make it shared among processes;
 *  - Load setting from configuration file to memory segemnt;
 *  - Save setting from memory to configuration file;
 *  - Add/Delete/Modify server (process) settings;
 *  - Add/Delete/Modify type (message) settings;
 *  - Miscellaneous operations.
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
 */
#ifndef __MPA_KERNEL__
#define __MPA_KERNEL__

#ifdef __cplusplus
extern "C" {
#endif

#include "mpatype.h"
#include "rscommon/commonbase.h"
#include "rscommon/msq.h"

// Constant declarations {{{
#define MPA_PF_MAIN_SEC "main"

#define MPA_PF_MAXSVRINFONUM "max_serverinfo_nums"
#define MPA_PF_MAXMSGTYPEINFONUM "max_typeinfo_nums"
#define MPA_PF_VERSION "version"

#define MPA_PF_SERVER_SEC "server"
#define MPA_PF_SVRNUM "server_nums"

#define MPA_PF_MSGTYPE_SEC "msgtype"
#define MPA_PF_TYPE_NUM "type_nums"
// Constant declarations }}}

// Type definitions {{{
typedef unsigned short int mpa_index_t;

typedef struct MPA_SIS_SrvInfo {
  DWORD dwSid;
  key_t dwQkey;
  int dwQid;
  DWORD dwQtype;
} MPA_SIS_SrvInfo;

typedef struct MPA_SIS_TypeInfo {
  DWORD dwType;
  WORD wSidIndex;
} MPA_SIS_TypeInfo;

typedef struct MPA_SISInfo {
  DWORD dwTotalSize;             /**< Total size in bytes of MPA information segment */
  WORD wMaxSvrInfo;              /**< Max server (process) numbers */
  WORD wMaxTypeInfo;             /**< Max type numbers */
  WORD wSAddrOffset;             /**< Server info section head offset */
  WORD wTAddrOffset;             /**< Type info section head offset */
  WORD *pwSrvInfoSize;           /**< Pointer to the memory which stores size of server
                                    infos */
  MPA_SIS_SrvInfo *pServerInfos; /**< Pointer to the head of server info list */
  WORD wTListHeadOffset;         /**< Offset of type info list in type info section */
  WORD *pwTListSize;             /**< Pointer to the memory which stores size of type list
                                    size */
  MPA_SIS_TypeInfo *pTypeInfos;  /**< Pointer to the head of type info list */
} MPA_SISInfo;
// Type definitions }}}

// Functions {{{
/** @brief Create MPA memory-map file and initialize memory.
 *
 *  This function creates MPA memory-map file with initial contents and maps it
 * to the memory. Then it setups the initial informations of MPA in this memeory
 * segment.
 *
 *  MPA information memory segment is used to store configurations of MPA in
 * memory, it can be accessed by GetSISInfo(), it will fill a MPA_SISInfo struct
 * object with all the informations you need.
 *
 *  The MPA information structure in memory:
 *  +-----+----+----+----+----+----+---------------+----+----+-------------+
 *  |DWORD|WORD|WORD|WORD|WORD|WORD|Server Infos...|WORD|WORD|Type Infos...|
 *  | (1) |(2) |(3) |(4) |(5) |(6) |     (7)       |(8) |(9) |    (10)     |
 *  +-----+----+----+----+----+----+---------------+----+----+-------------+
 *
 *  (1). Total size of the whole memory segment, stored in dwTotalSize in
 * MPA_SISInfo when returned by GetSISInfo;
 *
 *  (2). Max server info size, stored in wMaxSvrInfo;
 *
 *  (3). Max type info size, stored in wMaxTypeInfo;
 *
 *  (4). Offset of the server info list, stored in wSAddrOffset;
 *
 *  (5). Offset of the type info section head, stored in wTAddrOffset;
 *
 *  (6). Server list size, pointed by the pointer pwSrvInfoSize;
 *
 *  (7). Server info list which contains a list of all the server settings,
 *       the starting address is pointed by the pointer pServerInfos;
 *
 *  (8). Type list head offset, stored in wTListHeadOffset, which helps to
 *       calculate the starting address of Type info list(10);
 *
 *  (9). Type list size, pointed by the pointer pwTListSize;
 *
 *  (10). Type info list which contains a list of all the type settings,
 *        the starting address is pointed by the pointer pTypeInfos.
 *
 *  @see struct MPA_SISInfo
 *  @see GetSISInfo()
 *
 *  @param[in] pszFileName MPA memory map file name
 *  @param[in] nNumOfProcess Max number of processes
 *  @param[in] nNumOfType Max number of types
 *  @return 0 Sucesss
 *  @return <0 Failed
 */
DLL_PUBLIC int MPA_SIS_Create(const char *pszFileName, size_t nNumOfProcess, size_t nNumOfType);

/** @brief Map memory-map file to memory.
 *
 *  This function maps MPA memory-map file to memory and returns the
 *  head address.
 *
 *  @see manpage mmap(2)
 *
 *  @param[in] pszFileName MPA memory-map file name
 *  @return >0 Sucess; pointer to the head of memeory segment
 *  @return 0 Failed
 */
DLL_PUBLIC char *MPA_SIS_Init(const char *pszFileName);

/** @brief Add server info.
 *
 *  This function adds a server info into MPA configuration memory segment.
 *
 *  @param[in] pMPAStart Beginning address of MPA configuration memory segment
 *  @param[in] sid Server id
 *  @param[in] qkey Message queue key
 *  @param[in] qtype Message type
 *  @return 0 Success
 *  @return -1 Failed
 */
DLL_PUBLIC int MPA_SIS_SInfoAdd(const char *pMPAStart, DWORD sid, key_t qkey, DWORD qtype);

/** @brief Modify server info.
 *
 *  This function updates a server info in MPA configuration memory segment
 *  with new qkey and qtype.
 *
 *  @param[in] pMPAStart Beginning address of MPA configuration memory segment
 *  @param[in] sid Server id
 *  @param[in] qkey New message queue key
 *  @param[in] qtype New message type
 *  @return 0 Success
 *  @return -1 Failed
 */
DLL_PUBLIC int MPA_SIS_SInfoModify(const char *pMPAStart, DWORD sid, key_t qkey, DWORD qtype);
DLL_PUBLIC int MPA_SIS_SInfoDelLast(const char *pMPAStart);
DLL_PUBLIC int MPA_SIS_TInfoAdd(const char *pMPAStart, DWORD type, DWORD sid);
DLL_PUBLIC int MPA_SIS_TInfoModify(const char *pMPAStart, DWORD type, DWORD sid, DWORD new_type,
                                   DWORD new_sid);
DLL_PUBLIC int MPA_SIS_TInfoDelLast(const char *pMPAStart);
DLL_PUBLIC int MPA_SIS_End(const char *pMPAStart, Boolean bRelease);
DLL_PUBLIC void MPA_SIS_Display(const char *pMPAStart);
DLL_PUBLIC int MPA_SIS_LoadConfig(const char *pszSHMFileName, const char *pszFileName);
DLL_PUBLIC int MPA_SIS_ExportConfig(const char *pMPAStart, const char *pszFileName);
DLL_PUBLIC void GetSISInfo(const char *pMPAStart, MPA_SISInfo *pSISInfo);
DLL_PUBLIC int MPA_GetServerInfoByIndex(mpa_index_t index, MPA_SIS_SrvInfo *pSrvInfo,
                                        const char *pMPAStart);
DLL_PUBLIC int MPA_GetServerInfo(DWORD sid, MPA_SIS_SrvInfo *pSrvInfo, const char *pMPAStart);
DLL_PUBLIC int MPA_GetTypeInfo(mpa_index_t index_, DWORD type, MPA_SIS_TypeInfo *pTypeInfo,
                               const char *pMPAStart);
DLL_PUBLIC size_t MPA_CheckQKey(key_t qkey, const char *pMPAStart);
DLL_PUBLIC int MPA_CheckMsgQ(key_t qkey);
// Functions }}}

#ifdef __cplusplus
}
#endif

#endif
/* vim: set ts=2 sw=2 sts=2 tw=0 expandtab : */
