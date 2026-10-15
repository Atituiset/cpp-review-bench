// AUTO-DRAFT from curl/curl PR #7397
/* 64-bit lseek-like function unavailable */
#    define _lseeki64(hnd,ofs,whence) _lseek(hnd,ofs,whence)
#  else
#    define _lseeki64(hnd,ofs,whence) _lseek64(hnd,ofs,whence)
#  endif
#endif
