/** @file mpacli.c
 *  @brief Message Process Architecture (MPA) API implementations.
 *
 *  This file contains API implementations of Message Process
 *  Architecture (MPA). These APIs provide message handling operations,
 *  such as: receive, send, publish, etc. All API operations are based
 *  on mpaknl core operations to work.
 *
 *  @see mpacli.h for more details.
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
 *
 *  @date 2017-2-20
 *  - Format variable names
 *  - Make some pointer point to const vars in Getters
 *
 *  @date 2017-2-27
 *  - Add more trace logs
 *
 *  @date 2017-3-18
 *  - Optimize MPA_Send(), MPA_SendSelf(): unifying logic to MPA_Send_Stub()
 *  - Return MPA_ERR_INTR when interrupted while blocking on msgsnd()
 */
// Includes {{{
#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "mpacli.h"
#include "mpaknl.h"
#include "rscommon/debug.h"
// }}}

static DWORD g_sid = 0; /**< Server id of the running process */
/** Pointer to the beginning of memory map
 *  section which contains MPA configurations */
static char *g_pMPAStart = NULL;

static size_t CalculateMsgLength(const MPAMessage *pMessage);
static void GetMsgPart(const MPAMessage *pMessage, MPA_MSG_Head **head, MPA_MSG_Prop **prop,
                       MPA_MSG_Body **body);

DLL_PUBLIC int MPA_Init(const char *pszSHMFileName, DWORD sid) { // {{{
  if (sid <= 0) {
    return MPA_ERR_PARAM;
  }
  g_sid = sid;
  if ((g_pMPAStart = MPA_SIS_Init(pszSHMFileName)) == NULL) {
    return MPA_ERR_INIT;
  }
  return 0;
} // }}}

DLL_PUBLIC int MPA_End(Boolean bRelease) { // {{{
  if (g_pMPAStart == NULL) {
    return MPA_ERR_NOINIT;
  }
  if (0 != MPA_SIS_End(g_pMPAStart, bRelease)) {
    return MPA_ERR_END;
  }

  return 0;
} // }}}

DLL_PUBLIC void MPA_MsgInit(MPAMessage *pMessage) { // {{{
  MPA_MSG_Head *head;
  MPA_MSG_Prop *prop;
  MPA_MSG_Body *body;

  if (pMessage == NULL) {
    return;
  }
  GetMsgPart(pMessage, &head, &prop, &body);
  memset(pMessage, 0, sizeof(MPAMessage));

  head->wMsgID = 0;
  head->dwMsgType = 0;
  head->bMsgMode = 0;
  head->dwSourceID = 0;
  head->dwDestID = 0;
  head->dwReplyTo = 0;
  head->wTimeStamp = 0;
  head->wExpriation = 0;

  prop->wPropLen = 0;
  prop->bOffset = 0;

  body->wBodyLen = 0;

  head->wMsgLen = CalculateMsgLength(pMessage);
} // }}}

/* MPA Message Getters & Setters {{{ */
DLL_PUBLIC DWORD MPA_GetSID() { return g_sid; }

DLL_PUBLIC ssize_t MPA_GetMsgLength(const MPAMessage *pMessage) {
  MPA_MSG_Head *head;
  MPA_MSG_Prop *prop;
  MPA_MSG_Body *body;

  if (pMessage == NULL) {
    return MPA_ERR_PARAM;
  }

  GetMsgPart(pMessage, &head, &prop, &body);
  return (ssize_t)(head->wMsgLen == 0 ? CalculateMsgLength(pMessage) : head->wMsgLen);
}

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

DLL_PUBLIC WORD MPA_GetMsgID(const MPAMessage *pMessage) { return 0; } // NOLINT

DLL_PUBLIC void MPA_SetMsgID(WORD wMsgID, MPAMessage *pMessage) { return; } // NOLINT

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic pop
#endif

DLL_PUBLIC int MPA_GetMsgType(const MPAMessage *pMessage, DWORD *pMsgType) {
  MPA_MSG_Head *head;
  MPA_MSG_Prop *prop;
  MPA_MSG_Body *body;

  if (pMessage == NULL || pMsgType == NULL) {
    return MPA_ERR_PARAM;
  }
  GetMsgPart(pMessage, &head, &prop, &body);
  *pMsgType = head->dwMsgType;
  return 0;
}

