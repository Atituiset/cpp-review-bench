// AUTO-DRAFT from curl/curl PR #7babac86904c7ac0b34a22240737c7261cb059eb
}
    }
  }
  if(exit_event) {  // <<< BUG ANCHOR
    if(CloseHandle(exit_event)) {
      exit_event = NULL;
    }
  }
#endif
#endif
}
