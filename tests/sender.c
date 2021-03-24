#include <stdio.h>
#include <stdlib.h>

#include "mpacli.h"
#include "rscommon/debug.h"

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

int main(int argc, char **argv) {
  MPAMessage message;
  MPA_Init("mpa.mmap", 1000);

  MPA_MsgInit(&message);
  MPA_SetMsgProp("socketid", "12345", &message);
  MPA_SetMsgProp("asd", "234", &message);
  MPA_SetMsgBody(argv[2], strlen(argv[2]) + 1, &message);
  trace("Send to %d", atoi(argv[1]));
  MPA_Send((DWORD)atoi(argv[1]), &message);
  return 0;
}

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic pop
#endif

/* vim: set ts=2 sw=2 sts=2 tw=0 expandtab : */