DLL_PUBLIC int MPA_GetMsgMode(const MPAMessage *pMessage, BYTE *pMsgMode) {
  MPA_MSG_Head *head;
  MPA_MSG_Prop *prop;
  MPA_MSG_Body *body;

  if (pMessage == NULL || pMsgMode == NULL) {
    return MPA_ERR_PARAM;
  }
  GetMsgPart(pMessage, &head, &prop, &body);
  *pMsgMode = head->bMsgMode;
  return 0;
}

DLL_PUBLIC int MPA_GetMsgSource(const MPAMessage *pMessage, DWORD *pMsgSource) {
  MPA_MSG_Head *head;
  MPA_MSG_Prop *prop;
  MPA_MSG_Body *body;

  if (pMessage == NULL || pMsgSource == NULL) {
    return MPA_ERR_PARAM;
  }
  GetMsgPart(pMessage, &head, &prop, &body);
  *pMsgSource = head->dwSourceID;
  return 0;
}

DLL_PUBLIC int MPA_GetMsgDest(const MPAMessage *pMessage, DWORD *pMsgDest) {
  MPA_MSG_Head *head;
  MPA_MSG_Prop *prop;
  MPA_MSG_Body *body;

  if (pMessage == NULL || pMsgDest == NULL) {
    return MPA_ERR_PARAM;
  }
  GetMsgPart(pMessage, &head, &prop, &body);
  *pMsgDest = head->dwDestID;
  return 0;
}

DLL_PUBLIC int MPA_GetMsgReplyTo(const MPAMessage *pMessage, DWORD *pMsgReplyTo) {
  MPA_MSG_Head *head;
  MPA_MSG_Prop *prop;
  MPA_MSG_Body *body;

  if (pMessage == NULL || pMsgReplyTo == NULL) {
    return MPA_ERR_PARAM;
  }
  GetMsgPart(pMessage, &head, &prop, &body);
  *pMsgReplyTo = head->dwReplyTo;
  return 0;
}

DLL_PUBLIC void MPA_SetMsgReplyTo(DWORD sid, MPAMessage *pMessage) {
  MPA_MSG_Head *head;
  MPA_MSG_Prop *prop;
  MPA_MSG_Body *body;

  if (pMessage == NULL) {
    return;
  }
  GetMsgPart(pMessage, &head, &prop, &body);
  head->dwReplyTo = sid;
}

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

DLL_PUBLIC DWORD MPA_GetMsgTimeStamp(const MPAMessage *pMessage) { return 0; } // NOLINT

DLL_PUBLIC void MPA_SetMsgTimeStamp(WORD wTimeStamp, const MPAMessage *pMessage) {} // NOLINT

DLL_PUBLIC WORD MPA_GetMsgExpiration(const MPAMessage *pMessage) { return 0; } // NOLINT

DLL_PUBLIC void MPA_SetMsgExpiration(WORD wExpTime, MPAMessage *pMessage) {} // NOLINT

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic pop
#endif

DLL_PUBLIC ssize_t MPA_GetMsgProp(const char *pszName, char *pszValue, size_t size,
                                  const MPAMessage *pMessage) {
  MPA_MSG_Head *head = NULL;
  MPA_MSG_Prop *prop = NULL;
  MPA_MSG_Body *body = NULL;
  char *pProp = NULL;
  size_t i = 0;

  if (pszName == NULL) {
    return -1;
  }
  if (pszValue == NULL) {
    return -1;
  }
  if (pMessage == NULL) {
    return -1;
  }

  GetMsgPart(pMessage, &head, &prop, &body);
  pProp = body->text + body->wBodyLen;
  for (i = 0; i < prop->wPropLen; i++, pProp++) {
    if (memcmp(pProp, pszName, strlen(pszName)) == 0) {
      pProp += strlen(pszName);
      if ((*pProp) == '=') {
        pProp++;
        size_t len = strlen(pProp);
        if (len > size - 1) {
          len = size - 1;
        }
        strncpy(pszValue, pProp, len);
        pszValue[len] = '\0';
        return (ssize_t)len;
      }
    }
  }
  return -1;
}

