// Joern CPG 扫描脚本（Joern 2.x，参数经环境变量传入：SRC_DIR/ANCHOR_FILE/FUNCTION_FILE/SCENARIO/OUT_FILE）。
import java.nio.file.{Files, Paths}

val srcDir   = sys.env("SRC_DIR")
val anchorFile   = sys.env.getOrElse("ANCHOR_FILE","")
val functionFile = sys.env.getOrElse("FUNCTION_FILE","")
val scenario = sys.env.getOrElse("SCENARIO","")
val outFile  = sys.env("OUT_FILE")

def readFile(p: String): String =
  if (p != null && p != "" && new java.io.File(p).exists)
    new String(Files.readAllBytes(Paths.get(p)), "UTF-8").trim else ""

val anchor   = readFile(anchorFile)
val function = readFile(functionFile)

val out = scala.collection.mutable.ListBuffer[Map[String, Any]]()
try {
  importCode(srcDir)
  if (function != null && function.nonEmpty && anchor != null && anchor.nonEmpty) {
    cpg.method.name(function).l.foreach { m =>
      val base = m.lineNumber.getOrElse(1)
      m.code.split("\n").zipWithIndex.foreach { case (ln, idx) =>
        if (ln.contains(anchor)) {
          out += Map("file" -> m.filename, "line" -> (base + idx), "column" -> 0,
                     "message" -> ("joern CPG anchor match: " + ln.trim.take(120)), "scenario" -> scenario)
        }
      }
    }
  }
  cpg.call.name("(memcpy|strcpy|strncpy|memmove|strcat|free|realloc|memset)").l.foreach { c =>
    out += Map("file" -> c.method.filename, "line" -> c.lineNumber.getOrElse(0), "column" -> 0,
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
