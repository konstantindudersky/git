int ExportArchiveToCSV2(SYSTEMTIME stStart, SYSTEMTIME stStop, char *ArchiveName, char *VarName, char *FileName)
{
/*
Функция экспортирует данные тега VarName из архива ArchiveName в текстовый файл с именем FileName.
Данные экспортируются за период stStart..stStop
Функция возвращает целое число, отображающее результат экспорта:
	0 - Экспорт прошел без ошибок
	2 - Данные не получены
	4 - Невозможно получить данные из архива
	5 - Невозможно открыть текстовый файл
*/
#pragma code("pdertcli.dll")
#include "pde_glob.h"
#include "pdertcli.h"
#pragma code()

BOOL retcode, FirstPass = TRUE;
CMN_ERROR error;
int i, return_code;
FILE *LogFile;
PTLG_ARCHIVDATARAW pDat;
PTLG_ARCHIVDATARAW g_pTlgData = NULL;
DWORD g_TLGdwNumberOfData = 10000, g_TLGdwFlags = 0;

// Открываем файл для записи
LogFile = fopen(FileName, "w");
if (LogFile == NULL) {
	printf("ExportArchiveToCSV: Невозможно открыть файл %s\r\n", FileName);
	return 5;
}
printf("ExportArchiveToCSV: файл %s успешно создан\r\n", FileName);


// Подключаемся к Tag Logging
if (TLGConnect(NULL, &error) == FALSE) {
	printf("ExportArchiveToCSV: Ошибка выполнения TLGConnect. %s\r\n", error.szErrorText);
	return_code = 4;
	goto end;
}

// В цикле вызывается функция TLGGetArchivDataEx. Эта функция возвращает макисмум 10000 значений за один вызов.
do {
	g_TLGdwNumberOfData = 10000;
	g_pTlgData = NULL;

	// Забираем данные с архива
	retcode = TLGGetArchivDataEx(ArchiveName, VarName, &stStart, &stStop, &g_pTlgData, &g_TLGdwNumberOfData, &g_TLGdwFlags, &error);
	
	// Если ошибка при получении данных
	if (retcode != TRUE) {
		printf("ExportArchiveToCSV: Ошибка выполнения TLGGetArchivDataEx. %s\r\n", error.szErrorText);
		// Очищаем память
		if (g_pTlgData != NULL) {
			retcode = TLGFreeMemory(g_pTlgData);
			if (retcode != TRUE) { printf("ExportArchiveToCSV: Ошибка выполнения TLGFreeMemory\r\n"); }
		}
		return_code = 4;
		goto end;
	}

	// Если данные не получены
	if (g_TLGdwNumberOfData <= 0) {
		printf("ExportArchiveToCSV: TLGGetArchivDataEx ничего не вернул\r\n");
		// Очищаем память
		if (g_pTlgData != NULL) {
			retcode = TLGFreeMemory(g_pTlgData);
			if (retcode != TRUE) { printf("ExportArchiveToCSV: Ошибка выполнения TLGFreeMemory\r\n"); }
		}
		if (FirstPass == FALSE) {
			// Данных больше нет, но какие-то данные получены в предыдущих циклах
			break;
		}
		else {
			// Первый вызов функции, но данных нет
			return_code = 2;
			goto end;
		}
	}

	// Инициализируем указатель для перемещения по данным
	pDat = g_pTlgData;
	printf("ExportArchiveToCSV: TLGGetArchivDataEx: Получено %d значений\r\n", g_TLGdwNumberOfData);
	for (i = 0; i < (int)g_TLGdwNumberOfData; i++) {
		fprintf(LogFile, "%04d.%02d.%02d %02d.%02d.%02d,%7.3f,%d\r\n", 
			pDat->stTime.wYear, pDat->stTime.wMonth, pDat->stTime.wDay, 
			pDat->stTime.wHour, pDat->stTime.wMinute, pDat->stTime.wSecond,
			pDat->doValue, pDat->dwFlags);
		pDat++;
	}

	// Если получено 10000 значений, возможно в архиве ещё есть данные
	if (g_TLGdwNumberOfData >= 10000) {
		FirstPass = FALSE;
		stStart = (pDat - 1)->stTime;
		printf("ExportArchiveToCSV: next step\r\n");
	}

	// Очищаем память
	if (g_pTlgData != NULL) {
		retcode = TLGFreeMemory(g_pTlgData);
		if (retcode != TRUE) { printf("ExportArchiveToCSV: Ошибка выполнения TLGFreeMemory\r\n"); }
	}
} while (g_TLGdwNumberOfData >= 10000);


end:
if (TLGDisconnect(&error) == FALSE)
	printf("ExportArchiveToCSV: Ошибка выполнения TLGDisonnect. %s", error.szErrorText);
// Очищаем память
if (g_pTlgData != NULL)
	if (TLGFreeMemory(g_pTlgData) != TRUE)
		printf("ExportArchiveToCSV: Ошибка выполнения TLGFreeMemory");
fclose(LogFile);
return return_code;
}