DLL_PUBLIC int MPA_SetMsgProp(const char *pszName, const char *pszValue, MPAMessage *pMessage) {
  MPA_MSG_Head *head = NULL;
  MPA_MSG_Prop *prop = NULL;
  MPA_MSG_Body *body = NULL;
  char *pProp = NULL;
  size_t offset = 0, nSizeOfProp = 0;

  if (pszName == NULL) {
    return MPA_ERR_PARAM;
  }
  if (pszValue == NULL) {
    return MPA_ERR_PARAM;
  }
  if (pMessage == NULL) {
    return MPA_ERR_PARAM;
  }

  size_t lenCurrentMsg = CalculateMsgLength(pMessage);
  size_t lenName = strlen(pszName);
  size_t lenNewValue = strlen(pszValue);
  int found = 0;

  GetMsgPart(pMessage, &head, &prop, &body);
  pProp = body->text + body->wBodyLen;
  // Find the same prop first
  for (offset = 0; offset < prop->wPropLen; offset++, pProp++) {
    if (memcmp(pProp, pszName, lenName) == 0) {
      pProp += lenName;
      if ((*pProp) == '=') { // Found it
        pProp++;
        size_t lenValue = strlen(pProp);
        // Check if new value length will overlap the max mpa message size
        if ((lenCurrentMsg + lenNewValue - lenValue) > MPA_MESSAGESIZE) {
          return MPA_ERR_OUT_OF_RANGE;
        }

        // Get length of remain props
        size_t lenRemain = prop->wPropLen - (offset + lenName + 1 + lenValue + 1);
        if (lenRemain <= 0) {
          // It's the last one, copy it
          strcpy(pProp, pszValue);
          pProp += lenNewValue;
          (*pProp) = '\0';
          prop->wPropLen += (lenNewValue - lenValue);
        } else {
          // It's in middle
          if (lenValue == lenNewValue) {
            strcpy(pProp, pszValue);
          } else {
            // Move remain part to new location
            char *pRemainStart = pProp + lenValue + 1;
            char *pRemainDest = pProp + lenNewValue + 1;
            memmove(pRemainDest, pRemainStart, lenRemain);
            strcpy(pProp, pszValue);
            prop->wPropLen += (lenNewValue - lenValue);
          }
        }
        found = 1;
      }
    }
  }

  // If not found, append the prop at the end
  if (found == 0) {
    // Check if new value length will overlap the max mpa message size
    if ((lenCurrentMsg + lenNewValue + 1) > MPA_MESSAGESIZE) {
      return MPA_ERR_OUT_OF_RANGE;
    }

    pProp = body->text + body->wBodyLen + prop->wPropLen;
    strcpy(pProp, pszName);
    strcat(pProp, "=");
    strcat(pProp, pszValue);
    nSizeOfProp = lenName + lenNewValue + 1;
    pProp += nSizeOfProp;
    (*pProp) = '\0';
    prop->wPropLen += (nSizeOfProp + 1);
  }

  return 0;
}

DLL_PUBLIC char *MPA_GetMsgBody(char *body_, size_t *size, const MPAMessage *pMessage) {
  MPA_MSG_Head *head = NULL;
  MPA_MSG_Prop *prop = NULL;
  MPA_MSG_Body *body = NULL;

  if (size == NULL) {
    return NULL;
  }
  if (pMessage == NULL) {
    return NULL;
  }

  GetMsgPart(pMessage, &head, &prop, &body);
  (*size) = body->wBodyLen;
  if (body_ != NULL) {
    memcpy(body_, body->text, body->wBodyLen);
  }
  return body->text;
}

DLL_PUBLIC int MPA_SetMsgBody(const char *body_, size_t size, MPAMessage *pMessage) {
  MPA_MSG_Head *head = NULL;
  MPA_MSG_Prop *prop = NULL;
  MPA_MSG_Body *body = NULL;

  if (body_ == NULL) {
    return MPA_ERR_PARAM;
  }
  if (pMessage == NULL) {
    return MPA_ERR_PARAM;
  }

  size_t nCurrentMsgLen = CalculateMsgLength(pMessage);
  if ((nCurrentMsgLen + size) > MPA_MESSAGESIZE) {
    return MPA_ERR_OUT_OF_RANGE;
  }

  GetMsgPart(pMessage, &head, &prop, &body);
  if (prop->wPropLen > 0) {
    // move prop
    memmove(body->text + size, body->text + body->wBodyLen, prop->wPropLen);
  }
  body->wBodyLen = size;
  memcpy(body->text, body_, size);
  return 0;
}
  /* MPA Message Getters & Setters }}} */

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

