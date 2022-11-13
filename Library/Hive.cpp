#include "pch.h"
#include "Hive.h"
#include "Node.h"
#include "Grid.h"
#include "Foodsource.h"
#include "Onlooker.h"
const float Hive::WIDTH{ 200.0F };
const float Hive::HEIGHT{ 200.0F };
const float Hive::DANCE_DURATION{ 0.05F / 2 };

Hive::Hive(const Point& position) : Entity(position, Color::White, Color::Yellow), dimensions(WIDTH, HEIGHT), body(dimensions), food{}, count{}, idles{}, data{}, text(), center(position.x + dimensions.x / 2, position.y + dimensions.y / 2), dancing(false) {
	body.setPosition(position);
	body.setOutlineThickness(-10);
	body.setOutlineColor(outline);
	body.setFillColor(fill);

	text.setPosition(position + Point(10, 10));
	text.setCharacterSize(16);
	text.setOutlineColor(Color::White);
}

void Hive::update(const double& time) {
	if (dancing && danceTimer.getElapsedTime().asSeconds() > DANCE_DURATION) {
		std::vector<std::pair<Foodsource*, float>> weights{};
		float sum{};
		float minY{ *begin(data)->second.first };
		float maxY{ *begin(data)->second.first };
		float minD{ begin(data)->second.second };
		float maxD{ begin(data)->second.second };
		for (auto i{ begin(data) }; i != end(data); i++) {
			auto pair = i->second;
			minY = std::min(minY, *pair.first);
			maxY = std::max(maxY, *pair.first);
			minD = std::min(minD, pair.second);
			maxD = std::max(maxD, pair.second);
		}

		for (auto i{ begin(data) }; i != end(data); i++) {
			float weight = compute(i->second, minY, maxY, minD, maxD);
			std::pair<Foodsource* const, float> pair(i->first, weight);
			weights.push_back(pair);
			sum += weight;
		}

		std::sort(begin(weights), end(weights), [](const std::pair<Foodsource* const, float>& lhs, const std::pair<Foodsource* const, float>& rhs) {
			return lhs.second > rhs.second;
			});

		int count{};
		for (auto i{ begin(idles) }; i != end(idles); i++) {
			std::vector<int> w(weights.size());
			for (int i{}; i < w.size(); i++) {
				for (int j{}; j < w.size() - 1; j++) {
					w[j] = i + 1;
				}
			}

			std::discrete_distribution<int> distribution(begin(w), end(w));
			int roll{ distribution(engine) };
			(*i)->target(weights[roll].first);
			(*i)->state = Travelling;
			count++;
		}
		std::cout << count << '\n';

		validate();
		dancing = false;
	}

	if (node != nullptr && !node->contains(position)) {
		node->remove(this);
		node = nullptr;
	}

	if (node == nullptr) {
		node = Grid::get()->nodeAt(position);
		node->hives.push_back(this);
	}
}
void Hive::dance() {
	dancing = true;
	danceTimer.restart();
}
float Hive::compute(const std::pair<float*, float>& foodData,
	const float& minYield, const float& maxYield,
	const float& minDistance, const float& maxDistance) {
	float offsetFromMinYield = *foodData.first - minYield;
	float yieldRange = maxYield - minYield;

	float offsetFromMaxDistance = maxDistance - foodData.second;
	float distanceRange = maxDistance - minDistance;

	float result;
	if (yieldRange == 0.0f && distanceRange == 0.0f)
	{	// Avoid dividing by zero and apply uniform fitness
		result = 1.0f;
	} else if (yieldRange == 0.0f)
	{	// We apply uniform yield weight and compute distance
		result = ((1.0f) + (offsetFromMaxDistance / distanceRange)) / 2.0f;
	} else if (distanceRange == 0.0f)
	{	// We apply uniform distance weight and compute yield
		result = ((offsetFromMinYield / yieldRange) + (1.0f) / 2.0f);
	} else
	{	// We have two valid values
		result = ((offsetFromMinYield / yieldRange) + (offsetFromMaxDistance / distanceRange)) / 2.0f;
	}
	return result;
}
void Hive::remove(Onlooker* const bee) {
	for (auto i{ begin(idles) }; i != end(idles); i++) {
		if (*i == bee) {
			idles.erase(i);
			break;
		}
	}
}
void Hive::add(Onlooker* const bee) {
	bool has{ false };
	for (auto i{ begin(idles) }; i != end(idles); i++) {
		if (*i == bee) {
			has = true;
			break;
		}
	}

	if (!has) {
		idles.push_back(bee);
	}
}
void Hive::validate() {
	bool removed{ true };
	do {
		removed = false;
		for (auto i{ begin(idles) }; i != end(idles); i++) {
			if ((*i)->state != Idle) {
				idles.erase(i);
				removed = true;
				break;
			}
		}
	} while (removed);
}
void Hive::remove(Foodsource* const foodsource) {
	for (auto i{ begin(data) }; i != end(data); i++) {
		if (i->first == foodsource) {
			data.erase(i);
			break;
		}
	}
}
void Hive::add(const float& amount) {
	food += amount;
}
float Hive::remove(float amount) {
	amount = std::min(amount, food);
	food -= amount;
	return amount;
}
void Hive::render(Window& window) {
	window.draw(body);
	window.draw(text);
}