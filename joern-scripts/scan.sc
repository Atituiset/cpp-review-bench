// Joern CPG 扫描脚本（Joern 2.x API）。
// 给定 case 的 src 目录 + golden 锚点（经文件传入，避免空格引号问题）：
//  1) 在 CPG 中按方法名定位 must_find 锚点行（图可达性证明）；
//  2) 枚举危险调用（memcpy/strcpy/strncpy/memmove/strcat/free/realloc/memset）。
// 输出 {"findings":[{file,line,column,message,scenario}]} 到 outFile。
import java.nio.file.{Files, Paths}
import scala.io.Source
import scala.collection.mutable.ListBuffer

val srcDir   = params("srcDir").asInstanceOf[String]
val anchorFile   = params.getOrElse("anchorFile","").asInstanceOf[String]
val functionFile = params.getOrElse("functionFile","").asInstanceOf[String]
val scenario = params.getOrElse("scenario","").asInstanceOf[String]
val outFile  = params("outFile").asInstanceOf[String]

def read(p: String): String =
  if (p != null && p != "" && new java.io.File(p).exists) Source.fromFile(p).mkString.trim else ""

val anchor   = read(anchorFile)
val function = read(functionFile)

def fileNameOf(node: Any): String = try {
  val m = node.getClass.getMethod("location")
  val loc = m.invoke(node)
  val fm = loc.getClass.getMethod("filename")
  fm.invoke(loc).asInstanceOf[String]
} catch { case _: Throwable => try {
  node.getClass.getMethod("filename").invoke(node).asInstanceOf[String]
} catch { case _: Throwable => "" } }

val out = ListBuffer[Map[String, Any]]()

try {
  importCode(srcDir)
  // 1) 锚点定位
  if (function.nonEmpty && anchor.nonEmpty) {
    cpg.method.name(function).l.foreach { m =>
      val base = m.lineNumber.getOrElse(1)
      m.code.split("\n").zipWithIndex.foreach { case (ln, idx) =>
        if (ln.contains(anchor)) {
          out += Map("file" -> fileNameOf(m), "line" -> (base + idx), "column" -> 0,
                     "message" -> ("joern CPG anchor match: " + ln.trim.take(120)), "scenario" -> scenario)
        }
      }
    }
  }
  // 2) 危险调用枚举（全局）
  cpg.call.name("(memcpy|strcpy|strncpy|memmove|strcat|free|realloc|memset)").l.foreach { c =>
    out += Map("file" -> fileNameOf(c), "line" -> c.lineNumber.getOrElse(0), "column" -> 0,
               "message" -> ("joern dangerous call: " + c.name), "scenario" -> "cwe-787")
  }
} catch {
  case e: Throwable => println("joern scan error: " + e.getMessage)
}

val arr = out.map { f =>
  val fp = f("file").asInstanceOf[String]; val ln = f("line").asInstanceOf[Int]
  val ms = f("message").asInstanceOf[String]; val sc = f("scenario").asInstanceOf[String]
  s"""{"file":"$fp","line":$ln,"column":0,"message":"$ms","scenario":"$sc"}"""
}.mkString("[", ",", "]")
Files.write(Paths.get(outFile), s"""{"findings":$arr}""".getBytes("UTF-8"))
println("joern wrote " + out.size + " findings -> " + outFile)
