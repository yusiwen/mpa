#ifndef __MPA_TYPE__
#define __MPA_TYPE__

#ifdef __cplusplus
extern "C" {
#endif

#include "rscommon/commonbase.h"
#include "rscommon/msq.h"

#define MsgBufDef T_MsgbufM
#define MsgBufSize C_MsgbufM

typedef struct MPA_MSG_Head {
  size_t wMsgLen;
  WORD wMsgID;
  DWORD dwMsgType;
  BYTE bMsgMode;
  DWORD dwSourceID;
  DWORD dwDestID;
  DWORD dwReplyTo;
  time_t wTimeStamp;
  time_t wExpriation;
} MPA_MSG_Head;

typedef struct MPA_MSG_Prop {
  size_t wPropLen;
  BYTE bOffset;
} MPA_MSG_Prop;

typedef struct MPA_MSG_Body {
  size_t wBodyLen;
  char text[1];
} MPA_MSG_Body;

#ifdef __cplusplus
}
#endif

#endif
/* vim: set ts=2 sw=2 sts=2 tw=0 expandtab : */
