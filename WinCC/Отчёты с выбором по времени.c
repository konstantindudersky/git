#define DEBUG FALSE

char date_begin[32], date_end[32];
int day, month, year;
int report_number;
LPCTSTR report_name;

// считываю время
day = GetPropWord(lpszPictureName, "DTP_SelectDate", "Day");
month = GetPropWord(lpszPictureName, "DTP_SelectDate", "Month");
year = GetPropWord(lpszPictureName, "DTP_SelectDate", "Year");

// конструирую строки времени
sprintf(date_begin, "%02d.%02d.%04d", day, month, year);
sprintf(date_end, "%02d.%02d.%04d 23:59:59", day, month, year);

if (DEBUG){
	printf("date_begin: %s \r\n", date_begin);
	printf("date_end: %s \r\n", date_end);
}

// записываю строки времени в теги
SetTagChar("2203AZ_Report_BeginTime", date_begin);
SetTagChar("2203AZ_Report_EndTime", date_end);

// печатаем отчёт
report_number = GetPropWord(lpszPictureName, "Cmb_SelectReport", "SelIndex");
switch (report_number) {
	case 1: // подача сырья на печь
		report_name = "DayReport_KilnFeeding";
		break;
	case 2: // расход угля на печь №3
		report_name = "DayReport_CoalConsumptionKiln3";
		break;
	default:
		if (DEBUG) printf("Оператор switch - не выбрано ни одного значения\r\n");
		return;
}

if (GetPropBOOL(lpszPictureName, "Chk_Preview", "Process"))
	RPTJobPreview(report_name);
else
	RPTJobPrint(report_name); 
