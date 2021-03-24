#include <stdio.h>
#include <stdlib.h>

#include "mpacli.h"
#include "rscommon/debug.h"
#include "rscommon/strfunc.h"

void usage(void);

void usage() { printf("recver <mpa.mmap> <sid>\n"); }

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

int main(int argc, char **argv) {
  MPAMessage message, req;
  char s[2048], s1[1024];
  size_t i = 0;

  MPA_Init(argv[1], (DWORD)atoi(argv[2]));

  MPA_MsgInit(&req);
  MPA_SetMsgProp("test_prop2", "test for sendself prop", &req);
  MPA_SetMsgBody("test for sendself", 18, &req);
  MPA_SendSelfEx(&req);

  do {
    MPA_MsgInit(&message);
    memset(s, 0, sizeof(s));
    memset(s1, 0, sizeof(s1));
    MPA_Recv(&message);
    trace("Message received!");
    ShowBufferHex(message.buf, (size_t)MPA_GetMsgLength(&message));
    i = 1024;
    MPA_GetMsgProp("test_prop2", s1, i, &message);
    MPA_GetMsgBody(s, &i, &message);
    BYTE msgMode;
    DWORD msgType;
    MPA_GetMsgMode(&message, &msgMode);
    MPA_GetMsgType(&message, &msgType);
    trace("recv(%s): type=%d,text='%s'(%d),prop='%s'", (msgMode == MPA_SM_P2P) ? "P2P" : "Pub/Sub",
          msgType, s, i, s1);
  } while (strcmp(s, "quit") != 0);
  return 0;
}

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic pop
#endif

/* vim: set ts=2 sw=2 sts=2 tw=0 expandtab : */
