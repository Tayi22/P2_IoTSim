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
	float time_alive_save_time;
	int retry_alive;
	int max_retries;
	int retries;

	system_clock::time_point start_time;

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
				start_time = system_clock::now();
				time_alive = 0.f;
				retry_alive = 0;
				time_alive_save_time = 0;
			}


	int check(){
		passed_time = duration_cast<milliseconds>(system_clock::now() - start_time).count();
		if (passed_time < 1) return 0;
		time_alive = time_alive_save_time + passed_time;
		if (passed_time > max_time){
			retries++;
			retry_alive++;
			start_time = system_clock::now();
			if (retries > max_retries){
				retries = 0;
				return 2;
			}
			time_alive_save_time += passed_time;
			return 1;
		}
		return 0;


/*
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
		*/

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

	float getTimeSuccess() const {
		return duration_cast<milliseconds>(system_clock::now() - start_time).count();
	}

	int getTries() const {
		return retry_alive + 1;
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
