#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#include "rscommon/commonbase.h"
#include "rscommon/msq.h"

void usage(void);
int getSize(int qkey);

void usage() {
  printf("Get message numbers in a given queue\n"
         "  Usage: qnum QUEUE_KEY\n");
}

int getSize(int qkey) {
  int qid;
  struct msqid_ds qds;

  memset(&qds, 0, sizeof(struct msqid_ds));

  qid = MsqGet(qkey, C_MsqR);
  if (qid < 0) {
    return -1;
  }

  if (MsqInfo(qid, &qds) != 0) {
    return -2;
  }

  return (int)qds.msg_qnum;
}

int main(int args, char **argv) {
  int nRetCode = -1;

  if (args != 2) {
    usage();
    exit(-1);
  }

  if ((nRetCode = getSize(atoi(argv[1]))) < 0) {
    exit(-2);
  }

  printf("%d\n", nRetCode);
  return (0);
}
