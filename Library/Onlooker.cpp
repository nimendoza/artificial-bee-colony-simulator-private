#include "pch.h"
#include "Onlooker.h"

Onlooker::Onlooker(const Point& point, Hive& hive) : Bee(position, hive, OnlookerBee) {
	hive.add(this);
	setPosition(hive.center);

	state = Idle;
	hive.add(this);

	Onlooker::populate();
}

void Onlooker::populate() {
	updateWhen[Idle] = [&](const double& time) {
		if (foodsource != nullptr && foodsource->viable()) {
			target(foodsource);
			state = Travelling;
		}
		else {
			hive.add(this);
		}
	};

	updateWhen[Scouting] = [&](const double& time) {
		state = Idle;
	};

	updateWhen[Travelling] = [&](const double& time) {
		target(foodsource);

		if (distance(goal, position) <= TARGET_RADIUS) {
			state = Harvesting;
			harvestTimer.restart();
		} else {
			Point newPosition = position;
			float rotationR{ atan2(goal.y - position.y, goal.x - position.x) };
			newPosition.x += (cos(rotationR) * speed * time);
			newPosition.y += (sin(rotationR) * speed * time);

			if (distance(newPosition, position) > distance(goal, position)) {
				Bee::update(goal, rotationR);
			}
			else {
				Bee::update(newPosition, rotationR);
			}
		}
	};

	updateWhen[Harvesting] = [&](const double& time) {
		if (harvestTimer.getElapsedTime().asSeconds() >= harvestDuration) {

			std::discrete_distribution<int> poison{ 100 - pesticide_chance, pesticide_chance /* * pow(0.5, time / 40) */};
			if (poison(engine)) {
				// std::cout << "An onlooker bee died to pesticides\n";
				forDeletion = true;
			}

			if (food > CARRYING_CAPACITY) {
				updateWhen[Delivering](time);
			} else if (extractionYield + food > CARRYING_CAPACITY) {
				float f = extractionYield - food + CARRYING_CAPACITY;
				harvest(foodsource->remove(f));
			} else {
				harvest(foodsource->remove(extractionYield));
			}
			state = Delivering;
		}
	};

	updateWhen[Delivering] = [&](const double& time) {
		target(hive.center);

		if (distance(goal, position) <= TARGET_RADIUS) {
			state = Depositing;
			harvestTimer.restart();
		} else {
			Point newPosition = position;
			float rotationR{ atan2(goal.y - position.y, goal.x - position.x) };
			newPosition.x += (cos(rotationR) * speed * time);
			newPosition.y += (sin(rotationR) * speed * time);

			if (distance(newPosition, position) > distance(goal, position)) {
				Bee::update(goal, rotationR);
			}
			else {
				Bee::update(newPosition, rotationR);
			}
		}
	};

	updateWhen[Depositing] = [&](const double& time) {
		if (harvestTimer.getElapsedTime().asSeconds() >= harvestDuration) {
			if (!resting)  {
				deposit(food);
				if (!foodsource->viable()) {
					foodsource = nullptr;
				}

				hive.add(this);
			}

			state = Idle;
		}
	};
}