#include "apdefap.h"
void OnClick(char* lpszPictureName, char* lpszObjectName, char* lpszPropertyName)
{
#define NumberOfData 7
// порядок наименования газов в массивах:
// 0 - CO2
// 1 -  CO
// 2 - NOx
// 3 - NO
// 4 - NO2
// 5 - SO2
// 6 - запыленность


// подключаем функции для работы со временем
#pragma code ("oleaut32.dll")
INT SystemTimeToVariantTime(LPSYSTEMTIME lpSystemTime, double *pvtime);
INT VariantTimeToSystemTime(double vtime, LPSYSTEMTIME lpSystemTime);
#pragma code ()

// структуры для времени начальной и конечной дат
SYSTEMTIME TimeFrom, TimeTo;
// выбранные из WinCC день, месяц, год
WORD day, month, year;
// строки для представления начальной даты, конечной даты, обоих дат
char date_begin[20], date_end[20], date_range[60];
// для перевода из SystemTime в VariantTime и обратно
double date;
// массивы выбросов из архива, выбросов за простой, суммарных выбросов, средних выбросов, максимальных выбросов
double emisArchive[NumberOfData], emisStop[NumberOfData], emisTotal[NumberOfData], emisAvg[NumberOfData], emisMax[NumberOfData];
// массивы превышений и сумм превышений
double excess[10][NumberOfData], excessTotal[NumberOfData];
// время простоя
double timeBreak;
// процент превышений
double percent;
int i, j;
// выбранный тип отчета:
// 1 - выбросы за день
// 2 - выбросы за месяц
// 4 - выбросы за год
// 8 - превышения за день
// 16 - превышения за месяц
// 32 - превышения за год
long report_type;
// выбранное действие:
// 1 - показать
// 2 - печатать
long report_action;
BOOL fRet;
// массив данных из архива
PTLG_ARCHIVDATARAW ADR;
// количество данных из архива
DWORD Number;
// название архива
LPCTSTR lpszArchivName = "OS_SVR1::ProcessValueArchive";
// массив для имен тегов газов, время простоя
LPCTSTR lpszTagName[NumberOfData], lpszTagTimeBreak;
// массив тегов для суммарных выбросов
LPCTSTR lpszTagEmisArchive[NumberOfData], lpszTagEmisStop[NumberOfData], lpszTagEmisTotal[NumberOfData];
// двумерный массив для полей отчета по превышениям
LPCTSTR lpszExcess[10][NumberOfData], lpszExcessTotal[NumberOfData];
// имя отчета
LPCTSTR lpszReportName;


// деактивируем элементы
SetPropChar(lpszPictureName, "Report_Build", "Text", "Работаю...");
SetPropBOOL(lpszPictureName, "Report_Build", "Operation", 0);
SetPropBOOL(lpszPictureName, "2203AZ_Report_Type", "Operation", 0);
SetPropBOOL(lpszPictureName, "2203AZ_Report_Action", "Operation", 0);
SetPropBOOL(lpszPictureName, "2203AZ_Report_Day", "Operation", 0);
SetPropBOOL(lpszPictureName, "2203AZ_Report_Month", "Operation", 0);
SetPropBOOL(lpszPictureName, "2203AZ_Report_Year", "Operation", 0);

// скрываем столбцы Online Table Control
for (i = 0; i < 20; i++) {
	SetPropWord(lpszPictureName, "Table", "ValueColumnIndex", i);
	SetPropBOOL(lpszPictureName, "Table", "ValueColumnVisible", 0);
}

// считываем тип отчета
report_type = GetPropWord(lpszPictureName, "2203AZ_Report_Type", "Process");
report_action = GetPropWord(lpszPictureName, "2203AZ_Report_Action", "Process");

// считываем выбранную дату
day = GetTagWord("2203AZ_Report_Day");
month = GetTagWord("2203AZ_Report_Month");
year = GetTagWord("2203AZ_Report_Year");


// промежуток времени за день
if (report_type == 1 || report_type == 8) {
	// задаем начальную дату
	TimeFrom.wYear = year;
	TimeFrom.wMonth = month;
	TimeFrom.wDay = day;
	TimeFrom.wHour = 0;
	TimeFrom.wMinute = 0;
	TimeFrom.wSecond = 0;

	// прибавляем 1 день
	SystemTimeToVariantTime(&TimeFrom, &date);
	VariantTimeToSystemTime(date + 1, &TimeTo);
}


// промежуток времени за месяц
if (report_type == 2 || report_type == 16) {
	// задаем начальную дату
	TimeFrom.wYear = year;
	TimeFrom.wMonth = month;
	TimeFrom.wDay = 1;		// если нужен апрель - надо ставить 23 число
	TimeFrom.wHour = 0;
	TimeFrom.wMinute = 0;
	TimeFrom.wSecond = 0;

	// задаем конечную дату
	if (month == 12) {
		TimeTo.wYear = (WORD) ((INT)TimeFrom.wYear + 1);
		TimeTo.wMonth = 1;
	}
	else {
		TimeTo.wYear = TimeFrom.wYear;
		TimeTo.wMonth = (WORD) ((INT)TimeFrom.wMonth + 1);
	}
	TimeTo.wDay = 1;
	TimeTo.wHour = 0;
	TimeTo.wMinute = 0;
	TimeTo.wSecond = 0;
}


// отчет по выбросам за день
if (report_type == 1){
	if (report_action == 1) {
		// формат времени
		SetPropChar(lpszPictureName, "Table", "TimeColumnTimeFormat", "HH ч - ");
		// видимость даты
		SetPropBOOL(lpszPictureName, "Table", "TimeColumnShowDate", 0);
		// устанавливаем видимость необходимых столбцов
		for (i = 0; i < 7; i++) {
			SetPropWord(lpszPictureName, "Table", "ValueColumnIndex", i);
			SetPropBOOL(lpszPictureName, "Table", "ValueColumnVisible", 1);
		}
	}

	// массив имен архивов
	lpszTagName[0] = "2203AZ/CO2_emis_hour.V";
	lpszTagName[1] = "2203AZ/CO_emis_hour.V";
	lpszTagName[2] = "2203AZ/NOx_emis_hour.V";
	lpszTagName[3] = "2203AZ/NO_emis_hour.V";
	lpszTagName[4] = "2203AZ/NO2_emis_hour.V";
	lpszTagName[5] = "2203AZ/SO2_emis_hour.V";
	lpszTagName[6] = "2203AZ/dust_emis_hour.V";

	// время простоя
	lpszTagTimeBreak = "2203AZ/TimeBreak_Hour.V";

	// название отчета
	lpszReportName = "2203AZ_EmissionsDay";
}


// отчет по выбросам за месяц
if (report_type == 2){
	if (report_action == 1) {
		// задаем настройки элемента Online Trend Control
		// видимость даты
		SetPropBOOL(lpszPictureName, "Table", "TimeColumnShowDate", 1);
		// формат даты
		SetPropChar(lpszPictureName, "Table", "TimeColumnDateFormat", "dd.MM");
		// формат времени
		SetPropChar(lpszPictureName, "Table", "TimeColumnTimeFormat", "-");
		// устанавливаем видимость необходимых столбцов
		for (i = 7; i < 14; i++) {
			SetPropWord(lpszPictureName, "Table", "ValueColumnIndex", i);
			SetPropBOOL(lpszPictureName, "Table", "ValueColumnVisible", 1);
		}
	}

	// массив имен архивов
	lpszTagName[0] = "2203AZ/CO2_emis_day.V";
	lpszTagName[1] = "2203AZ/CO_emis_day.V";
	lpszTagName[2] = "2203AZ/NOx_emis_day.V";
	lpszTagName[3] = "2203AZ/NO_emis_day.V";
	lpszTagName[4] = "2203AZ/NO2_emis_day.V";
	lpszTagName[5] = "2203AZ/SO2_emis_day.V";
	lpszTagName[6] = "2203AZ/dust_emis_day.V";

	// время простоя
	lpszTagTimeBreak = "2203AZ/TimeBreak_Day.V";

	// название отчета
	lpszReportName = "2203AZ_EmissionsMonth";
}


// отчет по выбросам за год
if (report_type == 4){
	if (report_action == 1) {
		// задаем настройки элемента Online Trend Control
		// видимость даты
		SetPropBOOL(lpszPictureName, "Table", "TimeColumnShowDate", 1);
		// формат даты
		SetPropChar(lpszPictureName, "Table", "TimeColumnDateFormat", "MM");
		// формат времени
		SetPropChar(lpszPictureName, "Table", "TimeColumnTimeFormat", "-");
		// устанавливаем видимость необходимых столбцов
		for (i = 14; i < 19; i++) {
			SetPropWord(lpszPictureName, "Table", "ValueColumnIndex", i);
			SetPropBOOL(lpszPictureName, "Table", "ValueColumnVisible", 1);
		}
	}

	// массив имен архивов
	lpszTagName[0] = "2203AZ/CO2_emis_month.V";
	lpszTagName[1] = "2203AZ/CO_emis_month.V";
	lpszTagName[2] = "2203AZ/NOx_emis_month.V";
	lpszTagName[3] = "2203AZ/NO_emis_month.V";
	lpszTagName[4] = "2203AZ/NO2_emis_month.V";
	lpszTagName[5] = "2203AZ/SO2_emis_month.V";
	lpszTagName[6] = "2203AZ/SO2_emis_month.V";

	// время простоя
	lpszTagTimeBreak = "2203AZ/TimeBreak_Hour.V";
}


// отчет по выбросам (общая часть)
if (report_type == 1 || report_type == 2 || report_type == 4) {

	// массив тегов суммарных выбросов из архива
	lpszTagEmisArchive[0] = "2203AZ_Report_CO2";
	lpszTagEmisArchive[1] = "2203AZ_Report_CO";
	lpszTagEmisArchive[2] = "2203AZ_Report_NOx";
	lpszTagEmisArchive[3] = "2203AZ_Report_NO";
	lpszTagEmisArchive[4] = "2203AZ_Report_NO2";
	lpszTagEmisArchive[5] = "2203AZ_Report_SO2";
	lpszTagEmisArchive[6] = "2203AZ_Report_Dust";

	// массив тегов выбросов за простой
	lpszTagEmisStop[0] = "2203AZ_Report_CO2_Stop";
	lpszTagEmisStop[1] = "2203AZ_Report_CO_Stop";
	lpszTagEmisStop[2] = "2203AZ_Report_NOx_Stop";
	lpszTagEmisStop[3] = "2203AZ_Report_NO_Stop";
	lpszTagEmisStop[4] = "2203AZ_Report_NO2_Stop";
	lpszTagEmisStop[5] = "2203AZ_Report_SO2_Stop";
	lpszTagEmisStop[6] = "2203AZ_Report_Dust_Stop";
	
	// массив тегов суммарных выбросов
	lpszTagEmisTotal[0] = "2203AZ_Report_CO2_Sum";
	lpszTagEmisTotal[1] = "2203AZ_Report_CO_Sum";
	lpszTagEmisTotal[2] = "2203AZ_Report_NOx_Sum";
	lpszTagEmisTotal[3] = "2203AZ_Report_NO_Sum";
	lpszTagEmisTotal[4] = "2203AZ_Report_NO2_Sum";
	lpszTagEmisTotal[5] = "2203AZ_Report_SO2_Sum";
	lpszTagEmisTotal[6] = "2203AZ_Report_Dust_Sum";
	
	// массив значений средних выбросов
	emisAvg[0] = GetTagFloat("2203AZ/CO2_emis_avg.V");
	emisAvg[1] = GetTagFloat("2203AZ/CO_emis_avg.V");
	emisAvg[2] = GetTagFloat("2203AZ/NOx_emis_avg.V");
	emisAvg[3] = GetTagFloat("2203AZ/NO_emis_avg.V");
	emisAvg[4] = GetTagFloat("2203AZ/NO2_emis_avg.V");
	emisAvg[5] = GetTagFloat("2203AZ/SO2_emis_avg.V");
	emisAvg[6] = GetTagFloat("2203AZ/dust_emis_avg.V");

	// обнулить итоговые поля
	for (i = 0; i < NumberOfData; i++) {
		SetTagDouble(lpszTagEmisArchive[i], -1);
		SetTagDouble(lpszTagEmisStop[i], -1);
		SetTagDouble(lpszTagEmisTotal[i], -1);
	}

	// собираем строки
	sprintf(date_begin, "%02d.%02d.%04d 00:00:00", TimeFrom.wDay, TimeFrom.wMonth, TimeFrom.wYear);
	sprintf(date_end, "%02d.%02d.%04d 00:00:00", TimeTo.wDay, TimeTo.wMonth, TimeTo.wYear);
	sprintf(date_range, "(%s - %s)", date_begin, date_end);

	SetTagChar("2203AZ_Report_BeginTime", date_begin);
	SetTagChar("2203AZ_Report_EndTime", date_end);
	SetTagChar("2203AZ_Report_TimeRange", date_range);

	// Online Table Control
	if (report_action == 1) {
		SetPropChar(lpszPictureName, "Table", "TimeColumnBeginTime", date_begin);
		SetPropChar(lpszPictureName, "Table", "TimeColumnEndTime", date_end);
		SetPropChar(lpszPictureName, "2203AZ_Report_TimeRange_Emissions", "Text", date_range);
	}

	// пробуем подключаться к Tag Logging
	fRet = TLGConnect(NULL, NULL);
	if (fRet == FALSE) {
		MessageBox(NULL, "Нет соединения с Tag Logging", "Ошибка", MB_ICONWARNING | MB_OK | MB_SYSTEMMODAL);
		goto end;
	}

	// считываем выбросы по каждому газу
	for (i = 0; i < NumberOfData; i++) {
		Number = 0;
		if (TLGGetArchivDataEx(lpszArchivName, lpszTagName[i], &TimeFrom, &TimeTo, &ADR, &Number, NULL, NULL) == TRUE) {
			if (ADR != NULL) {
				emisArchive[i] = 0;
				for (j = 0; j < Number; j++) emisArchive[i] += ADR[j].doValue;
			}
			TLGFreeMemory(ADR);
			SetTagDouble(lpszTagEmisArchive[i], emisArchive[i] / 1000);
		}
		else {
			MessageBox(NULL, "Нет данных", "Ошибка", MB_ICONWARNING | MB_OK);
			goto end;
		}
	}

	// считываем время простоя
	Number = 0;
	if (TLGGetArchivDataEx(lpszArchivName, lpszTagTimeBreak, &TimeFrom, &TimeTo, &ADR, &Number, NULL, NULL) == TRUE) {
		if (ADR != NULL) {
			timeBreak = 0;
			for (j = 0; j < Number; j++)  timeBreak += ADR[j].doValue;
		}
		TLGFreeMemory(ADR);
		SetTagDouble("2203AZ_Report_TimeBreak", timeBreak / 60);
	}
	else {
		MessageBox(NULL, "Нет данных", "Ошибка", MB_ICONWARNING | MB_OK);
		goto end;
	}

	// рассчитываем выбросы за простой и суммарные выбросы
	for (i = 0; i < NumberOfData; i++) {
		emisStop[i] = timeBreak * emisAvg[i] / 3600;
		emisTotal[i] = emisStop[i] + emisArchive[i];
		SetTagDouble(lpszTagEmisStop[i], emisStop[i] / 1000);
		SetTagDouble(lpszTagEmisTotal[i], emisTotal[i] / 1000);
	}

	// отключаемся от Tag Logging
	TLGDisconnect(NULL);


	// печатаем отчет
	if (report_action == 2) {
		RPTJobPreview(lpszReportName);
	}
}

// отчет по превышениям (общая часть)
if (report_type == 8 || report_type == 16){
	// массив полей для вывода отчета
	lpszExcess[0][0] = "2203AZ_Report_Excess_5_CO2";
	lpszExcess[1][0] = "2203AZ_Report_Excess_10_CO2";
	lpszExcess[2][0] = "2203AZ_Report_Excess_15_CO2";
	lpszExcess[3][0] = "2203AZ_Report_Excess_20_CO2";
	lpszExcess[4][0] = "2203AZ_Report_Excess_25_CO2";
	lpszExcess[5][0] = "2203AZ_Report_Excess_30_CO2";
	lpszExcess[6][0] = "2203AZ_Report_Excess_35_CO2";
	lpszExcess[7][0] = "2203AZ_Report_Excess_40_CO2";
	lpszExcess[8][0] = "2203AZ_Report_Excess_45_CO2";
	lpszExcess[9][0] = "2203AZ_Report_Excess_50_CO2";
	lpszExcess[0][1] = "2203AZ_Report_Excess_5_CO";
	lpszExcess[1][1] = "2203AZ_Report_Excess_10_CO";
	lpszExcess[2][1] = "2203AZ_Report_Excess_15_CO";
	lpszExcess[3][1] = "2203AZ_Report_Excess_20_CO";
	lpszExcess[4][1] = "2203AZ_Report_Excess_25_CO";
	lpszExcess[5][1] = "2203AZ_Report_Excess_30_CO";
	lpszExcess[6][1] = "2203AZ_Report_Excess_35_CO";
	lpszExcess[7][1] = "2203AZ_Report_Excess_40_CO";
	lpszExcess[8][1] = "2203AZ_Report_Excess_45_CO";
	lpszExcess[9][1] = "2203AZ_Report_Excess_50_CO";
	lpszExcess[0][2] = "2203AZ_Report_Excess_5_NOx";
	lpszExcess[1][2] = "2203AZ_Report_Excess_10_NOx";
	lpszExcess[2][2] = "2203AZ_Report_Excess_15_NOx";
	lpszExcess[3][2] = "2203AZ_Report_Excess_20_NOx";
	lpszExcess[4][2] = "2203AZ_Report_Excess_25_NOx";
	lpszExcess[5][2] = "2203AZ_Report_Excess_30_NOx";
	lpszExcess[6][2] = "2203AZ_Report_Excess_35_NOx";
	lpszExcess[7][2] = "2203AZ_Report_Excess_40_NOx";
	lpszExcess[8][2] = "2203AZ_Report_Excess_45_NOx";
	lpszExcess[9][2] = "2203AZ_Report_Excess_50_NOx";
	lpszExcess[0][3] = "2203AZ_Report_Excess_5_NO";
	lpszExcess[1][3] = "2203AZ_Report_Excess_10_NO";
	lpszExcess[2][3] = "2203AZ_Report_Excess_15_NO";
	lpszExcess[3][3] = "2203AZ_Report_Excess_20_NO";
	lpszExcess[4][3] = "2203AZ_Report_Excess_25_NO";
	lpszExcess[5][3] = "2203AZ_Report_Excess_30_NO";
	lpszExcess[6][3] = "2203AZ_Report_Excess_35_NO";
	lpszExcess[7][3] = "2203AZ_Report_Excess_40_NO";
	lpszExcess[8][3] = "2203AZ_Report_Excess_45_NO";
	lpszExcess[9][3] = "2203AZ_Report_Excess_50_NO";
	lpszExcess[0][4] = "2203AZ_Report_Excess_5_NO2";
	lpszExcess[1][4] = "2203AZ_Report_Excess_10_NO2";
	lpszExcess[2][4] = "2203AZ_Report_Excess_15_NO2";
	lpszExcess[3][4] = "2203AZ_Report_Excess_20_NO2";
	lpszExcess[4][4] = "2203AZ_Report_Excess_25_NO2";
	lpszExcess[5][4] = "2203AZ_Report_Excess_30_NO2";
	lpszExcess[6][4] = "2203AZ_Report_Excess_35_NO2";
	lpszExcess[7][4] = "2203AZ_Report_Excess_40_NO2";
	lpszExcess[8][4] = "2203AZ_Report_Excess_45_NO2";
	lpszExcess[9][4] = "2203AZ_Report_Excess_50_NO2";
	lpszExcess[0][5] = "2203AZ_Report_Excess_5_SO2";
	lpszExcess[1][5] = "2203AZ_Report_Excess_10_SO2";
	lpszExcess[2][5] = "2203AZ_Report_Excess_15_SO2";
	lpszExcess[3][5] = "2203AZ_Report_Excess_20_SO2";
	lpszExcess[4][5] = "2203AZ_Report_Excess_25_SO2";
	lpszExcess[5][5] = "2203AZ_Report_Excess_30_SO2";
	lpszExcess[6][5] = "2203AZ_Report_Excess_35_SO2";
	lpszExcess[7][5] = "2203AZ_Report_Excess_40_SO2";
	lpszExcess[8][5] = "2203AZ_Report_Excess_45_SO2";
	lpszExcess[9][5] = "2203AZ_Report_Excess_50_SO2";
	
	// массив полей для вывода суммы
	lpszExcessTotal[0] = "2203AZ_Report_Excess_Total_CO2";
	lpszExcessTotal[1] = "2203AZ_Report_Excess_Total_CO";
	lpszExcessTotal[2] = "2203AZ_Report_Excess_Total_NOx";
	lpszExcessTotal[3] = "2203AZ_Report_Excess_Total_NO";
	lpszExcessTotal[4] = "2203AZ_Report_Excess_Total_NO2";
	lpszExcessTotal[5] = "2203AZ_Report_Excess_Total_SO2";

	// обнулим поля
	for (j = 0; j < 6; j++) {
		for (i = 0; i < 10; i++) SetPropDouble(lpszPictureName, lpszExcess[i][j], "OutputValue", 0);
		SetPropDouble(lpszPictureName, lpszExcessTotal[j], "OutputValue", 0);
	}

	// собираем строки
	sprintf(date_begin, "%02d.%02d.%04d 00:00:00", TimeFrom.wDay, TimeFrom.wMonth, TimeFrom.wYear);
	sprintf(date_end, "%02d.%02d.%04d 00:00:00", TimeTo.wDay, TimeTo.wMonth, TimeTo.wYear);
	sprintf(date_range, "(%s - %s)", date_begin, date_end);

	SetPropChar(lpszPictureName, "2203AZ_Report_TimeRange_Excess", "Text", date_range);

	//массив имен архивов
	lpszTagName[0] = "2203AZ/CO2_emis_current.V";
	lpszTagName[1] = "2203AZ/CO_emis_current.V";
	lpszTagName[2] = "2203AZ/NOx_emis_current.V";
	lpszTagName[3] = "2203AZ/NO_emis_current.V";
	lpszTagName[4] = "2203AZ/NO2_emis_current.V";
	lpszTagName[5] = "2203AZ/SO2_emis_current.V";

	// массив максимальных значений
	emisMax[0] = GetTagFloat("2203AZ/CO2_emis_max.V");
	emisMax[1] = GetTagFloat("2203AZ/CO_emis_max.V");
	emisMax[2] = GetTagFloat("2203AZ/NOx_emis_max.V");
	emisMax[3] = GetTagFloat("2203AZ/NO_emis_max.V");
	emisMax[4] = GetTagFloat("2203AZ/NO2_emis_max.V");
	emisMax[5] = GetTagFloat("2203AZ/SO2_emis_max.V");

	
	// пробуем подключаться к Tag Logging
	fRet = TLGConnect(NULL, NULL);
	if (fRet == FALSE) {
		MessageBox(NULL, "Нет соединения с Tag Logging", "Ошибка", MB_ICONWARNING | MB_OK | MB_SYSTEMMODAL);
		goto end;
	}
	
	// подсчитываем проценты превышений по каждому газу
	for (j = 0; j < 6; j++) {
		Number = 0;
		if (TLGGetArchivDataEx(lpszArchivName, lpszTagName[j], &TimeFrom, &TimeTo, &ADR, &Number, NULL, NULL) == TRUE) {
			if (ADR != NULL) {
				// обнуляем значения
				for (i = 0; i < 10; i ++) excess[i][j] = 0;
				// проходим по массиву значений
				for (i = 0; i < Number; i++){
					percent = (ADR[i].doValue - emisMax[j]) / emisMax[j] * 100;
					if (percent < 5) continue;
					else if (percent >=5 && percent < 10) excess[0][j] += 1;
					else if (percent >= 10 && percent < 15) excess[1][j] += 1;
					else if (percent >= 15 && percent < 20) excess[2][j] += 1;
					else if (percent >= 20 && percent < 25) excess[3][j] += 1;
					else if (percent >= 25 && percent < 30) excess[4][j] += 1;
					else if (percent >= 30 && percent < 35) excess[5][j] += 1;
					else if (percent >= 35 && percent < 40) excess[6][j] += 1;
					else if (percent >= 40 && percent < 45) excess[7][j] += 1;
					else if (percent >= 45 && percent < 50) excess[8][j] += 1;
					else excess[9][j] += 1;
				}
				emisTotal[j] = 0;
				for (i = 0; i < 10; i++) {
					// вычисляем проценты
					excess[i][j] = excess[i][j] / Number * 100;
					SetPropDouble(lpszPictureName, lpszExcess[i][j], "OutputValue", excess[i][j]);
					// вычисляем сумму
					excessTotal[j] += excess[i][j];
				}
				SetPropDouble(lpszPictureName, lpszExcessTotal[j], "OutputValue", excessTotal[j]);
			}
			TLGFreeMemory(ADR);
		}
		else {
			MessageBox(NULL, "Нет данных", "Ошибка", MB_ICONWARNING | MB_OK);
			goto end;
		}
	}

	// отключаемся от Tag Logging
	TLGDisconnect(NULL);
}


//метка завершения
end:

// активируем элементы
SetPropChar(lpszPictureName, "Report_Build", "Text", "OK");
SetPropBOOL(lpszPictureName, "Report_Build", "Operation", 1);
SetPropBOOL(lpszPictureName, "2203AZ_Report_Type", "Operation", 1);
SetPropBOOL(lpszPictureName, "2203AZ_Report_Action", "Operation", 1);
SetPropBOOL(lpszPictureName, "2203AZ_Report_Day", "Operation", 1);
SetPropBOOL(lpszPictureName, "2203AZ_Report_Month", "Operation", 1);
SetPropBOOL(lpszPictureName, "2203AZ_Report_Year", "Operation", 1);
}
