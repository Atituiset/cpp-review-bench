// AUTO-DRAFT from curl/curl PR #1549
/* reset all times except redirect, and reset the known transfer sizes */
void Curl_pgrsResetTimesSizes(struct Curl_easy *data)
{
  data->progress.t_nslookup = 0.0;  // <<< BUG ANCHOR
  data->progress.t_connect = 0.0;
  data->progress.t_pretransfer = 0.0;
  data->progress.t_starttransfer = 0.0;

  Curl_pgrsSetDownloadSize(data, -1);
  Curl_pgrsSetUploadSize(data, -1);
void Curl_pgrsTime(struct Curl_easy *data, timerid timer)
{
  struct timeval now = Curl_tvnow();

  switch(timer) {
  default:
    /* This is set at the start of each single fetch */
    data->progress.t_startsingle = now;
    break;

  case TIMER_STARTACCEPT:
    data->progress.t_acceptdata = Curl_tvnow();
    break;

  case TIMER_NAMELOOKUP:
    data->progress.t_nslookup =
      Curl_tvdiff_secs(now, data->progress.t_startsingle);
    break;
  case TIMER_CONNECT:
    data->progress.t_connect =
      Curl_tvdiff_secs(now, data->progress.t_startsingle);
    break;
  case TIMER_APPCONNECT:
    data->progress.t_appconnect =
      Curl_tvdiff_secs(now, data->progress.t_startsingle);
    break;
  case TIMER_PRETRANSFER:
    data->progress.t_pretransfer =
      Curl_tvdiff_secs(now, data->progress.t_startsingle);
    break;
  case TIMER_STARTTRANSFER:
    data->progress.t_starttransfer =
      Curl_tvdiff_secs(now, data->progress.t_startsingle);
    break;
  case TIMER_POSTRANSFER:
    /* this is the normal end-of-transfer thing */
    break;
  case TIMER_REDIRECT:
    data->progress.t_redirect = Curl_tvdiff_secs(now, data->progress.start);
    break;
  }
}

void Curl_pgrsStartNow(struct Curl_easy *data)
  now = Curl_tvnow(); /* what time is it */

  /* The time spent so far (from the start) */
  data->progress.timespent = curlx_tvdiff_secs(now, data->progress.start);
  timespent = (curl_off_t)data->progress.timespent;

  /* The average download speed this far */
  data->progress.dlspeed = (curl_off_t)
    ((double)data->progress.downloaded/
     (data->progress.timespent>0?data->progress.timespent:1));

  /* The average uplo
