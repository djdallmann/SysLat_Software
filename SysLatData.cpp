#include "stdafx.h"
#include "SysLatData.h"
#include<json/json.h>
#include<fstream>

CSysLatData::CSysLatData() {
	//sld = { 0 }; //this does not 0 out the values in the "m_allResults" array...also, I can't initialize a struct this way when it contains a vector??
	m_Mutex = CreateMutex(NULL, FALSE, NULL);

	m_startTime = time(0);
	ctime_s(m_startDate, sizeof(m_startDate), &m_startTime);

}

int CSysLatData::GetCounter() {
	return sld.m_counter;
}
int	CSysLatData::GetTotal() {
	return sld.m_systemLatencyTotal;
}
double CSysLatData::GetAverage() {
	return sld.m_systemLatencyAverage;
}
int	CSysLatData::GetCounterEVR() {
	return sld.m_counterEVR;
}
int	CSysLatData::GetTotalEVR() {
	return sld.m_systemLatencyTotalEVR;
}
double CSysLatData::GetAverageEVR() {
	return sld.m_systemLatencyAverageEVR;
}

int CSysLatData::GetMedian() {
	return sld.m_a_MovingAverage[50]; //THIS ISN'T RIGHT YOU DUMMY - going to have to implement a new data structure if I want this value
}
int CSysLatData::GetMedianEVR() {
	return sld.m_a_MovingAverage[50]; //going to have to keep another array of movingAverageEVR
}
int CSysLatData::GetMax() {
	return sld.m_systemLatencyMax;
}
int CSysLatData::GetMin() {
	return sld.m_systemLatencyMin;
}
int CSysLatData::GetMaxEVR() {
	return sld.m_systemLatencyMaxEVR;
}
int CSysLatData::GetMinEVR() {
	return sld.m_systemLatencyMinEVR;
}
CString CSysLatData::GetStringResult() {
	return m_strSysLatResultsComplete;
}

void CSysLatData::SetEndTime() {
	time(&m_endTime);
	ctime_s(m_endDate, sizeof(m_endDate), &m_endTime);
}

void CSysLatData::UpdateSLD(unsigned int loopCounter, const CString& sysLatResults, std::string targetWindow, std::string activeWindow)
{
	BOOL success = AcquireSLDMutex();		// begin the sync access to fields
	if (!success)
		return;

	sld.m_counter = loopCounter+1;

	m_strSysLatResultsComplete = sysLatResults;

	int systemLatency = 0;
	if (!m_strSysLatResultsComplete.IsEmpty()) {
		systemLatency = StrToInt(m_strSysLatResultsComplete);
		sld.m_allResults.push_back(systemLatency);
		sld.m_v_strTargetWindow.push_back(targetWindow);
		sld.m_v_strActiveWindow.push_back(activeWindow);
		sld.m_systemLatencyTotal += systemLatency;
		sld.m_systemLatencyAverage = static_cast<double>(sld.m_systemLatencyTotal) / sld.m_counter; //when I try to cast one of these to a double, it appears to get the program out of sync and shoots the displayed syslat up quite a bit... - working now?

		if (systemLatency > 3 && systemLatency < 100 && targetWindow == activeWindow) {
			sld.m_counterEVR++;
			sld.m_systemLatencyTotalEVR += systemLatency;
			sld.m_systemLatencyAverageEVR = static_cast<double>(sld.m_systemLatencyTotalEVR) / sld.m_counterEVR;
		}
	}

	ReleaseSLDMutex();		// end the sync access to fields
}

void CSysLatData::CheckSLDMutex() {
	if (m_Mutex == NULL)
	{
		AppendError("Error: Failed to create mutex");
	}
}
BOOL CSysLatData::AcquireSLDMutex() {
	if (m_Mutex != NULL)
	{
		return WAIT_ABANDONED != WaitForSingleObject(m_Mutex, INFINITE);
	}

	return TRUE;
}
void CSysLatData::ReleaseSLDMutex() {
	if (m_Mutex != NULL)
	{
		ReleaseMutex(m_Mutex);
	}
}
void CSysLatData::CloseSLDMutex() {
	if (m_Mutex != NULL)
	{
		CloseHandle(m_Mutex);
		m_Mutex = NULL;
	}
}

