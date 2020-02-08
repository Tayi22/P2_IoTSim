/*
 * NTask.h
 *
 *  Created on: Feb 2, 2020
 *      Author: richard
 */

#ifndef SCRATCH_IOTSIM_WIFI_NTASK_H_
#define SCRATCH_IOTSIM_WIFI_NTASK_H_

using namespace std::chrono;

class NTask{

private:
	system_clock::time_point startedAt;
	system_clock::time_point lastUpdate;
	float timeToComplete;
	float takenTime;
	Payload pl;
	int ramNeeded;
	bool selfSend;
	std::string sendToId;
public:
	NTask	(
				float timeToComplete,
				Payload pl,
				int ramNeeded,
				bool selfSend
			):
				timeToComplete(timeToComplete),
				pl(pl),
				ramNeeded(ramNeeded),
				selfSend(selfSend)
			{
				startedAt = system_clock::now();
				lastUpdate = startedAt;
				takenTime = 0;
			}

	float checkTask(){
		if (timeToComplete == 0) return 0;
		takenTime = duration_cast<milliseconds>(system_clock::now() - lastUpdate).count();
		if (takenTime < 1) return -1.f;
		if (timeToComplete > takenTime) return -1.f;
		return takenTime;

		/*
		lastUpdate = checkNow;
		timeToComplete -= passedTime;
		takenTime += passedTime;

		if (timeToComplete <= 0.f){
			return takenTime;
		}
		return -1.f;
		*/
	}

	int getRamNeeded() const {
		return ramNeeded;
	}

	const Payload& getPl() const {
		return pl;
	}

	bool isSelfSend() const {
		return selfSend;
	}
};



#endif /* SCRATCH_IOTSIM_WIFI_NTASK_H_ */
