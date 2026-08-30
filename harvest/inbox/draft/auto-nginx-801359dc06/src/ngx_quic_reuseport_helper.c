// AUTO-DRAFT from nginx/nginx PR #215
* actual map object is created by the "bpf" system call,
 * all pointers to this variable are replaced by the bpf loader
 */
struct bpf_map_def SEC("maps") ngx_quic_sockmap;  // <<< BUG ANCHOR


SEC(PROGNAME)
