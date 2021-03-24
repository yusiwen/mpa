#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#include "mpaknl.h"
#include "rscommon/debug.h"
#include "rscommon/profile.h"
#include "rscommon/strfunc.h"
#include "rscommon/strlist.h"

/****
 * 最大的MQ数量 = MAX_MQ_NUM + 1
 * 2006-06-05， 因亿沃电子充值上线而调整为64个
 *   ---- Changed by yusiwen
 * 2008-08-29,  因一卡充上线调整为96个
 * 2013-08-05,  调整为128个
 * */
#define MAX_MQ_NUM 128

void usage(void);

int parse(char *pszIniFileName);
void insertKey(int key);

void mqshow(void);
void mqclear(void);
void mqkill(void);

static int g_nMQNum = 0;
static int g_MQKeys[MAX_MQ_NUM];

int main(int args, char **argv) {
  char szIniFileName[64], szCommand[16];

  if (args != 3) {
    usage();
    return 0;
  }

  memset(szIniFileName, 0, sizeof(szIniFileName));
  memset(szCommand, 0, sizeof(szCommand));

  strcpy(szIniFileName, argv[1]);
  strcpy(szCommand, argv[2]);

  if (0 != parse(szIniFileName)) {
    debug_err("Cannot parse ini file[%s]!\n", szIniFileName);
    return -1;
  }

  if (strcmp(szCommand, "show") == 0)
    mqshow();
  else if (strcmp(szCommand, "clear") == 0)
    mqclear();
  else if (strcmp(szCommand, "kill") == 0)
    mqkill();
  else
    debug_err("No match command[%s]\n", szCommand);

  return 0;
}

void insertKey(int key) {
  int i;

  for (i = 0; i <= g_nMQNum - 1; i++) {
    if (key == g_MQKeys[i])
      return;
  }
  g_MQKeys[(++g_nMQNum) - 1] = key;
}

int parse(char *pszIniFileName) { // {{{
  int nCurServerInfoNums = -1, i, key, nRetCode, nVersion = 1;
  char sBuf[1024], sBuf1[11];
  char s1[100], s2[100], s3[100];

  check(0 == GetProfileInt(MPA_PF_MAIN_SEC, MPA_PF_VERSION, nVersion, pszIniFileName, &nVersion),
        "Cannot read version number");
  if (nVersion == 1) { /**< Version 1.0 mpa configuration file */
    check(0 == GetProfileInt(MPA_PF_SERVER_SEC, MPA_PF_SVRNUM, nCurServerInfoNums, pszIniFileName,
                             &nCurServerInfoNums),
          "Cannot read current server number");
    if (nCurServerInfoNums < 0)
      return -1;

    // load server info
    for (i = 0; i < nCurServerInfoNums; i++) {
      sprintf(sBuf1, "s%d", i);

      if ((nRetCode = (int)GetProfileString(MPA_PF_SERVER_SEC, sBuf1, "", sBuf, 1024,
                                            pszIniFileName)) <= 0)
        break;

      SplitStr(sBuf, s1, 100, sBuf, 1024, ':');
      SplitStr(sBuf, s2, 100, s3, 100, ':');

      key = atoi(s2);
      insertKey(key);
    }
  } else {
    node_t *serverList = NULL;
    int nMaxServerInfoNums = -1, n1 = 0, n2 = 0, n3 = 0, m = -1;

    check(0 == GetProfileInt(MPA_PF_MAIN_SEC, MPA_PF_MAXSVRINFONUM, 10, pszIniFileName,
                             &nMaxServerInfoNums),
          "Cannot read max server number");

    if (nMaxServerInfoNums < 0)
      return -1;

    trace("Loading server infos...");
    nCurServerInfoNums = (int)GetProfileList(MPA_PF_SERVER_SEC, &serverList,
                                             (size_t)nMaxServerInfoNums, pszIniFileName);
    if (nCurServerInfoNums < 0) {
      debug_err("Cannot read list of [%s]", MPA_PF_SERVER_SEC);
      return -1;
    }

    while (serverList) {
      serverList = remove_node(serverList, sBuf, 1024);

      m = sscanf(sBuf, "%d:%d:%d", &n1, &n2, &n3);
      if (m != 3) {
        debug_err("Server info format error[%s]", sBuf);
        continue;
      }

      insertKey(n2);
    }
    trace("Loading server infos...Done. Loaded [%d] server info(s).", nCurServerInfoNums);
  }

  return 0;

error:
  return -1;
} // }}}

