// AUTO-DRAFT from curl/curl PR #5585
else {
        result = FindWin32CACert(config, tls_backend_info->backend,
                                 "curl-ca-bundle.crt");
#if defined(USE_WIN32_CRYPTO)
        if(!config->cacert && !config->capath) {  // <<< BUG ANCHOR
          /* user, and environment did not specify any ca file or path
             and there is no "curl-ca-bundle.crt" file in standard path
             so the only possible solution is using the windows ca store */
          config->native_ca_store = TRUE;
        }
#endif
      }
#endif
    }
