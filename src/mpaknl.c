/** @file mpaknl.c
 *  @brief Message Process Architecture (MPA) core operation implementations.
 *
 *  This file contains core operation implementations of Message Process
 *  Architecture (MPA). These core operations maintain MPA memory segment
 *  which stores all MPA configurations and is shared among all processes
 *  of Payment platform.
 *
 *  These core operations provide:
 *  - Create and initialize memory segment, and make it shared among processes;
 *  - Load setting from configuraion file to memory segemnt;
 *  - Save setting from memory to configuraion file;
 *  - Add/Delete/Modify server (process) settings;
 *  - Add/Delete/Modify type (message) settings;
 *  - Miscellaneous operations.
 *
 *  @see mpaknl.h for more details.
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
 *  @date 2015-11-20
 *  - Add version 2.0 mpa.ini file loading support
 *
 *    Version 2.0 mpa.ini removes the sequence number of every server info and
 *    type info configuration items, and uses a linked list to store profile
 *    information read from file.
 *    @see strlist.h strlist.c
 *
 *  @date 2015-12-21
 *  - Add more Doxygen style comments
 *
 *  @date 2018-08-15
 *  - Optimize code
 *  - Suppress warnings
 *  - Fix type conversion problems
 */
// Includes {{{
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "mpaknl.h"
#include "rscommon/debug.h"
#include "rscommon/profile.h"
#include "rscommon/strfunc.h"
#include "rscommon/strlist.h"
// Includes }}}

// Local function declarations {{{
static int DumpSISInfoToFile(const MPA_SISInfo *pSISInfo, const char *pszFileName);
static void DisplaySISInfo(const MPA_SISInfo *pSISInfo);
static int FindServerInfo(const MPA_SISInfo *pSISInfo, DWORD sid);
static int FindTypeInfo(mpa_index_t index, const MPA_SISInfo *pSISInfo, DWORD type);
static int FindTypeInfoBySid(const MPA_SISInfo *pSISInfo, DWORD type, DWORD sid);
static int LoadFromFile(const char *pszSHMFileName, const char *pszINIFileName);
static int LoadFromList(const char *pMPAStart, const char *pszINIFileName,
                        size_t nMaxServerInfoNums, size_t nMaxTypeInfoNums);
// Local function declarations }}}

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#endif

