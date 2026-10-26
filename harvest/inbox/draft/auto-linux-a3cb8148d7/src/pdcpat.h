// AUTO-DRAFT from torvalds/linux PR #6b4b543fa18757297ac756a6fe6e64f506b7844a
#define PAT_GMMIO           2         /* > 4G MMIO Space */
#define PAT_NPIOP           3         /* Non Postable I/O Port Space */
#define PAT_PIOP            4         /* Postable I/O Port Space */
#define PAT_AHPA            5         /* Addional HPA Space */
#define PAT_UFO             6         /* HPA Space (UFO for Mariposa) */
#define PAT_GNIP            7         /* GNI Reserved Space */

/* …（同文件无关代码省略）… */
* ----------------------------------------------------------
* Bit  0 to 51 - conf_base_addr
* Bit 52 to 62 - reserved
* Bit       63 - endianess bit
********************************************************************/
#define PAT_GET_CBA(value) ((value) & 0xfffffffffffff000UL)
