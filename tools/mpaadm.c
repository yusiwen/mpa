#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "mpaknl.h"
#include "rscommon/strfunc.h"

#define COMMAND_MAX 1025
#define COMMAND_PART_MAX 65

static int CreateServer(int argc, char **argv);
static int ParserCommand(const char *s, int *pNum, char ***pppCmds);
static int HandleCommand(const char *pszSHMFileName, int argc, char **argv);
static int FormatCommandString(char *pszCommand);
static int FreeCommandBuf(int num, char **ppCmds);
static int Interact(const char *pszSHMFileName);
static void CommandHelp(void);
static void CopyRight(void);

static void Usage(char *sAppName) {
  printf("Usage:%s FILE {init|s+|s=|s-|t+|t=|t-|load|export|show|end args "
         "...}\n",
         sAppName);
  puts("FILE: 共享内存文件");
  CommandHelp();
}

static void CommandHelp() {
  puts("init: 初始化共享内存");
  puts("\tinit max_server_nums max_type_nums");
  puts("s+: 添加服务器信息");
  puts("\ts+ sid qkey qtype");
  puts("s=: 修改服务器信息");
  puts("\ts= sid new-qkey new-qtype");
  puts("s-: 删除最后一条服务器信息");
  puts("\ts-");
  puts("t+: 添加类型信息");
  puts("\tt+ type sid");
  puts("t=: 修改类型信息");
  puts("\tt= old-type old-sid new-type new-sid");
  puts("t-: 删除最后一条类型信息");
  puts("\tt-");
  puts("load: 从指定文件装载配置信息");
  puts("\tload filename");
  puts("export: 将当前配置信息导出到指定文件");
  puts("\texport filename");
  puts("show: 显示当前配置信息");
  puts("\tshow");
  puts("end: 清除所有配置信息，并释放使用的消息队列(不指定norelease选项)");
  puts("\tend <norelease> ");
}

static void CopyRight() {
  puts("Message Process Agent (MPA) 运行环境管理工具。<命令行模式>");
  puts("华腾软件系统有限公司。Copyright 1993-2003,2006,2010,2016,2018");
}

int main(int argc, char **argv) {
  int nRetCode;

  if ((nRetCode = CreateServer(argc, argv)) != 0) {
    return nRetCode;
  }
  return 0;
}

