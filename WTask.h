/*
 * WTask.h
 *
 *  Created on: Feb 3, 2020
 *      Author: richard
 */

#ifndef SCRATCH_IOTSIM_WIFI_WTASK_H_
#define SCRATCH_IOTSIM_WIFI_WTASK_H_

#include "Sim_Include.h"

using namespace std::chrono;

class WTask{
private:
	float passed_time;
	float max_time;
	float time_alive;
	int retry_alive;
	int max_retries;
	int retries;

	system_clock::time_point last_update;

	Payload pl;

	int affected_node_index;

public:
	WTask	(
				float max_time,
				int max_retries,
				Payload pl,
				int affected_node_index
			):
				max_time(max_time),
				max_retries(max_retries),
				pl(pl),
				affected_node_index(affected_node_index)
			{
				passed_time = 0.f;
				retries = 0;
				last_update = system_clock::now();
				time_alive = 0.f;
				retry_alive = 0;
			}


	int check(){
		system_clock::time_point cur = system_clock::now();
		float time_between_checks = duration_cast<milliseconds>(cur - last_update).count();
		if (time_between_checks < 1) return 0;

		last_update = cur;
		passed_time += time_between_checks;
		time_alive += time_between_checks;

		if (passed_time > max_time){
			retries++;
			retry_alive++;
			if (retries > max_retries){
				retries = 0;
				passed_time = 0;
				return 2;
			}
			passed_time = 0;
			return 1;
		}
		return 0;

	}

	std::string getCycleId() const {
		return pl.cycle_id;
	}

	std::string toString() const {
		return "Cycle: " + getCycleId() + " Tries: " + std::to_string(retries) + "/" + std::to_string(max_retries) + " Time Alive: " + std::to_string(time_alive);
	}

	Payload& getPl() {
		return pl;
	}

	float getTimeAlive() const {
		return time_alive;
	}

	float getAvGTimeAlive() const {
		int temp = retry_alive - 1;
		if (temp <= 0) temp = 1;
		return time_alive / temp;
	}

	int getAffectedNodeIndex() const {
		return affected_node_index;
	}
};



#endif /* SCRATCH_IOTSIM_WIFI_WTASK_H_ */
