// AUTO-DRAFT from curl/curl PR #15289
struct curl_slist *contents;
};

static size_t write_memory_callback(void *contents, size_t size,
  size_t nmemb, void *userp) {
    /* append the data to contents */
    size_t realsize = size * nmemb;
    struct Ctx *mem = (struct Ctx *)userp;
    char *data = (char *)malloc(realsize + 1);
    struct curl_slist *item_append = NULL;
    if(!data) {
      printf("not enough memory (malloc returned NULL)\n");
      return 0;
    }
    memcpy(data, contents, realsize);
    data[realsize] = '\0';
    item_append = curl_slist_append(mem->contents, data);
    free(data);
    if(item_append) {
      mem->contents = item_append;
    }
    else {
      printf("not enough memory (curl_slist_append returned NULL)\n");
      return 0;
    }
    return realsize;
}

static
