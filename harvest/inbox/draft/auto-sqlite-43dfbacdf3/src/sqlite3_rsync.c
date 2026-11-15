// AUTO-DRAFT from sqlite/sqlite PR #b5918a2af3db467924224506dcf51b801954a9a8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static void win32_fatal_error(const char *zMsg){
  fprintf(stderr, "%s", zMsg);
  exit(1);
}
/* …（同文件无关代码省略）… */
# define PTR_TO_INT(X)  ((int)(__PTRDIFF_TYPE__)(X))
/* …（同文件无关代码省略）… */
static int win32_create_child_process(
  wchar_t *zCmd,       /* The command that the child process will run */
  HANDLE hIn,          /* Standard input */
  HANDLE hOut,         /* Standard output */
  HANDLE hErr,         /* Standard error */
  DWORD *pChildPid     /* OUT: Child process handle */
){
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  BOOL rc;

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  SetHandleInformation(hIn, HANDLE_FLAG_INHERIT, TRUE);
  si.hStdInput  = hIn;
  SetHandleInformation(hOut, HANDLE_FLAG_INHERIT, TRUE);
  si.hStdOutput = hOut;
  SetHandleInformation(hErr, HANDLE_FLAG_INHERIT, TRUE);
  si.hStdError  = hErr;
  rc = CreateProcessW(
     NULL,  /* Application Name */
     zCmd,  /* Command-line */
     NULL,  /* Process attributes */
     NULL,  /* Thread attributes */
     TRUE,  /* Inherit Handles */
     0,     /* Create flags  */
     NULL,  /* Environment */
     NULL,  /* Current directory */
     &si,   /* Startup Info */
     &pi    /* Process Info */
  );
  if( rc ){
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    *pChildPid = pi.dwProcessId;
  }else{
    win32_fatal_error("cannot create child process");
  }
  return rc!=0;
}
void *win32_utf8_to_unicode(const char *zUtf8){
  int nByte = MultiByteToWideChar(CP_UTF8, 0, zUtf8, -1, 0, 0);
  wchar_t *zUnicode = malloc( nByte*2 );
  MultiByteToWideChar(CP_UTF8, 0, zUtf8, -1, zUnicode, nByte);
  return zUnicode;
}
/* …（同文件无关代码省略）… */
static int popen2(
  const char *zCmd,      /* Command to run in the child process */
  FILE **ppIn,           /* Read from child using this file descriptor */
  FILE **ppOut,          /* Write to child using this file descriptor */
  int *pChildPid,        /* PID of the child process */
  int bDirect            /* 0: run zCmd as a shell cmd.  1: run directly */
){
#ifdef _WIN32
  HANDLE hStdinRd, hStdinWr, hStdoutRd, hStdoutWr, hStderr;
  SECURITY_ATTRIBUTES saAttr;
  DWORD childPid = 0;
  int fd;

  saAttr.nLength = sizeof(saAttr);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;
  hStderr = GetStdHandle(STD_ERROR_HANDLE);
  if( !CreatePipe(&hStdoutRd, &hStdoutWr, &saAttr, 4096) ){
    win32_fatal_error("cannot create pipe for stdout");
  }
  SetHandleInformation( hStdoutRd, HANDLE_FLAG_INHERIT, FALSE);

  if( !CreatePipe(&hStdinRd, &hStdinWr, &saAttr, 4096) ){
    win32_fatal_error("cannot create pipe for stdin");
  }
  SetHandleInformation( hStdinWr, HANDLE_FLAG_INHERIT, FALSE);

  win32_create_child_process(win32_utf8_to_unicode(zCmd),
                             hStdinRd, hStdoutWr, hStderr,&childPid);
  *pChildPid = childPid;
  fd = _open_osfhandle(PTR_TO_INT(hStdoutRd), 0);
  *ppIn = fdopen(fd, "rb");
  fd = _open_osfhandle(PTR_TO_INT(hStdinWr), 0);
  *ppOut = _fdopen(fd, "wb");
  CloseHandle(hStdinRd);
  CloseHandle(hStdoutWr);
  return 0;
#else
  int pin[2], pout[2];
  *ppIn = 0;
  *ppOut = 0;
  *pChildPid = 0;

  if( pipe(pin)<0 ){
    return 1;
  }
  if( pipe(pout)<0 ){
    close(pin[0]);
    close(pin[1]);
    return 1;
  }
  *pChildPid = fork();
  if( *pChildPid<0 ){
    close(pin[0]);
    close(pin[1]);
    close(pout[0]);
    close(pout[1]);
    *pChildPid = 0;
    return 1;
  }
  signal(SIGPIPE,SIG_IGN);
  if( *pChildPid==0 ){
    int fd;
    /* This is the child process */
    close(0);
    fd = dup(pout[0]);
    if( fd!=0 ) {
      fprintf(stderr,"popen2() failed to open file descriptor 0");
      exit(1);
    }
    close(pout[0]);
    close(pout[1]);
    close(1);
    fd = dup(pin[1]);
    if( fd!=1 ){
      fprintf(stderr,"popen() failed to open file descriptor 1");
      exit(1);
    }
    close(pin[0]);
    close(pin[1]);
    if( bDirect ){
      execl(zCmd, zCmd, (char*)0);
    }else{
      execl("/bin/sh", "/bin/sh", "-c", zCmd, (char*)0);
    }
    return 1;
  }else{
    /* This is the parent process */
    close(pin[1]);
    *ppIn = fdopen(pin[0], "r");
    close(pout[0]);
    *ppOut = fdopen(pout[1], "w");
    return 0;
  }
#endif
}
/* …（同文件无关代码省略）… */
**
**    0      Ordinary character.  No encoding required
**    1      Needs to be escaped
**    2      Illegal character.  Do not allow in a filename
**    3      First byte of a 2-byte UTF-8
**    4      First byte of a 3-byte UTF-8
**    5      First byte of a 4-byte UTF-8
/* …（同文件无关代码省略）… */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 1x */
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0, /* 2x */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  1,  1, /* 3x */
     1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 4x */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  0, /* 5x */
     1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 6x */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  0,  1, /* 7x */
    /* all bytes 0x80 through 0xbf are unescaped, being secondary
    ** bytes to UTF8 characters.  Bytes 0xc0 through 0xff are the
    ** first byte of a UTF8 character and do get escaped */
