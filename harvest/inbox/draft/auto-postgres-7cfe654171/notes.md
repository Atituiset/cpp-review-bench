# auto-postgres-7cfe654171

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#dd568b047acc7cf7c13bcb397506ba850eb5a9e7](https://github.com/postgres/postgres/commit/dd568b047acc7cf7c13bcb397506ba850eb5a9e7) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 128 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #dd568b047acc7cf7c13bcb397506ba850eb5a9e7 (https://github.com/postgres/postgres/commit/dd568b047acc7cf7c13bcb397506ba850eb5a9e7)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 776；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT dd568b047acc7cf7c13bcb397506ba850eb5a9e7 findtimezone: update Windows timezone name list for recent events. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -722,14 +722,8 @@ static const struct
 
 {
 	/*
-	 * This list was built from the contents of the registry at
-	 * HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Time
-	 * Zones on Windows 7, Windows 10, and Windows Server 2019.  Some recent
-	 * additions have been made by comparing to the CLDR project's
-	 * windowsZones.xml file.
-	 *
-	 * The zones have been matched to IANA timezones based on CLDR's mapping
-	 * for "territory 001".
+	 * The contents of this array are mostly generated from the CLDR project's
+	 * windowsZones.xml file.  See src/timezone/README for more information.
 	 */
 	{
 		/* (UTC+04:30) Kabul */
@@ -741,6 +735,11 @@ static const struct
 		"Alaskan Standard Time", "Alaskan Daylight Time",
 		"America/Anchorage"
 	},
+	{
+		/* (UTC-07:00) Alberta */
+		"Alberta Standard Time", "Alberta Daylight Time",
+		"America/Edmonton"
+	},
 	{
 		/* (UTC-10:00) Aleutian Islands */
 		"Aleutian Standard Time", "Aleutian Daylight Time",
@@ -771,11 +770,6 @@ static const struct
 		"Argentina Standard Time", "Argentina Daylight Time",
 		"America/Buenos_Aires"
 	},
-	{
-		/* (UTC+04:00) Baku, Tbilisi, Yerevan */
-		"Armenian Standard Time", "Armenian Daylight Time",
-		"Asia/Yerevan"
-	},
 	{
 		/* (UTC+04:00) Astrakhan, Ulyanovsk */
 		"Astrakhan Standard Time", "Astrakhan Daylight Time",
@@ -832,17 +826,17 @@ static const struct
 		"Pacific/Bougainville"
 	},
 	{
-		/* (UTC-01:00) Cabo Verde Is. */
-		"Cabo Verde Standard Time", "Cabo Verde Daylight Time",
-		"Atlantic/Cape_Verde"
+		/* (UTC-07:00) British Columbia */
+		"British Columbia Standard Time", "British Columbia Daylight Time",
+		"America/Vancouver"
 	},
 	{
 		/* (UTC-06:00) Saskatchewan */
 		"Canada Central Standard Time", "Canada Central Daylight Time",
 		"America/Regina"
 	},
 	{
-		/* (UTC-01:00) Cape Verde Is. */
+		/* (UTC-01:00) Cabo Verde Is. */
 		"Cape Verde Standard Time", "Cape Verde Daylight Time",
 		"Atlantic/Cape_Verde"
 	},
@@ -864,7 +858,7 @@ static const struct
 	{
 		/* (UTC+06:00) Astana */
 		"Central Asia Standard Time", "Central Asia Daylight Time",
-		"Asia/Almaty"
+		"Asia/Bishkek"
 	},
 	{
 		/* (UTC-04:00) Cuiaba */
@@ -906,11 +900,6 @@ static const struct
 		"China Standard Time", "China Daylight Time",
 		"Asia/Shanghai"
 	},
-	{
-		/* (UTC) Coordinated Universal Time */
-		"Coordinated Universal Time", "Coordinated Universal Time",
-		"UTC"
-	},
 	{
 		/* (UTC-05:00) Havana */
 		"Cuba Standard Time", "Cuba Daylight Time",
@@ -992,13 +981,6 @@ static const struct
 		"America/Godthab"
 	},
 	{
-		/*
-		 * Windows uses this zone name in various places that lie near the
-		 * prime meridian, but are not in the UK.  However, most people
-		 * probably think that "Greenwich" means UK civil time, or maybe even
-		 * straight-up UTC.  Atlantic/Reykjavik is a decent match for that
-		 * interpretation because Iceland hasn't observed DST since 1968.
-		 */
 		/* (UTC+00:00) Monrovia, Reykjavik */
 		"Greenwich Standard Time", "Greenwich Daylight Time",
 		"Atlantic/Reykjavik"
@@ -1033,11 +1015,6 @@ static const struct
 		"Israel Standard Time", "Israel Daylight Time",
 		"Asia/Jerusalem"
 	},
-	{
-		/* (UTC+02:00) Jerusalem (old spelling of zone name) */
-		"Jerusalem Standard Time", "Jerusalem Daylight Time",
-		"Asia/Jerusalem"
-	},
 	{
 		/* (UTC+02:00) Amman */
 		"Jordan Standard Time", "Jordan Daylight Time",
@@ -1048,11 +1025,6 @@ static const struct
 		"Kaliningrad Standard Time", "Kaliningrad Daylight Time",
 		"Europe/Kaliningrad"
 	},
-	{
-		/* (UTC+12:00) Petropavlovsk-Kamchatsky - Old */
-		"Kamchatka Standard Time", "Kamchatka Daylight Time",
-		"Asia/Kamchatka"
-	},
 	{
 		/* (UTC+09:00) Seoul */
 		"Korea Standard Time", "Korea Daylight Time",
@@ -1084,9 +1056,9 @@ static const struct
 		"America/Punta_Arenas"
 	},
 	{
-		/* (UTC+08:00) Kuala Lumpur, Singapore */
-		"Malay Peninsula Standard Time", "Malay Peninsula Daylight Time",
-		"Asia/Kuala_Lumpur"
+		/* (UTC-05:00) Manitoba */
+		"Manitoba
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`CLDR`
- 大写宏：`DST`
- 大写宏：`HKEY_LOCAL_MACHINE`
- 大写宏：`IANA`
- 大写宏：`RTZ`
- 大写宏：`SOFTWARE`
- 大写宏：`UTC`
- 外部类型：`Alaskan`
- 外部类型：`Aleutian`
- 外部类型：`Almaty`
- 外部类型：`America`
- 外部类型：`Amman`
- 外部类型：`Ana`
- 外部类型：`Anchorage`
- 外部类型：`Argentina`
- 外部类型：`Armenian`
- 外部类型：`Asia`
- 外部类型：`Astana`
- 外部类型：`Astrakhan`
- 外部类型：`Atlantic`
- 外部类型：`Baku`
- 外部类型：`Beirut`
- 外部类型：`Bougainville`
- 外部类型：`Buenos_Aires`
- 外部类型：`Cabo`
- 外部类型：`Canada`
- 外部类型：`Cape`
- 外部类型：`Cape_Verde`
- 外部类型：`Central`
- 外部类型：`Chihuahua`
- 外部类型：`China`
- 外部类型：`Chokurdakh`
- 外部类型：`City`
- 外部类型：`Coordinated`
- 外部类型：`Cuba`
- 外部类型：`Cuiaba`
- 外部类型：`CurrentVersion`
- 外部类型：`Daylight`
- 外部类型：`East`
- 外部类型：`Ekaterinburg`
- 外部类型：`Europe`
- 外部类型：`Godthab`
- 外部类型：`Greenwich`
- 外部类型：`Guadalajara`
- 外部类型：`Havana`
- 外部类型：`However`
- 外部类型：`Iceland`
- 外部类型：`Indian`
- 外部类型：`Irkutsk`
- 外部类型：`Is`
- 外部类型：`Islands`
- 外部类型：`Israel`
- 外部类型：`Izhevsk`
- 外部类型：`Jerusalem`
- 外部类型：`Jordan`
- 外部类型：`Kabul`
- 外部类型：`Kaliningrad`
- 外部类型：`Kamchatka`
- 外部类型：`Kamchatsky`
- 外部类型：`Korea`
- 外部类型：`Krasnoyarsk`
- 外部类型：`Kuala`
- 外部类型：`Kuala_Lumpur`
- 外部类型：`La`
- 外部类型：`Lumpur`
- 外部类型：`Magadan`
- 外部类型：`Malay`
- 外部类型：`Marquesas`
- 外部类型：`Mauritius`
- 外部类型：`Mazatlan`
- 外部类型：`Mexico`
- 外部类型：`Mexico_City`
- 外部类型：`Microsoft`
- 外部类型：`Mid`
- 外部类型：`Middle`
- 外部类型：`Monrovia`
- 外部类型：`Monterrey`
- 外部类型：`Moscow`
- 外部类型：`Mountain`
- 外部类型：`North`
- 外部类型：`Novosibirsk`
- 外部类型：`Old`
- 外部类型：`Omsk`
- 外部类型：`Pacific`
- 外部类型：`Paris`
- 外部类型：`Paz`
- 外部类型：`Peninsula`
- 外部类型：`Petersburg`
- 外部类型：`Petropavlovsk`
- 外部类型：`Punta_Arenas`
- 外部类型：`Pyongyang`
- 外部类型：`Rangoon`
- 外部类型：`Regina`
- 外部类型：`Reykjavik`
- 外部类型：`Romance`
- 外部类型：`Russia`
- 外部类型：`Samara`
- 外部类型：`Saskatchewan`
- 外部类型：`Seoul`
- 外部类型：`Server`
- 外部类型：`Shanghai`
- 外部类型：`Singapore`
- 外部类型：`Some`
- 外部类型：`South_Georgia`
- 外部类型：`St`
- 外部类型：`Standard`
- 外部类型：`Tbilisi`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Time`
- 外部类型：`Ulyanovsk`
- 外部类型：`Universal`
- 外部类型：`Verde`
- 外部类型：`Vladivostok`
- 外部类型：`Windows`
- 外部类型：`Yakutsk`
- 外部类型：`Yekaterinburg`
- 外部类型：`Yerevan`
- 外部类型：`Zone`
- 外部类型：`Zones`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-7cfe654171` → 本草稿移入 `cases/defect/auto-postgres-7cfe654171/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
