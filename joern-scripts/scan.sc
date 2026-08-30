// Joern CPG 扫描脚本：给定 case 的 src 目录 + golden 锚点（经文件传入，避免空格引号问题），
// 1) 在 CPG 中定位锚点所在方法内的匹配行（证明图可达性）；
// 2) 枚举危险调用（memcpy/strcpy/strncpy/memmove/strcat/free/realloc）。
// 输出 JSON（仅 findings 数组）到 outFile，由 joern_to_findings.py 包装为归一化文档。
import java.nio.file.{Files, Paths}
import scala.io.Source
import scala.collection.mutable.ListBuffer

val srcDir   = params("srcDir").asInstanceOf[String]
val anchorFile   = params.getOrElse("anchorFile","").asInstanceOf[String]
val functionFile = params.getOrElse("functionFile","").asInstanceOf[String]
val scenario = params.getOrElse("scenario","").asInstanceOf[String]
val outFile  = params("outFile").asInstanceOf[String]

def read(p: String): String = if (p != null && p != "" && new java.io.File(p).exists) Source.fromFile(p).mkString.trim else ""

val anchor   = read(anchorFile)
val function = read(functionFile)

val out = ListBuffer[Map[String, Any]]()

try {
  importCode(srcDir)
  val meths = if (function.nonEmpty) cpg.method.name(function).l else cpg.method.l
  meths.foreach { m =>
    val baseLine = m.lineNumber.getOrElse(1)
    if (anchor.nonEmpty) {
      m.code.split("\n").zipWithIndex.foreach { case (ln, idx) =>
        if (ln.contains(anchor)) {
          out += Map("file" -> m.filename, "line" -> (baseLine + idx), "column" -> 0,
                     "message" -> ("joern CPG anchor match: " + ln.trim.take(120)), "scenario" -> scenario)
        }
      }
    }
    m.call.name("(memcpy|strcpy|strncpy|memmove|strcat|free|realloc|memset)").foreach { c =>
      out += Map("file" -> c.filename, "line" -> c.lineNumber.getOrElse(0), "column" -> 0,
                 "message" -> ("joern dangerous call: " + c.name), "scenario" -> "cwe-787")
    }
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