static int CreateServer(int argc, char **argv) {
  int nRetCode = -1;
  char *mpa_start;

  // Get share memory filename,init mpa env
  if (argc < 2) {
    fprintf(stderr, "命令行参数无效\n");
    Usage(argv[0]);
    return -1;
  }
  if (argc < 3) {
    puts("进入命令行模式。");
    CopyRight();
    return Interact(argv[1]);
  }
  // parse command
  if (strcmp(argv[2], "init") == 0) {
    int snum = 0, tnum = 0;
    if (0 != DecimalStrToInt(argv[3], &snum)) {
      fprintf(stderr, "无效的Server数量%s\n", argv[3]);
      return -2;
    }
    if (0 != DecimalStrToInt(argv[4], &tnum)) {
      fprintf(stderr, "无效的MessageType数量%s\n", argv[4]);
      return -2;
    }
    if (snum <= 0 || tnum <= 0) {
      fprintf(stderr, "无效的参数(<=0)\n");
      return -2;
    }
    if ((nRetCode = MPA_SIS_Create(argv[1], (size_t)snum, (size_t)tnum)) != 0) {
      fprintf(stderr, "MPA环境创建失败，错误码%d\n", nRetCode);
      return -2;
    }
  } else if (strcmp(argv[2], "s+") == 0) {
    if (argc < 6) {
      fprintf(stderr, "命令行参数无效\n");
      Usage(argv[0]);
      return -1;
    }
    if ((mpa_start = MPA_SIS_Init(argv[1])) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    int n2 = -1;
    DWORD n1 = 0, n3 = 0;
    if (0 != DecimalStrToUInt(argv[3], &n1)) {
      return -3;
    }
    if (0 != DecimalStrToInt(argv[4], &n2)) {
      return -3;
    }
    if (0 != DecimalStrToUInt(argv[5], &n3)) {
      return -3;
    }
    if ((nRetCode = MPA_SIS_SInfoAdd(mpa_start, n1, n2, n3)) != 0) {
      fprintf(stderr, "添加服务器信息失败，错误码%d\n", nRetCode);
      return -3;
    }
  } else if (strcmp(argv[2], "s=") == 0) {
    if (argc < 6) {
      fprintf(stderr, "命令行参数无效\n");
      Usage(argv[0]);
      return -1;
    }
    if ((mpa_start = MPA_SIS_Init(argv[1])) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    int n2 = -1;
    DWORD n1 = 0, n3 = 0;
    if (0 != DecimalStrToUInt(argv[3], &n1)) {
      return -3;
    }
    if (0 != DecimalStrToInt(argv[4], &n2)) {
      return -3;
    }
    if (0 != DecimalStrToUInt(argv[5], &n3)) {
      return -3;
    }
    if ((nRetCode = MPA_SIS_SInfoModify(mpa_start, n1, n2, n3)) != 0) {
      fprintf(stderr, "修改服务器信息失败，错误码%d\n", nRetCode);
      return -3;
    }
  } else if (strcmp(argv[2], "s-") == 0) {
    if ((mpa_start = MPA_SIS_Init(argv[1])) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    if ((nRetCode = MPA_SIS_SInfoDelLast(mpa_start)) != 0) {
      fprintf(stderr, "删除服务器信息失败，错误码%d\n", nRetCode);
      return -3;
    }
  } else if (strcmp(argv[2], "t+") == 0) {
    if (argc < 5) {
      fprintf(stderr, "命令行参数无效\n");
      Usage(argv[0]);
      return -1;
    }
    if ((mpa_start = MPA_SIS_Init(argv[1])) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    DWORD n1 = 0, n3 = 0;
    if (0 != DecimalStrToUInt(argv[3], &n1)) {
      return -3;
    }
    if (0 != DecimalStrToUInt(argv[5], &n3)) {
      return -3;
    }
    if ((nRetCode = MPA_SIS_TInfoAdd(mpa_start, n1, n3)) != 0) {
      fprintf(stderr, "添加类型信息失败，错误码%d\n", nRetCode);
      return -4;
    }
  } else if (strcmp(argv[2], "t=") == 0) {
    if (argc < 7) {
      fprintf(stderr, "命令行参数无效\n");
      Usage(argv[0]);
      return -1;
    }
    if ((mpa_start = MPA_SIS_Init(argv[1])) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    DWORD n1 = 0, n2 = 0, n3 = 0, n4 = 0;
    if (0 != DecimalStrToUInt(argv[3], &n1)) {
      return -3;
    }
    if (0 != DecimalStrToUInt(argv[4], &n2)) {
      return -3;
    }
    if (0 != DecimalStrToUInt(argv[5], &n3)) {
      return -3;
    }
    if (0 != DecimalStrToUInt(argv[6], &n4)) {
      return -3;
    }
    if ((nRetCode = MPA_SIS_TInfoModify(mpa_start, n1, n2, n3, n4)) != 0) {
      fprintf(stderr, "修改类型信息失败，错误码%d\n", nRetCode);
      return -4;
    }
  } else if (strcmp(argv[2], "t-") == 0) {
    if ((mpa_start = MPA_SIS_Init(argv[1])) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    if ((nRetCode = MPA_SIS_TInfoDelLast(mpa_start)) != 0) {
      fprintf(stderr, "删除类型信息失败，错误码%d\n", nRetCode);
      return -4;
    }
  } else if (strcmp(argv[2], "show") == 0) {
    if ((mpa_start = MPA_SIS_Init(argv[1])) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    MPA_SIS_Display(mpa_start);
  } else if (strcmp(argv[2], "load") == 0) {
    if (argc < 4) {
      fprintf(stderr, "命令行参数无效\n");
      Usage(argv[0]);
      return -1;
    }
    if ((nRetCode = MPA_SIS_LoadConfig(argv[1], argv[3])) != 0) {
      fprintf(stderr, "导入服务器信息失败，错误码%d\n", nRetCode);
      return -5;
    }
  } else if (strcmp(argv[2], "export") == 0) {
    if ((mpa_start = MPA_SIS_Init(argv[1])) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    if ((nRetCode = MPA_SIS_ExportConfig(mpa_start, argv[3])) != 0) {
      fprintf(stderr, "导出服务器信息失败，错误码%d\n", nRetCode);
      return -5;
    }
  } else if (strcmp(argv[2], "end") == 0) {
    Boolean bRelease = True;
    if ((mpa_start = MPA_SIS_Init(argv[1])) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    if ((argc > 3) && (strcmp(argv[3], "norelease") == 0)) {
      bRelease = False;
    }
    if ((nRetCode = MPA_SIS_End(mpa_start, bRelease)) != 0) {
      fprintf(stderr, "注销系统信息失败，错误码%d\n", nRetCode);
      return -4;
    }
  }

  return nRetCode;
}

static int Interact(const char *pszSHMFileName) {
  int nNumOfCmd = 0;
  char szCommand[COMMAND_MAX], **ppCommands = NULL;
  char szPrompt[11] = ">>";
  for (;;) {
    printf("%s", szPrompt);
    fgets(szCommand, COMMAND_MAX - 1, stdin);
    if (OK != ParserCommand(szCommand, &nNumOfCmd, &ppCommands)) {
      continue;
    }
    if ((strcmp(ppCommands[0], "quit") == 0) || (strcmp(ppCommands[0], "exit") == 0)) {
      puts("bye");
      break;
    }
    HandleCommand(pszSHMFileName, nNumOfCmd, ppCommands);
  }
  return FreeCommandBuf(nNumOfCmd, ppCommands);
}

static int ParserCommand(const char *s, int *pNum, char ***pppCmds) {
  int i = 0, size = 1;
  char szCommandTemp[COMMAND_MAX];
  char **ppCmdTemp = NULL;

  FreeCommandBuf((*pNum), (*pppCmds));
  strcpy(szCommandTemp, s);
  // format sCommanTemp
  size = FormatCommandString(szCommandTemp);
  if (size <= 0) {
    return FAILED;
  }
  (*pNum) = size;
  (*pppCmds) = (char **)malloc(sizeof(char *) * (size_t)size);
  ppCmdTemp = (*pppCmds);
  for (i = 0; i < size; i++, ppCmdTemp++) {
    (*ppCmdTemp) = (char *)malloc(sizeof(char) * COMMAND_PART_MAX);
    SplitStr(szCommandTemp, (*ppCmdTemp), COMMAND_PART_MAX, szCommandTemp, COMMAND_MAX, '|');
  }
  return OK;
}

static int FormatCommandString(char *pszCommand) {
  size_t len = strlen(pszCommand), i = 0;
  char *s1, *s2;
  int size = 1;

  s1 = s2 = pszCommand;
  for (i = 0; i < len; i++) {
    if ((*s2) != ' ') {
      s2++;
      continue;
    }
    if (((*s1) != ' ' && (*s1) != '|') || (s2 - s1) > 1) {
      s1 = s2;
      (*s1) = '|';
      size++;
    } else {
      // move next chars
      strcpy(s2, s2 + 1);
    }
  }
  // if last char is |, remove this char and size --
  len = strlen(pszCommand);
  for (i = 0, s1 = pszCommand + len - 1; i < len; i++, s1--) {
    if (*s1 == '\r' || *s1 == '\n' || *s1 == ' ') {
      (*s1) = '\0';
    } else if (*s1 == '|') {
      (*s1) = '\0';
      size--;
    } else {
      break;
    }
  }
  return size;
}

static int FreeCommandBuf(int num, char **ppCmds) {
  int i = 0;
  char **ppCmdsTemp = ppCmds;

  if (ppCmdsTemp == NULL) {
    return -1;
  }

  for (i = 0; i < num; i++, ppCmdsTemp++) {
    free(*ppCmdsTemp);
    (*ppCmdsTemp) = NULL;
  }
  free(ppCmds);
  ppCmds = NULL;

  return 0;
}

static int HandleCommand(const char *pszSHMFileName, int argc, char **argv) {
  int nRetCode = -1;
  char *mpa_start;

  // parse command
  if (argc < 1) {
    return -1;
  }
  if (strcmp(argv[0], "init") == 0) {
    int snum = 0, tnum = 0;
    if (0 != DecimalStrToInt(argv[3], &snum)) {
      fprintf(stderr, "无效的Server数量%s\n", argv[3]);
      return -2;
    }
    if (0 != DecimalStrToInt(argv[4], &tnum)) {
      fprintf(stderr, "无效的MessageType数量%s\n", argv[4]);
      return -2;
    }
    if (snum <= 0 || tnum <= 0) {
      fprintf(stderr, "无效的参数(<=0)\n");
      return -2;
    }
    if ((nRetCode = MPA_SIS_Create(pszSHMFileName, (size_t)snum, (size_t)tnum)) != 0) {
      fprintf(stderr, "MPA环境创建失败，错误码%d\n", nRetCode);
      return -2;
    }
  } else if (strcmp(argv[0], "s+") == 0) {
    if (argc < 4) {
      fprintf(stderr, "命令行参数无效\n");
      return -1;
    }
    if ((mpa_start = MPA_SIS_Init(pszSHMFileName)) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    int n2 = -1;
    DWORD n1 = 0, n3 = 0;
    if (0 != DecimalStrToUInt(argv[1], &n1)) {
      return -3;
    }
    if (0 != DecimalStrToInt(argv[2], &n2)) {
      return -3;
    }
    if (0 != DecimalStrToUInt(argv[3], &n3)) {
      return -3;
    }
    if ((nRetCode = MPA_SIS_SInfoAdd(mpa_start, n1, n2, n3)) != 0) {
      fprintf(stderr, "添加服务器信息失败，错误码%d\n", nRetCode);
      return -3;
    }
  } else if (strcmp(argv[0], "s=") == 0) {
    if (argc < 4) {
      fprintf(stderr, "命令行参数无效\n");
      return -1;
    }
    if ((mpa_start = MPA_SIS_Init(pszSHMFileName)) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    int n2 = -1;
    DWORD n1 = 0, n3 = 0;
    if (0 != DecimalStrToUInt(argv[1], &n1)) {
      return -3;
    }
    if (0 != DecimalStrToInt(argv[2], &n2)) {
      return -3;
    }
    if (0 != DecimalStrToUInt(argv[3], &n3)) {
      return -3;
    }
    if ((nRetCode = MPA_SIS_SInfoModify(mpa_start, n1, n2, n3)) != 0) {
      fprintf(stderr, "修改服务器信息失败，错误码%d\n", nRetCode);
      return -3;
    }
  } else if (strcmp(argv[0], "s-") == 0) {
    if ((mpa_start = MPA_SIS_Init(pszSHMFileName)) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    if ((nRetCode = MPA_SIS_SInfoDelLast(mpa_start)) != 0) {
      fprintf(stderr, "删除服务器信息失败，错误码%d\n", nRetCode);
      return -3;
    }
  } else if (strcmp(argv[0], "t+") == 0) {
    if (argc < 3) {
      fprintf(stderr, "命令行参数无效\n");
      return -1;
    }
    if ((mpa_start = MPA_SIS_Init(pszSHMFileName)) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    DWORD n1 = 0, n3 = 0;
    if (0 != DecimalStrToUInt(argv[1], &n1)) {
      return -3;
    }
    if (0 != DecimalStrToUInt(argv[3], &n3)) {
      return -3;
    }
    if ((nRetCode = MPA_SIS_TInfoAdd(mpa_start, n1, n3)) != 0) {
      fprintf(stderr, "添加类型信息失败，错误码%d\n", nRetCode);
      return -4;
    }
  } else if (strcmp(argv[0], "t=") == 0) {
    if (argc < 5) {
      fprintf(stderr, "命令行参数无效\n");
      return -1;
    }
    if ((mpa_start = MPA_SIS_Init(pszSHMFileName)) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    DWORD n1 = 0, n2 = 0, n3 = 0, n4 = 0;
    if (0 != DecimalStrToUInt(argv[3], &n1)) {
      return -3;
    }
    if (0 != DecimalStrToUInt(argv[4], &n2)) {
      return -3;
    }
    if (0 != DecimalStrToUInt(argv[5], &n3)) {
      return -3;
    }
    if (0 != DecimalStrToUInt(argv[6], &n4)) {
      return -3;
    }
    if ((nRetCode = MPA_SIS_TInfoModify(mpa_start, n1, n2, n3, n4)) != 0) {
      fprintf(stderr, "修改类型信息失败，错误码%d\n", nRetCode);
      return -4;
    }
  } else if (strcmp(argv[0], "t-") == 0) {
    if ((mpa_start = MPA_SIS_Init(pszSHMFileName)) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    if ((nRetCode = MPA_SIS_TInfoDelLast(mpa_start)) != 0) {
      fprintf(stderr, "删除类型信息失败，错误码%d\n", nRetCode);
      return -4;
    }
  } else if (strcmp(argv[0], "show") == 0) {
    if ((mpa_start = MPA_SIS_Init(pszSHMFileName)) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    MPA_SIS_Display(mpa_start);
  } else if (strcmp(argv[0], "load") == 0) {
    if (argc < 2) {
      fprintf(stderr, "命令行参数无效\n");
      return -1;
    }
    if ((nRetCode = MPA_SIS_LoadConfig(pszSHMFileName, argv[1])) != 0) {
      fprintf(stderr, "导入服务器信息失败，错误码%d\n", nRetCode);
      return -5;
    }
  } else if (strcmp(argv[0], "export") == 0) {
    if (argc < 2) {
      fprintf(stderr, "命令行参数无效\n");
      return -1;
    }
    if ((mpa_start = MPA_SIS_Init(pszSHMFileName)) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    if ((nRetCode = MPA_SIS_ExportConfig(mpa_start, argv[1])) != 0) {
      fprintf(stderr, "导出服务器信息失败，错误码%d\n", nRetCode);
      return -5;
    }
  } else if (strcmp(argv[0], "end") == 0) {
    Boolean bRelease = True;
    if ((mpa_start = MPA_SIS_Init(pszSHMFileName)) == NULL) {
      fprintf(stderr, "MPA初始化失败，错误码%d\n", errno);
      return -2;
    }
    if ((argc > 1) && (strcmp(argv[1], "norelease") == 0)) {
      bRelease = False;
    }
    if ((nRetCode = MPA_SIS_End(mpa_start, bRelease)) != 0) {
      fprintf(stderr, "注销系统信息失败，错误码%d\n", nRetCode);
      return -4;
    }
  } else if (strcmp(argv[0], "help") == 0) {
    CommandHelp();
  } else {
    puts("无效参数.");
  }
  return nRetCode;
}
/* vim: set ts=2 sw=2 sts=2 tw=0 expandtab : */
