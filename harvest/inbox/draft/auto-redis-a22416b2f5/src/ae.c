// AUTO-DRAFT from redis/redis PR #15391
te = te->next;
    }

    monotime now = getMonotonicUs();
    return (now >= earliest->when) ? 0 : earliest->when - now;
}
