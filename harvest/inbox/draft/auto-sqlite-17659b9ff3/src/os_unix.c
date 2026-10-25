// AUTO-DRAFT from sqlite/sqlite PR #9410bfb94645c995f604bfc1032c9955ab51e0fb
  UNUSED_PARAMETER(NotUsed);
  return microseconds;
#else
  int seconds = (microseconds+999999)/1000000;  // <<< BUG ANCHOR
  sleep(seconds);
  UNUSED_PARAMETER(NotUsed);
  return seconds*1000000;
