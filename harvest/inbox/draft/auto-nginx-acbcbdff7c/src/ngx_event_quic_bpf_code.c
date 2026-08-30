// AUTO-DRAFT from nginx/nginx PR #215
static ngx_bpf_reloc_t bpf_reloc_prog_ngx_quic_reuseport_helper[] = {
    { "ngx_quic_sockmap", 55 },
};

static struct bpf_insn bpf_insn_prog_ngx_quic_reuseport_helper[] = {
    /* opcode dst          src         offset imm */
    { 0x79,   BPF_REG_4,   BPF_REG_1, (int16_t)      0,        0x0 },
    { 0x79,   BPF_REG_3,   BPF_REG_1, (int16_t)      8,        0x0 },
    { 0xbf,   BPF_REG_2,   BPF_REG_4, (int16_t)      0,        0x0 },
    {  0x7,   BPF_REG_2,   BPF_REG_0, (int16_t)      0,        0x8 },
    { 0x2d,   BPF_REG_2,   BPF_REG_3, (int16_t)     54,        0x0 },
    { 0xbf,   BPF_REG_5,   BPF_REG_4, (int16_t)      0,        0x0 },
    {  0x7,   BPF_REG_5,   BPF_REG_0, (int16_t)      0,        0x9 },
    { 0x2d,   BPF_REG_5,   BPF_REG_3, (int16_t)     51,        0x0 },
    { 0xb7,   BPF_REG_5,   BPF_REG_0, (int16_t)      0,       0x14 },
    { 0xb7,   BPF_REG_0,   BPF_REG_0, (int16_t)      0,        0x9 },
    { 0x71,   BPF_REG_6,   BPF_REG_2, (int16_t)      0,        0x0 },
    { 0x67,   BPF_REG_6,   BPF_REG_0, (int16_t)      0,       0x38 },
    { 0xc7,   BPF_REG_6,   BPF_REG_0, (int16_t)      0,       0x38 },
    { 0x65,   BPF_REG_6,   BPF_REG_0, (int16_t)     10, 0xffffffff },
    { 0xbf,   BPF_REG_2,   BPF_REG_4, (int16_t)      0,        0x0 },
    {  0x7,   BPF_REG_2,   BPF_REG_0, (int16_t)      0,        0xd },
    { 0x2d,   BPF_REG_2,   BPF_REG_3, (int16_t)     42,        0x0 },
    { 0xbf,   BPF_REG_5,   BPF_REG_4, (int16_t)      0,        0x0 },
    {  0x7,   BPF_REG_5,   BPF_REG_0, (int16_t)      0,        0xe },
    { 0x2d,   BPF_REG_5,   BPF_REG_3, (int16_t)     39,        0x0 },
    { 0xb7,   BPF_REG_0,   BPF_REG_0, (int16_t)      0,        0xe },
    { 0x71,   BPF_REG_5,   BPF_REG_2, (int16_t)      0,        0x0 },
    { 0xb7,   BPF_REG_6,   BPF_REG_0, (int16_t)      0,        0x8 },
    { 0x2d,   BPF_REG_6,   BPF_REG_5, (int16_t)     35,        0x0 },
    {  0xf,   BPF_REG_5,   BPF_REG_0, (int16_t)      0,        0x0 },
    {  0xf,   BPF_REG_