DLL_PUBLIC int MPA_Sub(DWORD type) { // {{{ NOLINT
  // Not implemented yet
  return -1;
} // }}}

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic pop
#endif

static size_t CalculateMsgLength(const MPAMessage *pMessage) {
  MPA_MSG_Head *head;
  MPA_MSG_Prop *prop;
  MPA_MSG_Body *body;

  if (pMessage == NULL) {
    return 0;
  }

  GetMsgPart(pMessage, &head, &prop, &body);
  return sizeof(MPA_MSG_Head) + sizeof(MPA_MSG_Prop) + prop->wPropLen + sizeof(size_t) +
         body->wBodyLen;
}

static int MPA_Send_Stub(DWORD sid, DWORD type,
                         const MPAMessage *pMessage) { // {{{
  MPA_MSG_Head *head;
  MPA_MSG_Prop *prop;
  MPA_MSG_Body *body;
  MPA_SIS_SrvInfo ServerInfo;
  MsgBufDef MsgBuf;
  int nRetCode = -1;

  if (pMessage == NULL) {
    return MPA_ERR_PARAM;
  }

  GetMsgPart(pMessage, &head, &prop, &body);
  head->wMsgLen = CalculateMsgLength(pMessage);
  head->bMsgMode = MPA_SM_P2P;
  head->dwSourceID = g_sid;
  head->dwDestID = sid;

  if (MPA_GetServerInfo(sid, &ServerInfo, g_pMPAStart) < 0) {
    return MPA_ERR_SVRINFO;
  }

  if (type == 0) {
    MsgBuf.mtype = ServerInfo.dwQtype;
  } else {
    MsgBuf.mtype = type;
  }

  memcpy(MsgBuf.mtext, pMessage, sizeof(MPAMessage));
  if ((nRetCode = MsqSend(ServerInfo.dwQid, (T_Msgbuf *)&MsgBuf, head->wMsgLen)) == -1) {
    int err = errno;
    trace("MPA_Send>MsqSend error:%d, errno=%d", nRetCode, err);
    if (err == EINTR) {
      trace("MPA_Send>MsqSend was interrupted");
      return MPA_ERR_INTR;
    }

    if (err == EINVAL || err == EIDRM) {
      trace("MPA_Send>Invalid msqid[%d] or the queue is removed", ServerInfo.dwQid);
      return MPA_ERR_SEND_NOQ;
    }

    if (err == ENOMEM) {
      trace("MPA_Send>Sent message is too big");
      return MPA_ERR_SEND_NOMEM;
    }

    return MPA_ERR_SEND;
  }
  return 0;
} // }}}

DLL_PUBLIC int MPA_Send(DWORD sid, const MPAMessage *pMessage) { // {{{
  return MPA_Send_Stub(sid, 0, pMessage);
} // }}}

DLL_PUBLIC int MPA_SendSelf(DWORD mtype, const MPAMessage *pMessage) { // {{{
  return MPA_Send_Stub(g_sid, mtype, pMessage);
} // }}}

DLL_PUBLIC int MPA_SendSelfEx(const MPAMessage *pMessage) { // {{{
  return MPA_Send(g_sid, pMessage);
} // }}}

