// AUTO-DRAFT from nginx/nginx PR #1908
#define NGX_GEOIP_CITY_DMA_CODE        10
#define NGX_GEOIP_CITY_AREA_CODE       11
  // <<< BUG ANCHOR

typedef struct {
#if (NGX_HAVE_GEOIP)
/* …（同文件无关代码省略）… */

    case MMDB_DATA_TYPE_DOUBLE:
        *value = entry_data.double_value;
        return NGX_OK;

    case MMDB_DATA_TYPE_FLOAT:
        *value = entry_data.float_value;
        return NGX_OK;

    default:
        return NGX_DECLINED;
    }
}

