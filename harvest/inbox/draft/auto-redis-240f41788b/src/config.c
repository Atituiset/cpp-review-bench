// AUTO-DRAFT from redis/redis PR #15357
int moduleSetNumericConfig(client *c, sds name, long long val, const char **err) {
    standardConfig *config = getMutableConfig(c, name, err);
    if (config->type != NUMERIC_CONFIG) return 0;

    sds old_value = config->interface.get(config);