DLL_PUBLIC int MPA_SIS_Create(const char *pszFileName, size_t nNumOfProcess,
                              size_t nNumOfType) { //{{{
  FILE *fp = NULL;
  BYTE b = 0;
  DWORD dw = 0;
  WORD w = 0;
  size_t nSizeOfArea = 0, n = 0, i = 0;
  char *pMPAStart = NULL; /**< Pointer to head address of memory
                               storing MPA informations */
  WORD *pMPAWork = NULL;
  MPA_SISInfo SISInfo;

  check((nNumOfProcess <= USHRT_MAX), "Number of process is too large");
  check((nNumOfType <= USHRT_MAX), "Number of types is too large");

  /** Create MPA information memory map file. {{{
   *  Create an empty memory map file with the size calculated.
   */
  /** 1. Calculate size of memory map file, see comment of MPA_SIS_Create()
   *     in mpaknl.h for details*/
  nSizeOfArea = sizeof(DWORD) + 7 * sizeof(WORD) +
                (nNumOfProcess * sizeof(MPA_SIS_SrvInfo) + nNumOfType * sizeof(MPA_SIS_TypeInfo));

  fp = fopen(pszFileName, "wbe"); /**< 2. Open file with binary write */
  check(fp, "Create memory map file error");

  dw = (DWORD)nSizeOfArea;
  n = fwrite((void *)&dw, sizeof(DWORD), 1, fp); /**< 3. Write its size to it */
  check(n == 1, "Write to memory map file error");

  w = 0;
  for (i = 0; i < 5; i++) { /**< 4. Make spaces for 5 WORDs */
    n = fwrite((void *)&w, sizeof(WORD), 1, fp);
    check(n == 1, "Write to memory map file error");
  }
  b = 0;
  for (i = 0; i < (nNumOfProcess * sizeof(MPA_SIS_SrvInfo) + nNumOfType * sizeof(MPA_SIS_TypeInfo));
       i++) { /**< 5. Make spaces for detail informations */
    n = fwrite((void *)&b, sizeof(BYTE), 1, fp);
    check(n == 1, "Write to memory map file error");
  }
  fclose(fp);
  fp = NULL; //}}}

  /** Map file to memory, and make it shared among processes */
  pMPAStart = MPA_SIS_Init(pszFileName);
  check(pMPAStart, "Initialize memory map error");

  /** Setup MPA informations in this memory segment currently mapped. {{{
   *
   *  @see mpaknl.h For more details of MPA memory structure
   * */
  /** 1. Jump over the size, it's been already set when map the file
   *     to memory, see line 82-91 and MPA_SIS_Init() */
  pMPAWork = (WORD *)(pMPAStart + sizeof(DWORD));
  *pMPAWork++ = (WORD)nNumOfProcess; /**< 2. Set max process numbers */
  *pMPAWork++ = (WORD)nNumOfType;    /**< 3. Set max type numbers */

  /**< Jump over wSAddrOffset, wTAddrOffset, wSrvInfoSize to the start address
   *   of server info section, and store this offset to SISInfo.wSAddrOffset */
  SISInfo.wSAddrOffset = (WORD)(3 * sizeof(WORD) + (size_t)((char *)pMPAWork - pMPAStart));
  (*pMPAWork++) = SISInfo.wSAddrOffset; /**< 4. Set server info section head
                                           offset in the MPA memory segment  */

  /**< Jump over server info section, type list head offset, type list size to
   *   the start of type info section, and store this offset to
   *   SISInfo.wTAddrOffset */
  SISInfo.wTAddrOffset = (WORD)(nNumOfProcess * sizeof(MPA_SIS_SrvInfo) + 2 * sizeof(WORD) +
                                (size_t)((char *)pMPAWork - pMPAStart));
  (*pMPAWork++) = SISInfo.wTAddrOffset; /**< 5. Set type info section head
                                           offset in the MPA memory segment */

  (*pMPAWork) = (WORD)0; /**< 6. Set server info size to zero */

  pMPAWork = (WORD *)(pMPAStart + SISInfo.wTAddrOffset);
  /**< Jump over wTListHeadOffset and wTListSize */
  SISInfo.wTListHeadOffset = SISInfo.wTAddrOffset + (WORD)(2 * sizeof(WORD));
  (*pMPAWork++) = SISInfo.wTListHeadOffset; /**< 7. Set type list head offset
                                                    in the MPA memory segment */

  (*pMPAWork) = (WORD)0; /**< 8. Set type list size to zero */
  //}}}

  return 0;

error:
  if (fp) {
    fclose(fp);
  }
  return -1;
} //}}}

