/*
 * Copyright 2011-2012 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platform/profiler/Profiler.h"

#include "io/fs/FilePath.h"
#include "io/fs/FileStream.h"

#include <iomanip>
#include <map>
#include <vector>

#define BOOST_DATE_TIME_NO_LIB
#include <boost/date_time.hpp>

Profiler& Profiler::instance() {
	static Profiler instance;
	return instance;
}

Profiler::Profiler() {
	writeIndex = 0;
	canWrite = true;
	memset(profilePoints, 0, sizeof(profilePoints));
}

void Profiler::init() {
	registerThread("main");
}

void Profiler::shutdown() {
}

void Profiler::reset() {
	writeIndex = 0;
	memset(profilePoints, 0, sizeof(profilePoints));
	recordedVariables.clear();
}

void Profiler::registerThread(const std::string& threadName) {
	thread_id_type threadId = Thread::getCurrentThreadId();
	ThreadInfo& threadInfo = instance().threadsInfo[threadId];
	threadInfo.threadName = threadName;
	threadInfo.threadId = threadId;
	threadInfo.startTime = Time::getUs();
	threadInfo.endTime = threadInfo.startTime;
}
	
void Profiler::unregisterThread() {
	thread_id_type threadId = Thread::getCurrentThreadId();
	ThreadInfo& threadInfo = instance().threadsInfo[threadId];
	threadInfo.endTime = Time::getUs();
}

void Profiler::addProfilePoint(const char* tag, thread_id_type threadId, u64 startTime, u64 endTime) {

	while(!instance().canWrite);

	u32 pos = InterlockedIncrement(&instance().writeIndex) - 1;

	ProfilePoint& point = instance().profilePoints[pos % NB_POINTS];
	point.tag = tag;
	point.threadId = threadId;
	point.startTime = startTime;
	point.endTime = endTime;
}

void Profiler::flush() {

	canWrite = false;

	writeProfileLog();
	writeRecordedVariables();

	reset();

	canWrite = true;
}

void Profiler::recordVariable(const char* name, float value) {
	instance().recordedVariables[name].push_back(value);
}

void Profiler::writeProfileLog() {
		
	std::string filename = getDateTimeString() + ".perf";
	fs::ofstream out(fs::path(filename), std::ios::binary | std::ios::out);

	u32 numItems;

	// Threads info
	numItems = threadsInfo.size();
	out.write((const char*)&numItems, sizeof(numItems));
	for(std::map<thread_id_type, ThreadInfo>::const_iterator it = threadsInfo.begin(); it != threadsInfo.end(); ++it) {
		const ThreadInfo& threadInfo = it->second;
		numItems = threadInfo.threadName.length();
		out.write((const char*)&numItems, sizeof(numItems));
		out.write(threadInfo.threadName.c_str(), numItems);
		out.write((const char*)&threadInfo.threadId, sizeof(threadInfo.threadId));
		out.write((const char*)&threadInfo.startTime, sizeof(threadInfo.startTime));
		out.write((const char*)&threadInfo.endTime, sizeof(threadInfo.endTime));
	}

	// Profile points
	u32 index = 0;
	numItems = writeIndex;

	if(writeIndex >= NB_POINTS) {
		index = writeIndex;
		numItems = NB_POINTS;
	}

	out.write((const char*)&numItems, sizeof(numItems));
	for(u32 i = 0; i < numItems; ++i, ++index) {
		ProfilePoint& point = profilePoints[index % NB_POINTS];

		u32 len = strlen(point.tag);
		out.write((const char*)&len, sizeof(len));
		out.write((const char*)point.tag, len);
		out.write((const char*)&point.threadId, sizeof(point.threadId));
		out.write((const char*)&point.startTime, sizeof(point.startTime));
		out.write((const char*)&point.endTime, sizeof(point.endTime));
	}

	out.close();
}

void Profiler::writeRecordedVariables() {
		
	if(recordedVariables.empty())
		return;

	std::string filename = getDateTimeString() + ".csv";
	fs::ofstream out(fs::path(filename), std::ios::out);

	u32 maxNumEntries = 0;

	// Tell Excel that our separator is ';'
	out << "sep=;" << std::endl;

	// Write the header
	for(VariablesRecord::const_iterator it = recordedVariables.begin(); it != recordedVariables.end(); ++it) {
		out << it->first;
		out << ';';

		if(it->second.size() > maxNumEntries)
			maxNumEntries = it->second.size();
	}

	out << std::endl;

	// Write all entries
	for(u32 i = 0; i < maxNumEntries; i++) {

		for(VariablesRecord::const_iterator it = recordedVariables.begin(); it != recordedVariables.end(); ++it) {
				
			if(it->second.size() < i) {
				continue;
			}

			out << std::fixed << std::setprecision(5) << it->second[i];
			out << ';';
		}
		out << std::endl;
	}
}

std::string Profiler::getDateTimeString() const {
	boost::posix_time::ptime localTime = boost::posix_time::second_clock::local_time();
	boost::gregorian::date::ymd_type ymd = localTime.date().year_month_day();
	boost::posix_time::time_duration hms = localTime.time_of_day();

	std::stringstream localTimeString;
	localTimeString << std::setfill('0') 
		            << ymd.year << "." 
		            << std::setw(2)
		            << ymd.month.as_number() << "." 
		            << ymd.day.as_number() << "-" 
		            << hms.hours() << "." 
		            << hms.minutes() << "." 
		            << hms.seconds();

	return localTimeString.str();
}
