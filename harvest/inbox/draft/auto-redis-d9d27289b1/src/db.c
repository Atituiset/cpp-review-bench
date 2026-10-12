// AUTO-DRAFT from redis/redis PR #15408
}

            first += spec->fk.keynum.firstkey;
            last = first + ((long)numkeys - 1) * step;
        } else {
            /* unknown spec */