DLL_PUBLIC int MPA_Pub(DWORD type, const MPAMessage *pMessage) { // {{{
  MPA_MSG_Head *head;
  MPA_MSG_Prop *prop;
  MPA_MSG_Body *body;
  MPA_SIS_SrvInfo ServerInfo;
  MPA_SIS_TypeInfo TypeInfo;
  MsgBufDef MsgBuf;
  int nRetCode, nCount = 0, nIndex = 0;

  if (pMessage == NULL) {
    return MPA_ERR_PARAM;
  }

  GetMsgPart(pMessage, &head, &prop, &body);
  head->wMsgLen = CalculateMsgLength(pMessage);
  head->bMsgMode = MPA_SM_PUB;
  head->dwSourceID = g_sid;
  head->dwMsgType = type;

  for (;;) {
    nIndex = MPA_GetTypeInfo((mpa_index_t)nIndex, type, &TypeInfo, g_pMPAStart);
    if (nIndex < 0 || nIndex > USHRT_MAX) {
      break; /**< If type is not found, quit */
    }
    nCount++;
    if (MPA_GetServerInfoByIndex((mpa_index_t)TypeInfo.wSidIndex, &ServerInfo, g_pMPAStart) < 0) {
      return (MPA_ERR_TYPEINFO - nIndex);
    }
    MsgBuf.mtype = ServerInfo.dwQtype;
    memcpy(MsgBuf.mtext, pMessage, sizeof(MPAMessage));
    if ((nRetCode = MsqSend(ServerInfo.dwQid, (T_Msgbuf *)&MsgBuf, head->wMsgLen)) == -1) {
      int err = errno;
      trace("MPA_Pub>MsqSend error:%d, errno=%d", nRetCode, err);
      if (err == EINTR) {
        trace("MPA_Pub>MsqSend was interrupted");
        return MPA_ERR_INTR;
      }

      if (err == EINVAL || err == EIDRM) {
        trace("MPA_Send>Invalid msqid[%d] or the queue is removed", ServerInfo.dwQid);
        return MPA_ERR_SEND_NOQ;
      }

      if (err == ENOMEM) {
        trace("MPA_Send>Sent message is too big");
        return MPA_ERR_SEND_NOMEM;
      }

      return (MPA_ERR_SEND - nIndex);
    }
    nIndex++; /**< Search from next index in the next cycle */
  }

  if (nCount == 0 /**< If no type is found, return error */) {
    return MPA_ERR_TYPEINFO;
  }

  return 0;
} // }}}

DLL_PUBLIC ssize_t MPA_Recv(MPAMessage *pMessage) { // {{{
  MPA_SIS_SrvInfo ServerInfo;
  MsgBufDef MsgBuf;
  ssize_t nMsgLen = 0;
  int nRetCode = 0;

  if (pMessage == NULL) {
    return MPA_ERR_PARAM;
  }

  if ((nRetCode = MPA_GetServerInfo(g_sid, &ServerInfo, g_pMPAStart)) < 0) {
    trace("MPA_Recv>GetServerInfo error:%d", nRetCode);
    return MPA_ERR_SVRINFO;
  }

  memset(&MsgBuf, 0, sizeof(MsgBufDef));
  if ((nMsgLen = MsqRecvType(ServerInfo.dwQid, (T_Msgbuf *)&MsgBuf, MsgBufSize,
                             ServerInfo.dwQtype)) < 0) {
    int err = errno;
    trace("MPA_Recv>MsqRecvType error:%d, errno=%d", nMsgLen, err);
    if (err == EINTR) {
      trace("MPA_Recv>MsqRecvType was interrupted");
      return MPA_ERR_INTR;
    }

    if (err == E2BIG) {
      trace("MPA_Recv>Received message is too big for MPAMessage");
      return MPA_ERR_RECV_2BIG;
    }

    if (err == EINVAL || err == EIDRM) {
      trace("MPA_Recv>Invalid msqid[%d] or the queue is removed", ServerInfo.dwQid);
      return MPA_ERR_RECV_NOQ;
    }

    return MPA_ERR_RECV;
  }
  memcpy(pMessage, MsgBuf.mtext, (size_t)nMsgLen);
  return nMsgLen;
} // }}}