void mqshow() {
  int i = 0;
  int qid;
  char TimeStr[22], szBuf[8];
  struct msqid_ds qds;

  printf("\n");
  // clang-format off
  puts("ID      |IPCKey |Bytes  |Num |Max bytes|LS pid|LR pid|LS Time            |LR Time            |LC TIME            ");
  puts("--------|-------|-------|----|---------|------|------|-------------------|-------------------|-------------------");
  // clang-format on
  for (i = 0; i < g_nMQNum; i++) {
    memset(&qds, 0, sizeof(struct msqid_ds));
    memset(TimeStr, 0, sizeof(TimeStr));
    memset(szBuf, 0, sizeof(szBuf));

    qid = MsqGet(g_MQKeys[i], C_MsqR);
    if (qid < 0) {
      debug_err("Cannot connect to the message queue[%d]!\n", g_MQKeys[i]);
      continue;
    }

    if (MsqInfo(qid, &qds) != 0) {
      debug_err("Can't get message info of queue [ipckey=%d].Err msg:%s\n", g_MQKeys[i],
                strerror(errno));
      continue;
    }

    sprintf(szBuf, "%d#", i);
    printf("%-8s %-7d %07zd %04zd %9zd %-6d %-6d ", szBuf, g_MQKeys[i], qds.msg_cbytes,
           qds.msg_qnum, qds.msg_qbytes, qds.msg_lspid, qds.msg_lrpid);

    if (qds.msg_stime != 0) {
      ConvertTimeToString(TimeStr, 22, "%Y/%m/%d.%H:%M:%S", qds.msg_stime);
      printf("%s ", TimeStr);
    } else
      printf("%-19d ", 0);

    if (qds.msg_rtime != 0) {
      ConvertTimeToString(TimeStr, 22, "%Y/%m/%d.%H:%M:%S", qds.msg_rtime);
      printf("%s ", TimeStr);
    } else
      printf("%-19d ", 0);

    if (qds.msg_ctime != 0) {
      ConvertTimeToString(TimeStr, 22, "%Y/%m/%d.%H:%M:%S", qds.msg_ctime);
      printf("%s\n", TimeStr);
    } else
      printf("%-19d\n", 0);
  }
  printf("\n");
}

void mqclear() {
  int i = 0, nRet;
  int qid;
  char szSuccess[] = "Compeleted";
  char szFail[] = "Failed";
  char szBuf[8];

  printf("\n");
  puts("ID      |IPCKey |Clear Result        ");
  puts("--------|-------|--------------------");
  for (i = 0; i < g_nMQNum; i++) {
    memset(szBuf, 0, sizeof(szBuf));

    qid = MsqGet(g_MQKeys[i], C_MsqRW);
    if (qid < 0) {
      printf("Cannot connect to the message queue[%d]!\n", g_MQKeys[i]);
      continue;
    }

    sprintf(szBuf, "%d#", i);
    printf("%-8s %-7d ", szBuf, g_MQKeys[i]);
    nRet = MsqClear(qid);
    if (nRet != 0)
      printf("%-20s\n", szFail);
    else
      printf("%-20s\n", szSuccess);
  }
}

void mqkill() {
  int i = 0, nRet;
  int qid;
  char szSuccess[] = "Compeleted";
  char szFail[] = "Failed";
  char szBuf[8];

  printf("\n");
  puts("ID      |IPCKey |Remove Result       ");
  puts("--------|-------|--------------------");
  for (i = 0; i < g_nMQNum; i++) {
    memset(szBuf, 0, sizeof(szBuf));

    qid = MsqGet(g_MQKeys[i], C_MsqRW);
    if (qid < 0) {
      printf("Cannot connect to the message queue[%d]!\n", g_MQKeys[i]);
      continue;
    }

    sprintf(szBuf, "%d#", i);
    printf("%-8s %-7d ", szBuf, g_MQKeys[i]);
    nRet = MsqClose(qid);
    if (nRet != 0)
      printf("%-20s\n", szFail);
    else
      printf("%-20s\n", szSuccess);
  }
}

void usage() {
  printf("-----------------------------------------------\n");
  printf("mqm v1.0\n\n");
  printf("Message Queue Manager for YKT2\n");
  printf("(C) 2004-2018, Huateng Software System Co.\n");
  printf("-----------------------------------------------\n");
  printf("USAGE:\n");
  printf("mqm [FILE] [show|clear|kill]\n\n");
  printf("PARAMETERS:\n");
  printf("\tFILE  - MPA configuration file\n");
  printf("\tshow  - display information of all message queues\n");
  printf("\tclear - [CAUTION]clear the content of all message queues\n");
  printf("\tkill  - [CAUTION]remove all message queues\n");
}