void CSysLatData::AppendError(const CString& error)
{
	//AcquireSLDMutex();

	if (!m_strError.IsEmpty())
		m_strError.Append("\n");
	m_strError.Append(error);
	m_strError.Append("\n");

	//ReleaseSLDMutex();
}

//data IO functions?
void CSysLatData::ExportData(int testNumber) {
	Json::Value jsonSLD;
	Json::Value resultsSize(Json::arrayValue);
	resultsSize.append(sld.m_allResults.size());
	resultsSize.append(sld.m_v_strTargetWindow.size());
	resultsSize.append(sld.m_v_strActiveWindow.size());
	Json::Value resultsArray(Json::arrayValue);
	
	for (int i = 0; i < sld.m_allResults.size(); i++) {
		Json::Value subResultsArray(Json::arrayValue);
		subResultsArray.append(Json::Value(i));
		subResultsArray.append(Json::Value(sld.m_allResults[i]));
		subResultsArray.append(Json::Value(sld.m_v_strTargetWindow[i]));
		subResultsArray.append(Json::Value(sld.m_v_strActiveWindow[i]));
		resultsArray.append(subResultsArray);
	}

	
	/* //This commented out block of code would keep the 3 arrays of data found in the SYSLAT_DATA struct seperate in the JSON. They are currently formatted to be an array of arrays to make the data easier to read.
	Json::Value resultsArray(Json::arrayValue);
	for (int i = 0; i < sld.m_counter; i++ ) {
		resultsArray.append(Json::Value(sld.m_allResults[i]));
	}
	Json::Value targetArray(Json::arrayValue);
	for (int i = 0; i < sld.m_counter; i++) {
		targetArray.append(Json::Value(sld.m_a_strTargetWindow[i]));
	}
	Json::Value activeArray(Json::arrayValue);
	for (int i = 0; i < sld.m_counter; i++) {
		activeArray.append(Json::Value(sld.m_a_strActiveWindow[i]));
	}
	

	jsonSLD["SysLatData"]["targetWindow"] = targetArray;
	jsonSLD["SysLatData"]["activeWindow"] = activeArray;
	*/


	//Add elapsed time at some point
	//Apparently(and the documentation doesn't reveal this FYI), gmtime is a static object(???) so if I don't set it right before I output it, I get the wrong thing.
	struct tm* startTimeUTC = gmtime(&m_startTime);
	char* startDateUTC = asctime(startTimeUTC);
	jsonSLD["-MetaData"]["1StartTimeUTC"] = startDateUTC;
	struct tm* endTimeUTC = gmtime(&m_endTime);
	char *endDateUTC = asctime(endTimeUTC);
	jsonSLD["-MetaData"]["2EndTimeUTC"] = endDateUTC;
	jsonSLD["-MetaData"]["3StartTimeLocal"] = m_startDate;
	jsonSLD["-MetaData"]["4EndTimeLocal"] = m_endDate;

	jsonSLD["AggregateData"]["EVRCounter"] = sld.m_counterEVR;
	jsonSLD["AggregateData"]["EVRSystemLatencyTotal"] = sld.m_systemLatencyTotalEVR;
	jsonSLD["AggregateData"]["EVRsystemLatencyAverage"] = sld.m_systemLatencyAverageEVR;
	jsonSLD["AggregateData"]["SysLatTestCount"] = sld.m_counter;
	jsonSLD["AggregateData"]["SystemLatencyTotal"] = sld.m_systemLatencyTotal;
	jsonSLD["AggregateData"]["systemLatencyAverage"] = sld.m_systemLatencyAverage;

	jsonSLD["SysLatData"]["SysLatResultSize"] = resultsSize;
	jsonSLD["SysLatData"]["SysLatResults"] = resultsArray;

	std::ofstream exportData;
	exportData.open("sld_export" + std::to_string(testNumber) + ".log");

	if (exportData.is_open()) {
		exportData << jsonSLD;
		dataExported = true;
	}
	else {
		m_strError += "Unable to open file";
	}

	exportData.close();
}