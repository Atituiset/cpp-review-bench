// AUTO-DRAFT from postgres/postgres PR #dd568b047acc7cf7c13bcb397506ba850eb5a9e7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR

{
	/*
	 * This list was built from the contents of the registry at
	 * HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Time
	 * Zones on Windows 7, Windows 10, and Windows Server 2019.  Some recent
	 * additions have been made by comparing to the CLDR project's
	 * windowsZones.xml file.
	 *
	 * The zones have been matched to IANA timezones based on CLDR's mapping
	 * for "territory 001".
	 */
	{
		/* (UTC+04:30) Kabul */
/* …（同文件无关代码省略）… */
		"Alaskan Standard Time", "Alaskan Daylight Time",
		"America/Anchorage"
	},
	{
		/* (UTC-10:00) Aleutian Islands */
		"Aleutian Standard Time", "Aleutian Daylight Time",
/* …（同文件无关代码省略）… */
		"Argentina Standard Time", "Argentina Daylight Time",
		"America/Buenos_Aires"
	},
	{
		/* (UTC+04:00) Baku, Tbilisi, Yerevan */
		"Armenian Standard Time", "Armenian Daylight Time",
		"Asia/Yerevan"
	},
	{
		/* (UTC+04:00) Astrakhan, Ulyanovsk */
		"Astrakhan Standard Time", "Astrakhan Daylight Time",
/* …（同文件无关代码省略）… */
		"Pacific/Bougainville"
	},
	{
		/* (UTC-01:00) Cabo Verde Is. */
		"Cabo Verde Standard Time", "Cabo Verde Daylight Time",
		"Atlantic/Cape_Verde"
	},
	{
		/* (UTC-06:00) Saskatchewan */
		"Canada Central Standard Time", "Canada Central Daylight Time",
		"America/Regina"
	},
	{
		/* (UTC-01:00) Cape Verde Is. */
		"Cape Verde Standard Time", "Cape Verde Daylight Time",
		"Atlantic/Cape_Verde"
	},
/* …（同文件无关代码省略）… */
	{
		/* (UTC+06:00) Astana */
		"Central Asia Standard Time", "Central Asia Daylight Time",
		"Asia/Almaty"
	},
	{
		/* (UTC-04:00) Cuiaba */
/* …（同文件无关代码省略）… */
		"China Standard Time", "China Daylight Time",
		"Asia/Shanghai"
	},
	{
		/* (UTC) Coordinated Universal Time */
		"Coordinated Universal Time", "Coordinated Universal Time",
		"UTC"
	},
	{
		/* (UTC-05:00) Havana */
		"Cuba Standard Time", "Cuba Daylight Time",
/* …（同文件无关代码省略）… */
		"America/Godthab"
	},
	{
		/*
		 * Windows uses this zone name in various places that lie near the
		 * prime meridian, but are not in the UK.  However, most people
		 * probably think that "Greenwich" means UK civil time, or maybe even
		 * straight-up UTC.  Atlantic/Reykjavik is a decent match for that
		 * interpretation because Iceland hasn't observed DST since 1968.
		 */
		/* (UTC+00:00) Monrovia, Reykjavik */
		"Greenwich Standard Time", "Greenwich Daylight Time",
		"Atlantic/Reykjavik"
/* …（同文件无关代码省略）… */
		"Israel Standard Time", "Israel Daylight Time",
		"Asia/Jerusalem"
	},
	{
		/* (UTC+02:00) Jerusalem (old spelling of zone name) */
		"Jerusalem Standard Time", "Jerusalem Daylight Time",
		"Asia/Jerusalem"
	},
	{
		/* (UTC+02:00) Amman */
		"Jordan Standard Time", "Jordan Daylight Time",
/* …（同文件无关代码省略）… */
		"Kaliningrad Standard Time", "Kaliningrad Daylight Time",
		"Europe/Kaliningrad"
	},
	{
		/* (UTC+12:00) Petropavlovsk-Kamchatsky - Old */
		"Kamchatka Standard Time", "Kamchatka Daylight Time",
		"Asia/Kamchatka"
	},
	{
		/* (UTC+09:00) Seoul */
		"Korea Standard Time", "Korea Daylight Time",
/* …（同文件无关代码省略）… */
		"America/Punta_Arenas"
	},
	{
		/* (UTC+08:00) Kuala Lumpur, Singapore */
		"Malay Peninsula Standard Time", "Malay Peninsula Daylight Time",
		"Asia/Kuala_Lumpur"
	},
	{
		/* (UTC-09:30) Marquesas Islands */
/* …（同文件无关代码省略）… */
		"Mauritius Standard Time", "Mauritius Daylight Time",
		"Indian/Mauritius"
	},
	{
		/* (UTC-06:00) Guadalajara, Mexico City, Monterrey */
		"Mexico Standard Time", "Mexico Daylight Time",
		"America/Mexico_City"
	},
	{
		/* (UTC-07:00) Chihuahua, La Paz, Mazatlan */
		"Mexico Standard Time 2", "Mexico Daylight Time 2",
		"America/Chihuahua"
	},
	{
		/* (UTC-02:00) Mid-Atlantic - Old */
		"Mid-Atlantic Standard Time", "Mid-Atlantic Daylight Time",
		"Atlantic/South_Georgia"
	},
	{
		/* (UTC+02:00) Beirut */
		"Middle East Standard Time", "Middle East Daylight Time",
/* …（同文件无关代码省略）… */
	{
		/* (UTC-07:00) Chihuahua, La Paz, Mazatlan */
		"Mountain Standard Time (Mexico)", "Mountain Daylight Time (Mexico)",
		"America/Chihuahua"
	},
	{
		/* (UTC+06:30) Yangon (Rangoon) */
/* …（同文件无关代码省略）… */
		"North Korea Standard Time", "North Korea Daylight Time",
		"Asia/Pyongyang"
	},
	{
		/* (UTC+07:00) Novosibirsk */
		"Novosibirsk Standard Time", "Novosibirsk Daylight Time",
		"Asia/Novosibirsk"
	},
	{
		/* (UTC+06:00) Omsk */
		"Omsk Standard Time", "Omsk Daylight Time",
/* …（同文件无关代码省略）… */
		"Romance Standard Time", "Romance Daylight Time",
		"Europe/Paris"
	},
	{
		/* (UTC+04:00) Izhevsk, Samara */
		"Russia Time Zone 3", "Russia Time Zone 3",
		"Europe/Samara"
	},
	{
		/* (UTC+11:00) Chokurdakh */
		"Russia Time Zone 10", "Russia Time Zone 10",
/* …（同文件无关代码省略）… */
		"Russia Time Zone 11", "Russia Time Zone 11",
		"Asia/Kamchatka"
	},
	{
		/* (UTC+02:00) Kaliningrad */
		"Russia TZ 1 Standard Time", "Russia TZ 1 Daylight Time",
		"Europe/Kaliningrad"
	},
	{
		/* (UTC+03:00) Moscow, St. Petersburg */
		"Russia TZ 2 Standard Time", "Russia TZ 2 Daylight Time",
		"Europe/Moscow"
	},
	{
		/* (UTC+04:00) Izhevsk, Samara */
		"Russia TZ 3 Standard Time", "Russia TZ 3 Daylight Time",
		"Europe/Samara"
	},
	{
		/* (UTC+05:00) Ekaterinburg */
		"Russia TZ 4 Standard Time", "Russia TZ 4 Daylight Time",
		"Asia/Yekaterinburg"
	},
	{
		/* (UTC+06:00) Novosibirsk (RTZ 5) */
		"Russia TZ 5 Standard Time", "Russia TZ 5 Daylight Time",
		"Asia/Novosibirsk"
	},
	{
		/* (UTC+07:00) Krasnoyarsk */
		"Russia TZ 6 Standard Time", "Russia TZ 6 Daylight Time",
		"Asia/Krasnoyarsk"
	},
	{
		/* (UTC+08:00) Irkutsk */
		"Russia TZ 7 Standard Time", "Russia TZ 7 Daylight Time",
		"Asia/Irkutsk"
	},
	{
		/* (UTC+09:00) Yakutsk */
		"Russia TZ 8 Standard Time", "Russia TZ 8 Daylight Time",
		"Asia/Yakutsk"
	},
	{
		/* (UTC+10:00) Vladivostok */
		"Russia TZ 9 Standard Time", "Russia TZ 9 Daylight Time",
		"Asia/Vladivostok"
	},
	{
		/* (UTC+11:00) Chokurdakh */
		"Russia TZ 10 Standard Time", "Russia TZ 10 Daylight Time",
		"Asia/Magadan"
	},
	{
		/* (UTC+12:00) Ana
