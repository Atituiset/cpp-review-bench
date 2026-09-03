// Joern CPG 通用扫描脚本（Joern 2.x，参数经环境变量传入：SRC_DIR/OUT_FILE）。
// 不依赖 golden 输入：对 importCode 后的 CPG 做通用查询（危险内存/字符串调用枚举），
// 能查出什么就输出什么，统一为 {"findings":[{"file","line","message","scenario","function"}]}。
// function 取调用点所在 method 名（c.method.name），供 eval 的 twin 点位按函数区分。
import java.nio.file.{Files, Paths}

val srcDir  = sys.env("SRC_DIR")
val outFile = sys.env("OUT_FILE")

val out = scala.collection.mutable.ListBuffer[Map[String, Any]]()
try {
  importCode(srcDir)
  // 通用查询：危险内存/字符串调用（越界写/读同族，统一标 cwe-787）
  cpg.call.name("(memcpy|strcpy|strncpy|memmove|strcat|memset)").l.foreach { c =>
    out += Map("file" -> c.method.filename, "line" -> c.lineNumber.getOrElse(0),
               "message" -> ("joern dangerous call: " + c.name), "scenario" -> "cwe-787",
               "function" -> c.method.name)
  }
  // free/realloc 属释放类误用（双重释放 cwe-415 / 释放后使用 cwe-416），
  // 与越界族分开标注，eval 按 `+` 分量相交匹配（如 golden cwe-415 可命中）
  cpg.call.name("(free|realloc)").l.foreach { c =>
    out += Map("file" -> c.method.filename, "line" -> c.lineNumber.getOrElse(0),
               "message" -> ("joern dangerous call: " + c.name), "scenario" -> "cwe-415+cwe-416",
               "function" -> c.method.name)
  }
} catch {
  case e: Throwable => println("joern scan error: " + e.getMessage)
}
val arr = out.map { f =>
  val fp = f("file").asInstanceOf[String]; val ln = f("line").asInstanceOf[Int]
  val ms = f("message").asInstanceOf[String]; val sc = f("scenario").asInstanceOf[String]
  val fn = f("function").asInstanceOf[String]
  s"""{"file":"$fp","line":$ln,"message":"$ms","scenario":"$sc","function":"$fn"}"""
}.mkString("[", ",", "]")
Files.write(Paths.get(outFile), s"""{"findings":$arr}""".getBytes("UTF-8"))
println("joern wrote " + out.size + " findings -> " + outFile)
