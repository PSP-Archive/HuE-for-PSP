
int abs(int c)
{
    return (c<0) ? -c: c;
}

// copy from pspsdk
int atoi(const char *s)
{
  int neg = 0, ret = 1;

  for (;ret;++s) {
    switch(*s) {
      case ' ':
      case '\t':
        continue;
      case '-':
        ++neg;
      case '+':
        ++s;
      default:
        ret = 0;
        break;
    }
  }
  /* calculate the integer value. */
  for (; ((*s >= '0') && (*s <= '9')); ) ret = ((ret * 10) + (int)(*s++ - '0'));
  return ((neg == 0) ? ret : -ret);
}

