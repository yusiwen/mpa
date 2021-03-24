#include <stdio.h>
#include <stdlib.h>

#include "mpacli.h"
#include "mpaknl.h"
#include "rscommon/debug.h"
#include "rscommon/strfunc.h"

void usage(void);

void usage() {
  trace("%d", MPA_MESSAGESIZE);
  printf("puber <mpa.mmap> <sid> <message> <message_type>\n");
}

int main(int argc, char **argv) {
  MPAMessage message;
  char buf[256], buf2[256];

  if (argc != 5) {
    usage();
    return -1;
  }

  memset(buf, 0, sizeof(buf));
  memset(buf2, 0, sizeof(buf2));
  strcpy(buf, argv[3]);
  strcpy(buf2, argv[4]);

  DWORD sid = (DWORD)atoi(argv[2]);
  if (0 != MPA_Init(argv[1], sid)) {
    trace("MPA_Init(%s, %d) failed", argv[1], sid);
    return -1;
  }

  MPA_MsgInit(&message);
  MPA_SetMsgProp("p1", "v1", &message);
  MPA_SetMsgProp("p1", "a1", &message);
  MPA_SetMsgProp("p2", "v2", &message);
  MPA_SetMsgProp("p2", "v9", &message);
  MPA_SetMsgProp("p3", "v3", &message);
  MPA_SetMsgProp("p3", "v", &message);
  MPA_SetMsgProp("p4", "v4", &message);
  MPA_SetMsgProp("p5", "v5", &message);
  MPA_SetMsgProp("p5", "v55", &message);
  MPA_SetMsgProp("p6", "v6", &message);
  MPA_SetMsgProp("p6", "a6", &message);

  size_t len = strlen(buf);
  MPA_SetMsgBody(buf, len, &message);
  trace("pub message[%s] with type: %d", buf, atoi(buf2));
  MPA_Pub((DWORD)atoi(buf2), &message);
  ShowBufferHex(message.buf, (size_t)MPA_GetMsgLength(&message));
  return 0;
}

/* vim: set ts=2 sw=2 sts=2 tw=0 expandtab : */
