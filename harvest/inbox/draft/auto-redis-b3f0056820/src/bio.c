// AUTO-DRAFT from redis/redis PR #15364
}
}

void bioSubmitJob(int type, bio_job *job) {
    job->header.type = type;
    unsigned long worker = bio_job_to_worker[type];