/* …（同文件无关代码省略）… */

/*
** Return true if the input contains special characters that need
** to be escaped.
*/
int need_escape(const char *zIn){
  size_t i;
/* …（同文件无关代码省略）… */
** appends filename argument zIn.
**
** The argument is escaped if it contains white space or other characters
** that need to be escaped for the shell.  If zIn contains characters
** that cannot be safely escaped, then throw a fatal error.
**
** If the isFilename argument is 1 or 2, then the argument is expected
** to be a filename.  isFilename==1 means to use shell-style escaping
/* …（同文件无关代码省略）… */
void append_escaped_arg(sqlite3_str *pStr, const char *zIn, int isFilename){
  int n = sqlite3_str_length(pStr);
  char *z = sqlite3_str_value(pStr);
  size_t nIn = strlen(zIn);

  /* Separate from the previous argument by a space */
  if( n>0 && !isspace(z[n-1]) ){
    sqlite3_str_appendchar(pStr, 1, ' ');
  }

  /* Check for characters that need quoting */
  if( !need_escape(zIn) ){
    if( (isFilename>=1 && zIn[0]=='-')
     || (isFilename==2 && (nIn&1)==0 && strspn(zIn,"ABCDEFGHIJKLMNOP")==nIn)
    ){
      sqlite3_str_appendchar(pStr, 1, '.');
      sqlite3_str_appendchar(pStr, 1, '/');
    }
    sqlite3_str_appendall(pStr, zIn);
  }else if( isFilename==2 ){
    /* Encode as hexadecimal using A-P as the hex digits. */
    size_t j;
    for(j=0; j<nIn; j++){
      sqlite3_str_appendchar(pStr, 1, (((zIn[j])>>4)&0xf)+'A');
      sqlite3_str_appendchar(pStr, 1, ((zIn[j])&0xf)+'A');
    }
    nHexEncode++;
  }else{
    /* Use shell escapes */
#if defined(_WIN32)
    /* Quoting strategy for windows:
    ** Put the entire name inside of "...".  Any " characters within
    ** the name get doubled.
    */
    size_t i;
    unsigned char c;
    sqlite3_str_appendchar(pStr, 1, '"');
    if( isFilename && zIn[0]=='-' ){
      sqlite3_str_appendchar(pStr, 1, '.');
      sqlite3_str_appendchar(pStr, 1, '\\');
    }else if( zIn[0]=='/' ){
      sqlite3_str_appendchar(pStr, 1, '.');
    }
    for(i=0; (c = (unsigned char)zIn[i])!=0; i++){
      sqlite3_str_appendchar(pStr, 1, (char)c);
      if( c=='"' ) sqlite3_str_appendchar(pStr, 1, '"');
      if( c=='\\' ) sqlite3_str_appendchar(pStr, 1, '\\');
      if( c=='%' && isFilename ) sqlite3_str_append(pStr, "%cd:~,%", 7);
    }
    sqlite3_str_appendchar(pStr, 1, '"');
#else
    /* Quoting strategy for unix:
    ** If the name does not contain ', then surround the whole thing
    ** with '...'.   If there is one or more ' characters within the
    ** name, then put \ before each special character.
    */
    if( strchr(zIn,'\'') ){
      unsigned char c;
      size_t i;
      if( isFilename && zIn[0]=='-' ){
        sqlite3_str_appendchar(pStr, 1, '.');
        sqlite3_str_appendchar(pStr, 1, '/');
      }
      for(i=0; (c = (unsigned char)zIn[i])!=0; i++){
        if( aSafeChar[c] && aSafeChar[c]!=2 ){
          sqlite3_str_appendchar(pStr, 1, '\\');
        }
        sqlite3_str_appendchar(pStr, 1, (char)c);
      }
    }else{
      sqlite3_str_appendchar(pStr, 1, '\'');
      if( isFilename && zIn[0]=='-' ){
        sqlite3_str_appendchar(pStr, 1, '.');
        sqlite3_str_appendchar(pStr, 1, '/');
      }
      sqlite3_str_appendall(pStr, zIn);
      sqlite3_str_appendchar(pStr, 1, '\'');
    }
#endif
  }
}
/* …（同文件无关代码省略）… */
void add_path_argument(sqlite3_str *pStr){
  append_escaped_arg(pStr,
     "PATH=$HOME/bin:/usr/local/bin:/opt/homebrew/bin"
     ":/opt/local/bin:$PATH", 0);
}
/* …（同文件无关代码省略）… */
static char *decode_filename(const char *zIn){
  size_t n;
  if( zIn==0 ) return 0;
  n = strlen(zIn);
  if( (n&1)==1 || strspn(zIn,"ABCDEFGHIJKLMNOP")!=n ){
    /* Name is literal.  No decoding required. */
    return sqlite3_mprintf("%s", zIn);
  }else{
    /* Name is hex encoded.  Need to decode it. */
    char *zOut = sqlite3_malloc64(n/2 + 1);
    size_t i;
    if( zOut==0 ){
      fprintf(stderr, "out of memory\n");
      exit(1);
    }
    for(i=0; i<n/2; i++){
      zOut[i] = (zIn[i*2]-'A')*16 + zIn[i*2+1]-'A';
    }
    zOut[i] =  0;
    return zOut;
  }
}
/* …（同文件无关代码省略）… */
static const char *cmdline_option_value(int argc, const char * const*argv,
                                        int i){
  if( i==argc ){
    fprintf(stderr,"%s: Error: missing argument to %s\n",
            argv[0], argv[argc-1]);
    exit(1);
  }
  return argv[i];
}
/* …（同文件无关代码省略）… */
**
**  (2)    sqlite3_rsync  USER@HOST:FILENAME1  FILENAME2
**
**  (3)    sqlite3_rsync --origin FILENAME1
**
**  (4)    sqlite3_rsync --replica FILENAME2
**
** The user types (1) or (2).  SSH launches (3) or (4).
**
/* …（同文件无关代码省略）… */
  const char *zRemoteErrFile = 0;
  const char *zRemoteDebugFile = 0;
  int nEncoded = 0;

#define cli_opt_val cmdline_option_value(argc, argv, ++i)
  memset(&ctx, 0, sizeof(ctx));
/* …（同文件无关代码省略）… */
      ctx.bCommCheck = 1;
      continue;
    }
    if( strcmp(z,"-arg-escape-check")==0 ){  /* DEBUG ONLY */
      /* Test the append_escaped_arg() routine by using it to render a
      ** copy of the input command-line, assuming all arguments except
/* …（同文件无关代码省略）… */
      ctx.zErrFile = decode_filename(ctx.zErrFile);
      ctx.zDebugFile = decode_filename(ctx.zDebugFile);
    }
  }else{
    /* Must be the local side */
    if( nHexEncode ){
/* …（同文件无关代码省略）… */
      sqlite3_str *pStr = sqlite3_str_new(0);
      append_escaped_arg(pStr, zSsh, 1);
      if( iPort>0 ) sqlite3_str_appendf(pStr, " -p %d", iPort);
      sqlite3_str_appendf(pStr, " -e none");
      append_escaped_arg(pStr, ctx.zOrigin, 1);
      if( iRetry ) add_path_argument(pStr);
      append_escaped_arg(pStr, zExe, 1);
/* …（同文件无关代码省略）… */
        append_escaped_arg(pStr, "2>/dev/null", 0);
      }
      zCmd = sqlite3_str_finish(pStr);
      if( ctx.eVerbose>=2 ) printf("%s\n", zCmd);
      if( popen2(zCmd, &ctx.pIn, &ctx.pOut, &childPid, 0) ){
        if( iRetry>=1 ){
/* …（同文件无关代码省略）… */
      sqlite3_str *pStr = sqlite3_str_new(0);
      append_escaped_arg(pStr, zSsh, 1);
      if( iPort>0 ) sqlite3_str_appendf(pStr, " -p %d", iPort);
      sqlite3_str_appendf(pStr, " -e none");
      append_escaped_arg(pStr, ctx.zReplica, 1);
      if( iRetry==1 ) add_path_argument(pStr);
      append_escaped_arg(pStr, zExe, 1);
/* …（同文件无关代码省略）… */
        append_escaped_arg(pStr, "2>/dev/null", 0);
      }
      zCmd = sqlite3_str_finish(pStr);
      if( ctx.eVerbose>=2 ) printf("%s\n", zCmd);
      if( popen2(zCmd, &ctx.pIn, &ctx.pOut, &childPid, 0) ){
        if( iRetry>=1 ){
/* …（同文件无关代码省略）… */
      sqlite3_str_appendf(pStr, " --encoded-filenames %d", nHexEncode);
    }
    zCmd = sqlite3_str_finish(pStr);
    if( ctx.eVerbose>=2 ) printf("%s\n", zCmd);
    if( popen2(zCmd, &ctx.pIn, &ctx.pOut, &childPid, 0) ){
      fprintf(stderr, "Could not start auxiliary process: %s\n", zCmd);
