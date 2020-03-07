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
	int wt_id;
	std::string curr_step;
	int step_num;
public:
	NTask	(
				float timeToComplete,
				Payload pl,
				int ramNeeded,
				bool selfSend,
				int wt_id,
				std::string cs,
				int step_num
			):
				timeToComplete(timeToComplete),
				pl(pl),
				ramNeeded(ramNeeded),
				selfSend(selfSend),
				wt_id(wt_id),
				curr_step(cs),
				step_num(step_num)
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

	std::string toString(){
		return "RamNeed:" + std::to_string(ramNeeded) + ", Time to complete:" + std::to_string(timeToComplete) + "StepNum: " + curr_step + " Packet: " + pl.toString();
	}

	std::string toStringShort(){
		return pl.cycle_id + "_" + curr_step;
	}

	std::string getStepName(){
		return curr_step;
	}

	std::string getCycleId(){
		return pl.cycle_id;
	}

	const int& getWtId() const {
		return wt_id;
	}
	
	const int& getStepNum() const {
		return step_num;
	}

	
};



#endif /* SCRATCH_IOTSIM_WIFI_NTASK_H_ */
