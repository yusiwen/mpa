#include <stdio.h>
#include <string.h>

#include "mpacli.h"
#include "rscommon/commonbase.h"
#include "version.h"

DLL_PUBLIC void mpa_getVersion(int *major, int *minor, int *patch, char *meta) {
  *major = MPA_VERSION_MAJOR;
  *minor = MPA_VERSION_MINOR;
  *patch = MPA_VERSION_PATCH;

  if (meta) {
    strcpy(meta, MPA_VERSION_META);
  }
}

DLL_PUBLIC char *mpa_getBuildInfo(char *buffer, size_t len) {
  if (buffer) {
    snprintf(buffer, len, "%d.%d.%d-%s (Build on '%s' at '%s', %s %s UTC)", MPA_VERSION_MAJOR,
             MPA_VERSION_MINOR, MPA_VERSION_PATCH, MPA_VERSION_META, MPA_VERSION_REV,
             MPA_VERSION_BRANCH, MPA_BUILD_DATE, MPA_BUILD_TIME);
    return buffer;
  }

  return NULL;
}
/* vim: set ts=2 sw=2 sts=2 tw=0 expandtab : */