DLL_PUBLIC ssize_t MPA_RecvTypeNonBlock(DWORD mtype, MPAMessage *pMessage) { // {{{
  MPA_SIS_SrvInfo ServerInfo;
  MsgBufDef MsgBuf;
  ssize_t nMsgLen = 0;
  int nRetCode = 0;

  if (pMessage == NULL) {
    return MPA_ERR_PARAM;
  }

  if ((nRetCode = MPA_GetServerInfo(g_sid, &ServerInfo, g_pMPAStart)) < 0) {
    trace("MPA_RecvTypeNonBlock>GetServerInfo error:%d", nRetCode);
    return MPA_ERR_SVRINFO;
  }

  memset(&MsgBuf, 0, sizeof(MsgBufDef));
  if ((nMsgLen = MsqRecvTypeNonBlock(ServerInfo.dwQid, (T_Msgbuf *)&MsgBuf, MsgBufSize, mtype)) <
      0) {
    int err = errno;
    trace("MPA_RecvTypeNonBlock>MsqRecvTypeNonBlock error:%d, errno=%d", nMsgLen, err);
    if (err == E2BIG) {
      trace("MPA_RecvTypeNonBlock>Received message is too big for MPAMessage");
      return MPA_ERR_RECV_2BIG;
    }

    if (err == EINVAL) {
      trace("MPA_RecvTypeNonBlock>Invalid msqid[%d]", ServerInfo.dwQid);
      return MPA_ERR_RECV_NOQ;
    }

    if (err == ENOMSG) {
      trace("MPA_RecvTypeNonBlock>No message on the queue when IPC_NOWAIT");
      return MPA_ERR_RECV_NOMSG;
    }

    return MPA_ERR_RECV;
  }
  memcpy(pMessage, MsgBuf.mtext, (size_t)nMsgLen);
  return nMsgLen;
} // }}}

DLL_PUBLIC ssize_t MPA_RecvNonBlock(MPAMessage *pMessage) { // {{{
  MPA_SIS_SrvInfo ServerInfo;
  int nRetCode = 0;

  if (pMessage == NULL) {
    return MPA_ERR_PARAM;
  }

  if ((nRetCode = MPA_GetServerInfo(g_sid, &ServerInfo, g_pMPAStart)) < 0) {
    trace("MPA_RecvNonBlock>GetServerInfo error:%d", nRetCode);
    return MPA_ERR_SVRINFO;
  }

  return MPA_RecvTypeNonBlock(ServerInfo.dwQtype, pMessage);
} // }}}

DLL_PUBLIC int MPA_Validate() {
  MPA_SIS_SrvInfo ServerInfo;
  int nRetCode = 0;

  if ((nRetCode = MPA_GetServerInfo(g_sid, &ServerInfo, g_pMPAStart)) < 0) {
    trace("MPA_Validate>GetServerInfo error:%d", nRetCode);
    return MPA_ERR_SVRINFO;
  }

  return MPA_CheckMsgQ(ServerInfo.dwQkey);
}

DLL_PUBLIC void DumpMPAMessage(const MPAMessage *pMessage) { // {{{
#ifndef _NDUMP
  if (pMessage == NULL) {
    return;
  }

  MPA_MSG_Head *head = NULL;
  MPA_MSG_Prop *prop = NULL;
  MPA_MSG_Body *body = NULL;

  GetMsgPart(pMessage, &head, &prop, &body);

  trace("MPAMessage: {head.wMsgLen=%d, head.wMsgID=%d, head.dwMsgType=%d, "
        "head.bMsgMode=%d, head.dwSourceID=%d, head.dwDestID=%d, head.dwReplyTo=%d, "
        "head.wTimeStamp=%d, head.wExpriation=%d, body.wBodyLen=%d}",
        head->wMsgLen, head->wMsgID, head->dwMsgType, head->bMsgMode, head->dwSourceID,
        head->dwDestID, head->dwReplyTo, head->wTimeStamp, head->wExpriation, body->wBodyLen);
#endif
} // }}}

static void GetMsgPart(const MPAMessage *pMessage, MPA_MSG_Head **head, // {{{
                       MPA_MSG_Prop **prop, MPA_MSG_Body **body) {
  if (pMessage == NULL) {
    return;
  }

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#endif

  const char *mpa_start = (const char *)pMessage;
  (*head) = (MPA_MSG_Head *)mpa_start;
  (*prop) = (MPA_MSG_Prop *)(mpa_start + sizeof(MPA_MSG_Head));
  (*body) = (MPA_MSG_Body *)(mpa_start + sizeof(MPA_MSG_Head) + sizeof(MPA_MSG_Prop));

#if defined(__clang__) ||                                                                          \
    (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic pop
#endif
} // }}}
/* vim: set ts=2 sw=2 sts=2 tw=0 expandtab : */