DLL_PUBLIC char *MPA_SIS_Init(const char *pszFileName) { //{{{
  int fd = -1;
  DWORD dw = 0;
  char *shmPtr = NULL;

  /**< Open queue information memory map file */
  fd = open(pszFileName, O_RDWR | O_CLOEXEC, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
  check(fd != -1, "Open memory map file error");

  read(fd, &dw, sizeof(DWORD)); /**< Get its size stored in head */
  check(dw > 0, "Invalid memory map file format");

  lseek(fd, 0, SEEK_SET); /**< Reallocate offset to file head */
  /**< Map file to memory */
  shmPtr = mmap(NULL, (size_t)dw, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  check(shmPtr != MAP_FAILED, "Map file to memory failed");

  close(fd);
  return shmPtr; /**< Return memory head address */

error:
  if (fd > 0) {
    close(fd);
  }
  return NULL;
} //}}}

DLL_PUBLIC int MPA_SIS_SInfoAdd(const char *pMPAStart, DWORD sid, key_t qkey,
                                DWORD qtype) { //{{{
  int index = -1;
  int qid = -1;
  MPA_SISInfo SISInfo;
  MPA_SIS_SrvInfo *pSvrInfo = NULL;

  GetSISInfo(pMPAStart, &SISInfo);
  check((*SISInfo.pwSrvInfoSize) < SISInfo.wMaxSvrInfo, "Maximum server info number[%d] reached",
        SISInfo.wMaxSvrInfo);

  index = FindServerInfo(&SISInfo, sid);
  check(index == -1, "Server info[%d] already exists", sid);

  qid = MsqCreate(qkey, C_MsqRW);
  check(qid >= 0, "Cannot create message queue[qkey=%d]", qkey);

  pSvrInfo = SISInfo.pServerInfos + (*SISInfo.pwSrvInfoSize);
  pSvrInfo->dwSid = sid;
  pSvrInfo->dwQkey = qkey;
  pSvrInfo->dwQid = qid;
  pSvrInfo->dwQtype = qtype;
  (*SISInfo.pwSrvInfoSize)++;
  return 0;

error:
  return -1;
} //}}}

DLL_PUBLIC int MPA_SIS_SInfoModify(const char *pMPAStart, DWORD sid, key_t qkey,
                                   DWORD qtype) { //{{{
  int index = -1;
  int qid = -1;
  MPA_SISInfo SISInfo;
  MPA_SIS_SrvInfo *pSvrInfo = NULL;

  GetSISInfo(pMPAStart, &SISInfo);
  index = FindServerInfo(&SISInfo, sid);
  check(index >= 0, "Server info[%d] does not exist", sid);

  qid = MsqCreate(qkey, C_MsqRW);
  check(qid >= 0, "Cannot create message queue[qkey=%d]", qkey);

  pSvrInfo = SISInfo.pServerInfos + index;
  pSvrInfo->dwQkey = qkey;
  pSvrInfo->dwQid = qid;
  pSvrInfo->dwQtype = qtype;
  return 0;

error:
  return -1;
} //}}}

DLL_PUBLIC int MPA_SIS_SInfoDelLast(const char *pMPAStart) { //{{{
  MPA_SISInfo SISInfo;

  GetSISInfo(pMPAStart, &SISInfo);
  if ((*SISInfo.pwSrvInfoSize) > 0) {
    (*SISInfo.pwSrvInfoSize)--;
  }
  return 0;
} //}}}

DLL_PUBLIC int MPA_SIS_TInfoAdd(const char *pMPAStart, DWORD type, DWORD sid) { //{{{
  int index = 0;
  MPA_SISInfo SISInfo;
  MPA_SIS_TypeInfo *pTypeInfo = NULL;

  GetSISInfo(pMPAStart, &SISInfo);
  check((*SISInfo.pwTListSize) < SISInfo.wMaxTypeInfo, "Maximum type info number[%d] reached",
        SISInfo.wMaxTypeInfo);

  index = FindServerInfo(&SISInfo, sid);
  check(index >= 0, "Cannot find server info[%d]", sid);

  pTypeInfo = SISInfo.pTypeInfos + (*SISInfo.pwTListSize);
  pTypeInfo->dwType = type;
  pTypeInfo->wSidIndex = (WORD)index;
  (*SISInfo.pwTListSize)++;
  return 0;

error:
  return -1;
} //}}}

DLL_PUBLIC int MPA_SIS_TInfoModify(const char *pMPAStart, DWORD type, DWORD sid, DWORD new_type,
                                   DWORD new_sid) { //{{{
  int type_index = 0, new_sid_index = 0;
  MPA_SISInfo SISInfo;
  MPA_SIS_TypeInfo *pTypeInfo = NULL;

  GetSISInfo(pMPAStart, &SISInfo);
  type_index = FindTypeInfoBySid(&SISInfo, new_type, new_sid);
  check(type_index < 0, "Type info[%d:%d] already exists", new_type, new_sid);
  type_index = FindTypeInfoBySid(&SISInfo, type, sid);
  check(type_index >= 0, "Type info[%d:%d] does not exist", type, sid);
  new_sid_index = FindServerInfo(&SISInfo, new_sid);
  check(new_sid_index >= 0, "Cannot find server info[%d]", new_sid);
  pTypeInfo = SISInfo.pTypeInfos + type_index;
  pTypeInfo->dwType = new_type;
  pTypeInfo->wSidIndex = (WORD)new_sid_index;
  return 0;

error:
  return -1;
} //}}}

DLL_PUBLIC int MPA_SIS_TInfoDelLast(const char *pMPAStart) { //{{{
  MPA_SISInfo SISInfo;

  GetSISInfo(pMPAStart, &SISInfo);
  if ((*SISInfo.pwTListSize) > 0) {
    (*SISInfo.pwTListSize)--;
  }
  return 0;
} //}}}

DLL_PUBLIC void MPA_SIS_Display(const char *pMPAStart) { //{{{

  MPA_SISInfo SISInfo;

  GetSISInfo(pMPAStart, &SISInfo);
  DisplaySISInfo(&SISInfo);
} //}}}

DLL_PUBLIC int MPA_SIS_End(const char *pMPAStart, Boolean bRelease) { //{{{
  MPA_SISInfo SISInfo;

  GetSISInfo(pMPAStart, &SISInfo);
  if (bRelease == True) {
    int i = 0, numOfServer = 0;
    MPA_SIS_SrvInfo *pSvrInfo;

    numOfServer = (*SISInfo.pwSrvInfoSize);
    for (i = 0; i < numOfServer; i++) {
      pSvrInfo = SISInfo.pServerInfos + i;
      MsqClose(pSvrInfo->dwQid);
      (*SISInfo.pwSrvInfoSize)--;
    }
  } else {
    (*SISInfo.pwSrvInfoSize) = 0;
  }
  (*SISInfo.pwTListSize) = 0;
  return 0;
} //}}}

DLL_PUBLIC int MPA_SIS_LoadConfig(const char *pszSHMFileName, const char *pszFileName) { //{{{
  return LoadFromFile(pszSHMFileName, pszFileName);
} //}}}

DLL_PUBLIC int MPA_SIS_ExportConfig(const char *pMPAStart, const char *pszFileName) { //{{{
  MPA_SISInfo SISInfo;

  GetSISInfo(pMPAStart, &SISInfo);
  return DumpSISInfoToFile(&SISInfo, pszFileName);
} //}}}

DLL_PUBLIC void GetSISInfo(const char *pMPAStart, MPA_SISInfo *pSISInfo) { //{{{
  WORD *pMPAWork = (WORD *)(pMPAStart + sizeof(DWORD));

  pSISInfo->dwTotalSize = *((DWORD *)pMPAStart);
  pSISInfo->wMaxSvrInfo = (*pMPAWork++);
  pSISInfo->wMaxTypeInfo = (*pMPAWork++);
  pSISInfo->wSAddrOffset = (*pMPAWork++);
  pSISInfo->wTAddrOffset = (*pMPAWork++);
  pSISInfo->pwSrvInfoSize = pMPAWork++;
  pSISInfo->pServerInfos = ((MPA_SIS_SrvInfo *)pMPAWork);
  pMPAWork = (WORD *)(pMPAStart + pSISInfo->wTAddrOffset);
  pSISInfo->wTListHeadOffset = (*pMPAWork++);
  pSISInfo->pwTListSize = pMPAWork;
  pSISInfo->pTypeInfos = (MPA_SIS_TypeInfo *)(pMPAStart + pSISInfo->wTListHeadOffset);
} //}}}

DLL_PUBLIC int MPA_GetServerInfo(DWORD sid, MPA_SIS_SrvInfo *pSrvInfo,
                                 const char *pMPAStart) { //{{{
  int index = 0;
  MPA_SISInfo SISInfo;

  GetSISInfo(pMPAStart, &SISInfo);
  if ((index = FindServerInfo(&SISInfo, sid)) == -1) {
    return -2;
  }
  memcpy(pSrvInfo, SISInfo.pServerInfos + index, sizeof(MPA_SIS_SrvInfo));
  return index;
} //}}}

DLL_PUBLIC int MPA_CheckMsgQ(key_t qkey) {
  int nRetCode = MsqGet(qkey, IPC_EXCL);
  if (nRetCode < 0) {
    int err = errno;
    if (err == ENOENT) {
      return -1;
    }

    return -2;
  }
  return 0;
}

DLL_PUBLIC size_t MPA_CheckQKey(key_t qkey, const char *pMPAStart) {
  size_t count = 0, i;
  MPA_SISInfo SISInfo;
  MPA_SIS_SrvInfo *pServerInfo;

  GetSISInfo(pMPAStart, &SISInfo);
  for (i = 0, pServerInfo = SISInfo.pServerInfos; i < (size_t)(*SISInfo.pwSrvInfoSize);
       i++, pServerInfo++) {
    if (pServerInfo->dwQkey == qkey) {
      count++;
    }
  }

  return count;
}

DLL_PUBLIC int MPA_GetServerInfoByIndex(mpa_index_t index, MPA_SIS_SrvInfo *pSrvInfo,
                                        const char *pMPAStart) { //{{{
  MPA_SISInfo SISInfo;

  GetSISInfo(pMPAStart, &SISInfo);
  memcpy(pSrvInfo, SISInfo.pServerInfos + index, sizeof(MPA_SIS_SrvInfo));
  return 0;
} //}}}

DLL_PUBLIC int MPA_GetTypeInfo(mpa_index_t index_, DWORD type, MPA_SIS_TypeInfo *pTypeInfo,
                               const char *pMPAStart) { //{{{
  int index = 0;
  MPA_SISInfo SISInfo;

  GetSISInfo(pMPAStart, &SISInfo);
  if ((index = FindTypeInfo(index_, &SISInfo, type)) == -1) {
    return -2;
  }
  memcpy(pTypeInfo, SISInfo.pTypeInfos + index, sizeof(MPA_SIS_TypeInfo));
  return index;
} //}}}

// Static functions {{{
static int DumpSISInfoToFile(const MPA_SISInfo *pSISInfo,
                             const char *pszFileName) { //{{{
  int i;
  FILE *fp;
  MPA_SIS_SrvInfo *pServerInfos;
  MPA_SIS_TypeInfo *pTypeInfos;

  if ((fp = fopen(pszFileName, "we")) == NULL) {
    fprintf(stderr, "文件[%s]打开失败. [%s]", pszFileName, strerror(errno));
    return -1;
  }
  fprintf(fp, "################################################################"
              "######\n");
  fprintf(fp, "# [main]                                                        "
              "     #\n");
  fprintf(fp, "# max_serverinfo_nums : 最大系统信息数                          "
              "     #\n");
  fprintf(fp, "# max_typeinfo_nums : 最大类型信息数                            "
              "     #\n");
  fprintf(fp, "################################################################"
              "######\n");
  fprintf(fp, "[main]\n");
  fprintf(fp, "max_serverinfo_nums = %d\n", pSISInfo->wMaxSvrInfo);
  fprintf(fp, "max_typeinfo_nums = %d\n", pSISInfo->wMaxTypeInfo);
  fprintf(fp, "\n");
  fprintf(fp, "################################################################"
              "######\n");
  fprintf(fp, "# [server]                                                      "
              "     #\n");
  fprintf(fp, "# server_nums :       进程信息数                                "
              "     #\n");
  fprintf(fp, "# s#=sid:qkey:qtype   进程标识:消息队列键值:消息类型            "
              "     #\n");
  fprintf(fp, "################################################################"
              "######\n");
  fprintf(fp, "[server]\n");
  fprintf(fp, "server_nums=%d\n", (*pSISInfo->pwSrvInfoSize));
  for (i = 0, pServerInfos = pSISInfo->pServerInfos; i < (*pSISInfo->pwSrvInfoSize);
       i++, pServerInfos++) {
    fprintf(fp, "s%d=%d:%d:%d\n", i, pServerInfos->dwSid, pServerInfos->dwQkey,
            pServerInfos->dwQtype);
  }
  fprintf(fp, "\n");
  fprintf(fp, "################################################################"
              "######\n");
  fprintf(fp, "# [type]                                                        "
              "     #\n");
  fprintf(fp, "# type_nums :         类型信息数                                "
              "     #\n");
  fprintf(fp, "# t#=type:sid         交易消息类型:进程标识                     "
              "     #\n");
  fprintf(fp, "################################################################"
              "######\n");
  fprintf(fp, "[type]\n");
  fprintf(fp, "type_nums=%d\n", (*pSISInfo->pwTListSize));
  for (i = 0, pTypeInfos = pSISInfo->pTypeInfos; i < (*pSISInfo->pwTListSize); i++, pTypeInfos++) {
    fprintf(fp, "t%d=%d:%d\n", i, pTypeInfos->dwType,
            (pSISInfo->pServerInfos + pTypeInfos->wSidIndex)->dwSid);
  }
  fprintf(fp, "\n");
  fprintf(fp, "###############################end##############################"
              "######\n");
  fclose(fp);
  return 0;
} //}}}

static void DisplaySISInfo(const MPA_SISInfo *pSISInfo) { //{{{
  int i;
  MPA_SIS_SrvInfo *pServerInfos;
  MPA_SIS_TypeInfo *pTypeInfos;

  printf("+++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("最大系统信息数:%d\n", pSISInfo->wMaxSvrInfo);
  printf("最大交易类型数:%d\n", pSISInfo->wMaxTypeInfo);
  printf("当前系统信息数:%d\n", (*pSISInfo->pwSrvInfoSize));
  printf("|进程索引号|系统标识号|消息队列Key|消息队列ID|消息类型|\n");
  printf("|----------|----------|-----------|----------|--------|\n");
  for (i = 0, pServerInfos = pSISInfo->pServerInfos; i < (*pSISInfo->pwSrvInfoSize);
       i++, pServerInfos++) {
    printf("|%10d|%10d|%11d|0x%08x|%8d|\n", i, pServerInfos->dwSid, pServerInfos->dwQkey,
           pServerInfos->dwQid, pServerInfos->dwQtype);
  }
  printf("当前消息类型数:%d\n", (*pSISInfo->pwTListSize));
  printf("|类型索引号|  类型号  |系统索引号|进程索引号|\n");
  printf("|----------|----------|----------|----------|\n");
  for (i = 0, pTypeInfos = pSISInfo->pTypeInfos; i < (*pSISInfo->pwTListSize); i++, pTypeInfos++) {
    printf("|%10d|%10d|%10d|%10d|\n", i, pTypeInfos->dwType, pTypeInfos->wSidIndex,
           (pSISInfo->pServerInfos + pTypeInfos->wSidIndex)->dwSid);
  }
  printf("+++++++++++++++++++++++++++++++++++++++++++++\n");
} //}}}

static int FindTypeInfo(mpa_index_t index, const MPA_SISInfo *pSISInfo,
                        DWORD type) { //{{{
  int i;
  MPA_SIS_TypeInfo *pTypeInfo;

  for (i = 0 + index, pTypeInfo = (pSISInfo->pTypeInfos + index); i < (*pSISInfo->pwTListSize);
       i++, pTypeInfo++) {
    if (pTypeInfo->dwType == type) {
      return i;
    }
  }
  return -1;
} //}}}

static int FindTypeInfoBySid(const MPA_SISInfo *pSISInfo, DWORD type,
                             DWORD sid) { //{{{
  int index, i;
  MPA_SIS_TypeInfo *pTypeInfo;

  if ((index = FindServerInfo(pSISInfo, sid)) == -1) {
    fprintf(stderr, "该系统标识%d没有注册。请先注册系统信息。\n", sid);
    return -2;
  }
  for (i = 0, pTypeInfo = (pSISInfo->pTypeInfos); i < (*pSISInfo->pwTListSize); i++, pTypeInfo++) {
    if (pTypeInfo->dwType == type && pTypeInfo->wSidIndex == index) {
      return i;
    }
  }
  return -1;
} //}}}

static int FindServerInfo(const MPA_SISInfo *pSISInfo, DWORD sid) { //{{{
  int i;
  MPA_SIS_SrvInfo *pServerInfo;

  for (i = 0, pServerInfo = pSISInfo->pServerInfos; i < (*pSISInfo->pwSrvInfoSize);
       i++, pServerInfo++) {
    if (pServerInfo->dwSid == sid) {
      return i;
    }
  }
  return -1;
} //}}}

static void freeArray(char ***parr, size_t num) {
  if (*parr == NULL) {
    return;
  }

  char **pp = *parr;
  for (size_t i = 0; i < num; i++) {
    free(*(pp + i));
  }
  free(pp);
  *parr = NULL;
}

static int parseServerInfo(const char *sBuf, DWORD *n1, int *n2, DWORD *n3) {
  char **pp = NULL;
  ssize_t m = SplitStrToArray(sBuf, &pp, ":");
  if (m != 3) {
    trace("Server info format error[%s]", sBuf);
    if (m > 0) {
      freeArray(&pp, (size_t)m);
    }
    return -1;
  }

  if (0 != DecimalStrToUInt(*pp, n1)) {
    trace("Server info format error[%s]", sBuf);
    freeArray(&pp, (size_t)m);
    return -1;
  }
  if (0 != DecimalStrToInt(*(pp + 1), n2)) {
    trace("Server info format error[%s]", sBuf);
    freeArray(&pp, (size_t)m);
    return -1;
  }
  if (0 != DecimalStrToUInt(*(pp + 2), n3)) {
    trace("Server info format error[%s]", sBuf);
    freeArray(&pp, (size_t)m);
    return -1;
  }
  freeArray(&pp, (size_t)m);
  return 0;
}

static int parseTypeInfo(const char *sBuf, DWORD *n1, DWORD *n3) {
  char **pp = NULL;
  ssize_t m = SplitStrToArray(sBuf, &pp, ":");
  if (m != 2) {
    trace("Type info format error[%s]", sBuf);
    if (m > 0) {
      freeArray(&pp, (size_t)m);
    }
    return -1;
  }

  if (0 != DecimalStrToUInt(*pp, n1)) {
    trace("Type info format error[%s]", sBuf);
    freeArray(&pp, (size_t)m);
    return -1;
  }
  if (0 != DecimalStrToUInt(*(pp + 1), n3)) {
    trace("Type info format error[%s]", sBuf);
    freeArray(&pp, (size_t)m);
    return -1;
  }
  freeArray(&pp, (size_t)m);
  return 0;
}

static int LoadFromFile(const char *pszSHMFileName,
                        const char *pszINIFileName) { //{{{
  ssize_t nRetCode = -1;
  int i = 0;

  char *pMPAStart = NULL;

  int nMaxServerInfoNums = 0, nMaxTypeInfoNums = 0;
  int nCurServerInfoNums = 99, nCurTypeInfoNums = 99;
  int version = 1;
  char sBuf[1024], sBuf1[11];
  key_t n2 = 0;
  DWORD n1 = 0, n3 = 0;

  check(0 == GetProfileInt(MPA_PF_MAIN_SEC, MPA_PF_MAXSVRINFONUM, 10, pszINIFileName,
                           &nMaxServerInfoNums),
        "Cannot read max server number from file[%s]", pszINIFileName);
  check(nMaxServerInfoNums >= 0, "Invalid max server number[%d]", nMaxServerInfoNums);
  check(nMaxServerInfoNums <= USHRT_MAX, "Invalid max server number[%d]", nMaxServerInfoNums);

  check(0 == GetProfileInt(MPA_PF_MAIN_SEC, MPA_PF_MAXMSGTYPEINFONUM, 100, pszINIFileName,
                           &nMaxTypeInfoNums),
        "Cannot read max type number from file[%s]", pszINIFileName);
  check(nMaxTypeInfoNums >= 0, "Invalid max type number[%d]", nMaxTypeInfoNums);
  check(nMaxTypeInfoNums <= USHRT_MAX, "Invalid max type number[%d]", nMaxTypeInfoNums);

  // create share memory
  nRetCode = MPA_SIS_Create(pszSHMFileName, (size_t)nMaxServerInfoNums, (size_t)nMaxTypeInfoNums);
  check(nRetCode == 0, "Cannot initialize MPA memory map file[%s]", pszSHMFileName);
  pMPAStart = MPA_SIS_Init(pszSHMFileName);
  check(pMPAStart, "Cannot mount MPA memory map file[%s] to memory", pszSHMFileName);

  check(0 == GetProfileInt(MPA_PF_MAIN_SEC, MPA_PF_VERSION, version, pszINIFileName, &version),
        "Cannot read version from file[%s]", pszINIFileName);
  if (version == 2) {
    return LoadFromList(pMPAStart, pszINIFileName, (size_t)nMaxServerInfoNums,
                        (size_t)nMaxTypeInfoNums);
  }

  // Version 1.0 Loading Procedure
  check(0 == GetProfileInt(MPA_PF_SERVER_SEC, MPA_PF_SVRNUM, nCurServerInfoNums, pszINIFileName,
                           &nCurServerInfoNums),
        "Cannot read current server number from file[%s]", pszINIFileName);
  check(0 == GetProfileInt(MPA_PF_MSGTYPE_SEC, MPA_PF_TYPE_NUM, nCurTypeInfoNums, pszINIFileName,
                           &nCurTypeInfoNums),
        "Cannot read current type number from file[%s]", pszINIFileName);

  // load server info
  for (i = 0; i < nCurServerInfoNums; i++) {
    sprintf(sBuf1, "s%d", i);

    if ((nRetCode = GetProfileString(MPA_PF_SERVER_SEC, sBuf1, "", sBuf, 1024, pszINIFileName)) <=
        0) {
      break;
    }

    if (0 != parseServerInfo(sBuf, &n1, &n2, &n3)) {
      continue;
    }

    MPA_SIS_SInfoAdd(pMPAStart, n1, n2, n3);
  }
  // load type info
  for (i = 0; i < nCurTypeInfoNums; i++) {
    sprintf(sBuf1, "t%d", i);

    if ((nRetCode = GetProfileString(MPA_PF_MSGTYPE_SEC, sBuf1, "", sBuf, 1024, pszINIFileName)) <=
        0) {
      break;
    }

    if (0 != parseTypeInfo(sBuf, &n1, &n3)) {
      continue;
    }

    MPA_SIS_TInfoAdd(pMPAStart, n1, n3);
  }
  return 0;

error:
  return -1;
} // }}}

static int LoadFromList(const char *pMPAStart, const char *pszINIFileName, //{{{
                        size_t nMaxServerInfoNums, size_t nMaxTypeInfoNums) {
  node_t *serverList = NULL;
  node_t *typeList = NULL;
  ssize_t serverNums = 0, typeNums = 0;
  char sBuf[1024];
  key_t n2 = 0;
  int qcount = 0;
  DWORD n1 = 0, n3 = 0;

  trace("Loading server information from [%s]...", pszINIFileName);
  serverNums = GetProfileList(MPA_PF_SERVER_SEC, &serverList, nMaxServerInfoNums, pszINIFileName);
  check(serverNums >= 0, "Cannot read list of [%s]", MPA_PF_SERVER_SEC);
  check(serverNums <= USHRT_MAX, "Too many server infos[%ld]", serverNums);
  while (serverList) {
    serverList = remove_node(serverList, sBuf, 1024);

    if (0 != parseServerInfo(sBuf, &n1, &n2, &n3)) {
      continue;
    }

    if (MPA_CheckQKey(n2, pMPAStart) == 0) {
      qcount++;
    }
    MPA_SIS_SInfoAdd(pMPAStart, n1, n2, n3);
  }
  trace("Loading server information...Done.\n"
        ">  Loaded [%d] item(s).\n"
        ">  Created [%d] message queue(s).",
        serverNums, qcount);

  if (serverNums >= (ssize_t)nMaxServerInfoNums) {
    trace("WARNING: Some server informations may not be loaded due to the "
          "max_serverinfo_nums cap setting[%d] in [%s]",
          nMaxServerInfoNums, pszINIFileName);
  }

  trace("Loading type information from [%s]...", pszINIFileName);
  typeNums = GetProfileList(MPA_PF_MSGTYPE_SEC, &typeList, nMaxTypeInfoNums, pszINIFileName);
  check(typeNums >= 0, "Cannot read list of [%s]", MPA_PF_MSGTYPE_SEC);
  check(typeNums <= USHRT_MAX, "Too many type infos[%ld]", typeNums);
  while (typeList) {
    typeList = remove_node(typeList, sBuf, 1024);

    if (0 != parseTypeInfo(sBuf, &n1, &n3)) {
      continue;
    }

    MPA_SIS_TInfoAdd(pMPAStart, n1, n3);
  }
  trace("Loading type information...Done.\n>  Loaded [%d] item(s).", typeNums);

  if (typeNums >= (ssize_t)nMaxTypeInfoNums) {
    trace("WARNING: Some type informations may not be loaded due to the "
          "max_typeinfo_nums cap setting[%d] in [%s]",
          nMaxTypeInfoNums, pszINIFileName);
  }
  return 0;

error:
  if (serverList) {
    close_list(serverList);
  }
  if (typeList) {
    close_list(typeList);
  }
  return -1;
} //}}}
  //}}}

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic pop
#endif

/* vim: set ts=2 sw=2 sts=2 tw=0 expandtab : */
