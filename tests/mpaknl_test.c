/**
 * MPA kernel test module
 *
 * To test IPC memory sharing:
 * 1. Start multiple processes of this module, they will load
 *   the same mpa.mmap file which is mapped to the same area
 *   in the memory.
 * 2. Add some server or type info by menu in process A
 * 3. Show MPA kernel info using '3.Show' menu in another
 *   process, and you can see the change made by process A
 * */
#include "mpaknl.h"
#include "rscommon/strfunc.h"
#include <stdio.h>

void usage(void);

int main(int argc, char const *argv[]) {
  char *pSHM = 0;

  if (argc > 1) {
    if (0 == strcmp(argv[1], "-c")) {
      if (0 != MPA_SIS_Create("mpa.mmap", 10, 10)) {
        printf("Error creating shared memory\n");
        return -1;
      }
    } else if (0 == strcmp(argv[1], "-l")) {
      if (0 != MPA_SIS_LoadConfig((const char *)"mpa.mmap", (const char *)"mpa.ini")) {
        printf("Loading config file error\n");
        return -1;
      }
    } else {
      usage();
      return 0;
    }
  } else {
    pSHM = MPA_SIS_Init("mpa.mmap");
    if (pSHM == NULL) {
      printf("Error mapping shared memory\n");
      return -1;
    }

    int nFlag = 1;
    char c;
    while (nFlag) {
      printf("1.Add server; 2.Add type; 3.Show; 4.Check; 9.End; 0.Quit>");
      scanf(" %c", &c);
      switch (c) {
      case '0':
        nFlag = 0;
        break;
      case '1': {
        DWORD sid, qtype;
        key_t qkey;

        printf("sid>");
        scanf("%d", &sid);
        printf("qkey>");
        scanf("%d", &qkey);
        printf("qtype>");
        scanf("%d", &qtype);

        if (0 != MPA_SIS_SInfoAdd(pSHM, sid, qkey, qtype)) {
          printf("Error adding server info\n");
        }
        continue;
      }
      case '2': {
        DWORD sid, qtype;
        printf("sid>");
        scanf("%d", &sid);
        printf("qtype>");
        scanf("%d", &qtype);

        if (0 != MPA_SIS_TInfoAdd(pSHM, qtype, sid)) {
          printf("Error adding type info\n");
        }
        continue;
      }
      case '3': {
#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#endif

        int size = *(int *)pSHM;

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic pop
#endif
        printf("size=%x, %d\n", size, size);
        printf("sizeof(MPA_SIS_SrvInfo)=%ld\n", sizeof(MPA_SIS_SrvInfo));
        printf("sizeof(MPA_SIS_TypeInfo)=%ld\n", sizeof(MPA_SIS_TypeInfo));
        ShowBufferHex(pSHM, (size_t)size);
        MPA_SIS_Display(pSHM);
        continue;
      }
      case '4': {
        int qkey;
        printf("qkey>");
        scanf("%d", &qkey);
        int n = MPA_CheckMsgQ(qkey);
        switch (n) {
        case -1:
          printf("MsgQ(key=%d) does not exist\n", qkey);
          break;

        case 0:
          printf("MsgQ(key=%d) does exist\n", qkey);
          break;

        default:
          printf("MPA_CheckMsgQueue error\n");
          break;
        }
        continue;
      }
      case '9': {
        MPA_SIS_End(pSHM, True);
        continue;
      }

      default:
        continue;
      }
    }
  }

  return 0;
}

void usage() {
  printf("mpaknl_test [-c|-l|-h]\n"
         "  Load mpa.mmap to memory and enter interactive menu\n"
         "Optional Parameter:\n"
         "  -c Create a empty mpa.mmap of 10 servers and 10 types\n"
         "  -l Load mpa.ini to create mpa.mmap\n"
         "  -h Show this help\n");
}

/* vim: set ts=2 sw=2 sts=2 tw=0 expandtab : */
