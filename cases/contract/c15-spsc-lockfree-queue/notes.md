# c15-spsc-lockfree-queue 用例说明（三段式）

## 1. 真实仓形态（这个 case 从哪来）
高性能网络/驱动常用 **SPSC 无锁环形队列**：生产者维护 tail、消费者维护 head，
各自单线程，无需互斥锁；共享指针以 `_Atomic` + acquire/release 内存序同步
（生产者 release 发布数据、消费者 acquire 读取，反向同理）。这是成熟正确模式。
工具看到「共享变量无互斥锁访问」容易误报 cwe-362 数据竞争——但指针访问均为
原子操作且内存序成对，C11 下无 data race。

## 2. 为什么契约安全 / 哪里是真缺陷
- **安全点**：`g_head`/`g_tail` 均为 `_Atomic uint8_t`，各自由一方写、对方以
  acquire 读；`spsc_dequeue` 读 `g_q[head % QSZ]`，模 QSZ 后恒在 [0,15] 界内，
  既无数据竞争也无越界——报 cwe-362 属误报。
- **真缺陷（混入）**：`spsc_enqueue` 以 `g_q[tail]` 直索引——tail 按 uint8_t
  模 256 推进，而槽位只有 QSZ=16。满判 `next == head` 只在两指针模 256 恰好
  差 1 时触发，挡不住 tail 越过 15：连续入队不消费时第 17 次 enqueue 写
  `g_q[16]`，越界写（cwe-787）。消费侧取模、生产侧未取模，两侧不对称正是
  缺陷所在。真实并发模式缺陷。

## 3. 各工具可能误判方式
- CSA / CppCheck / clang-tidy：可能把无互斥锁的共享访问误报 cwe-362（不识别
  原子量与内存序同步）；对 enqueue 越界可能因模 256 回绕不敏感漏报。
- cooddy：若做并发模式识别，应能不误报 SPSC 且查到 enqueue 边界——对照。
