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

#ifndef ARX_PLATFORM_PROFILER_H
#define ARX_PLATFORM_PROFILER_H

#include "platform/Platform.h"
#include "platform/Thread.h"
#include "platform/Time.h"

#include <map>
#include <vector>

class Profiler {

public:
	static Profiler& instance();

	Profiler();

	static void init();
	static void shutdown();	

	static void registerThread(const std::string& threadName);
	static void unregisterThread();

	static void addProfilePoint(const char* tag, thread_id_type threadId, u64 startTime, u64 endTime);
	static void recordVariable(const char* name, float value);

	void reset();
	void flush();

private:
	static const u32 NB_POINTS = 4 * 1024;

	struct ProfilePoint {
		const char*    tag;
		thread_id_type threadId;
		u64            startTime;
		u64            endTime;
	};

	struct ThreadInfo {
		std::string    threadName;
		thread_id_type threadId;
		u64            startTime;
		u64            endTime;
	};

	typedef std::map<std::string, std::vector<float> > VariablesRecord;

private:
	void writeProfileLog();
	void writeRecordedVariables();

	std::string getDateTimeString() const;

private:
	ProfilePoint profilePoints[NB_POINTS];
	std::map<thread_id_type, ThreadInfo> threadsInfo;

	VariablesRecord recordedVariables;

	volatile u32 writeIndex;
	bool canWrite;
};

class ProfileScope {
public:
	ProfileScope(const char* _tag)
		: tag(_tag)
		, startTime(Time::getUs()) {
		
		arx_assert(_tag != 0 && _tag != "");
	}

	~ProfileScope() {
		Profiler::addProfilePoint(tag, Thread::getCurrentThreadId(), startTime, Time::getUs());
	}

private:
	const char* tag;
	u64 startTime;
};

#define ARX_PROFILE(tag)           ProfileScope profileScope##__LINE__(#tag)
#define ARX_PROFILE_FUNC()         ProfileScope profileScope##__LINE__(__FUNCTION__)
#define ARX_PROFILE_RECORD(name)   Profiler::recordVariable(#name, name)

#endif // ARX_PLATFORM_PROFILE_H
