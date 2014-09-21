#include "apdefap.h"
void OnClick(char* lpszPictureName, char* lpszObjectName, char* lpszPropertyName)
{
#define DEBUG TRUE

char report_names[2][64] = {
	"DayReport_KilnFeeding",
	"DayReport_CoalConsumptionKiln3",
};
int shift_select;		// 1 - дневная смена; 2 - ночная смена
char date_begin[32], date_end[32];	// форматированные строки начала и конца отчёта
SYSTEMTIME TimeBeg, TimeEnd;

// Считываю время
TimeBeg.wDay = (WORD) GetPropWord(lpszPictureName, "Dtp_BeginDate", "Day");
TimeBeg.wMonth = (WORD) GetPropWord(lpszPictureName, "Dtp_BeginDate", "Month");
TimeBeg.wYear = (WORD) GetPropWord(lpszPictureName, "Dtp_BeginDate", "Year");

// Считываю тип смены
shift_select = GetPropWord(lpszPictureName, "Rbg_DayNight", "Process");
switch (shift_select) {
	case 1:
		TimeBeg.wHour = 8;
		TimeEnd = TimeBeg;
		TimeEnd.wHour = 20;
		break;
	case 2:
		TimeBeg.wHour = 20;
		DayAdd(&TimeBeg, &TimeEnd, 1);
		TimeEnd.wHour = 8;
		break;
	default:
		printf("Неверный выбор Rbg_DayNight");
		break;
}

// Конструирую строки времени
DateFormat(&TimeBeg, date_begin);
DateFormat(&TimeEnd, date_end);
SetTagChar("Report_BeginTime", date_begin);
SetTagChar("Report_EndTime", date_end);
if (DEBUG) {
	printf("date_begin: %s\r\n", date_begin);
	printf("date_end: %s\r\n", date_end);
}

// Печатаю отчёт
RPTJobPreview(report_names[GetPropWord(lpszPictureName, "Cmb_SelectReport", "SelIndex")- 1]);

}
