#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "include/bool.h"
#include "include/dbf.h"

int main() {
  DBF_HANDLE handle = dbf_open("testdbf.dbf", nullptr);
  BOOL ok = (handle != nullptr);
  printf("%d\n",ok);
  if(ok)
      dbf_close(&handle);
  return 0;
}

