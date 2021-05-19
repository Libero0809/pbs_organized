

#ifndef __STRINGUTILS_H_
#define __STRINGUTILS_H_

#include <stdarg.h>
#include <string.h>
#include <string>

/* Only support char array */
std::string strJoin(int count, const char *delimiter, ...) {
  static char storage[MAX_FILENAME_LEN];
  memset(storage, 0, MAX_FILENAME_LEN);

  va_list args;
  /* Initialize the argument list. */
  va_start(args, delimiter);
  int k = 0;
  int dl = strlen(delimiter);

  for (int i = 0; i < count; i++) {
    const char *str = va_arg(args, const char *);
    auto len = strlen(str);
    strncpy(storage + k, str, len);
    k += len;
    strncpy(storage + k, delimiter, dl);
    k += dl;
  }
  va_end(args);
  return string(storage);
}

#endif // __STRINGUTILS_H_
