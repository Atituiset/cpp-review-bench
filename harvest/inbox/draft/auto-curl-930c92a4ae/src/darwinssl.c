// AUTO-DRAFT from curl/curl PR #2397
*out_cert_and_key = (SecIdentityRef) identity;
          break;
        }
        else if(itemID == SecCertificateGetTypeID()) {
          status = SecIdentityCreateWithCertificate(NULL,
                                                 (SecCertificateRef) item,
                                                 out_cert_and_key);
          break;
        }
      }
    }
